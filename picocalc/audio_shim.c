/* audio_shim.c — upstream's cabal_audio_* over the PicoCalc's PWM audio
 * (design.md §5.3, §8).
 *
 * PWM on GP26/27, both channels of one slice, one compare word a stereo
 * frame; two DMA channels chained to each other ping-pong a two-half
 * ring, paced by the slice's wrap (hardware-notes.md §5.1-5.3). Adapted
 * from pico-oric's audio.c, which is a queue its producer pushes; here
 * upstream's mixer is pulled inside the refill IRQ, as upstream's I2S
 * driver does (§8.2): mixing from the main loop underran whenever an
 * engine frame was longer than a buffer.
 *
 * The IRQ is DMA_IRQ_1, because that is the line upstream masks: its
 * lockMutex() for the mixer's state, and its PSRAM allocator for the
 * heap, which the mixer uses from the IRQ (it frees finished channels).
 * Everything here runs on core 0.
 */

#include <stddef.h>
#include <stdint.h>

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "pico/time.h"

#include "audio.h"
#include "sound.h"

#define AUDIO_PIN_L 26u   /* PWM5 A, hardware-notes.md §1.1 */
#define AUDIO_PIN_R 27u   /* PWM5 B */

/* 12 bits and a 73.2 kHz carrier at 300 MHz, each frame written twice
 * (§8.1). The divider follows clk_sys, so the carrier stays put if the
 * clock moves. */
#define PWM_TOP        SOUND_TOP
#define PWM_DIV_HZ     300000000u
#define PWM_MID        SOUND_MID
#define OVERSAMPLE     2u
#define SLOTS_PER_HALF (SOUND_FRAMES_PER_HALF * OVERSAMPLE)
#define RING_SLOTS     (2u * SLOTS_PER_HALF)
#define RING_BYTES     (RING_SLOTS * sizeof(uint32_t))
#define RING_BITS      13u
_Static_assert(RING_BYTES == (1u << RING_BITS), "the ring must be 2^RING_BITS bytes");

#define SILENCE (PWM_MID | (PWM_MID << 16))

sound_stats_t g_sound;

/* Aligned to its own size: channel_config_set_ring wraps the low
 * RING_BITS of the read address (hardware-notes.md §5.3). */
static uint32_t s_ring[RING_SLOTS] __attribute__((aligned(RING_BYTES)));
/* One half of the mixer's stereo int16 output. */
static int16_t s_mix[2u * SOUND_FRAMES_PER_HALF] __attribute__((aligned(4)));

static void (*volatile mixer_callback)(uint8_t *stream, int len);
static bool s_running;
static int s_ch[2];
static volatile int32_t s_gain = SOUND_UNITY;
static int s_volume = SOUND_UNITY;
static volatile bool s_enabled = true;

void cabal_audio_set_mixer_callback(void (*callback)(uint8_t *stream, int len));

static uint32_t pwm_div(void) {
    uint32_t div = clock_get_hz(clk_sys) / PWM_DIV_HZ;
    return div < 1u ? 1u : div;
}

/* The integer part of the rate (§8.2): the mixer only resamples to it. */
unsigned cabal_audio_sample_rate(void) {
    return (unsigned)(clock_get_hz(clk_sys) / (pwm_div() * (PWM_TOP + 1u) * OVERSAMPLE));
}

static void __not_in_flash_func(fill_half)(uint32_t *dst) {
    void (*callback)(uint8_t *, int) = mixer_callback;
    if (callback == NULL || !s_enabled) {
        for (unsigned i = 0; i < SLOTS_PER_HALF; i++) dst[i] = SILENCE;
        return;
    }
    callback((uint8_t *)s_mix, (int)sizeof s_mix);
    int32_t gain = s_gain;
    for (unsigned i = 0; i < SOUND_FRAMES_PER_HALF; i++) {
        uint32_t w = sound_word(s_mix[2u * i], s_mix[2u * i + 1u], gain);
        dst[2u * i] = w;
        dst[2u * i + 1u] = w;
    }
}

static void __not_in_flash_func(refill)(unsigned which) {
    int ch = s_ch[which], other = s_ch[which ^ 1u];
    uint32_t *half = &s_ring[which * SLOTS_PER_HALF];

    /* As pico-oric's (hardware-notes.md §5.3): this channel running again
     * already means the other half drained too and chained back before
     * this IRQ ran, so the deadline was missed and the channel is
     * replaying through the wrap; it is left alone until its next
     * completion. Nor is this half written while the other channel's
     * read address is inside it. The mixer is not called either: its
     * sound is late, not lost. */
    uintptr_t off = (uintptr_t)dma_hw->ch[other].read_addr - (uintptr_t)half;
    if (dma_channel_is_busy(ch) ||
        (dma_channel_is_busy(other) && off < SLOTS_PER_HALF * sizeof(uint32_t))) {
        g_sound.late++;
        return;
    }
    fill_half(half);
    /* A trigger carries the read address on from where the last run
     * ended; set it back, and the count with it (hardware-notes.md §5.3). */
    dma_channel_set_read_addr(ch, half, false);
    dma_channel_set_trans_count(ch, SLOTS_PER_HALF, false);
}

static void __not_in_flash_func(audio_irq)(void) {
    uint32_t t0 = time_us_32();
    for (unsigned which = 0; which < 2u; which++) {
        uint32_t bit = 1u << s_ch[which];
        if (dma_hw->ints1 & bit) {
            dma_hw->ints1 = bit;
            refill(which);
            g_sound.halves++;
        }
    }
    uint32_t us = time_us_32() - t0;
    g_sound.mix_us += us;
    if (us > g_sound.mix_max_us) g_sound.mix_max_us = us;
}

/* The slice, the ring and the IRQ, once: OSystem is torn down and made
 * again at each launch, and the output runs on through it, silent while
 * no mixer is set. */
static void start(void) {
    gpio_set_function(AUDIO_PIN_L, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_PIN_R, GPIO_FUNC_PWM);
    /* Both pins are one slice; ask, do not hard-code
     * (hardware-notes.md §5.1). */
    unsigned slice = pwm_gpio_to_slice_num(AUDIO_PIN_L);

    pwm_config pc = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&pc, pwm_div());
    pwm_config_set_wrap(&pc, PWM_TOP);
    pwm_init(slice, &pc, false);
    pwm_set_both_levels(slice, PWM_MID, PWM_MID);

    for (unsigned i = 0; i < RING_SLOTS; i++) s_ring[i] = SILENCE;

    s_ch[0] = dma_claim_unused_channel(true);
    s_ch[1] = dma_claim_unused_channel(true);
    for (unsigned which = 0; which < 2u; which++) {
        dma_channel_config c = dma_channel_get_default_config(s_ch[which]);
        channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        channel_config_set_dreq(&c, pwm_get_dreq(slice));
        channel_config_set_ring(&c, false, RING_BITS);
        channel_config_set_chain_to(&c, s_ch[which ^ 1u]);
        dma_channel_configure(s_ch[which], &c,
                              &pwm_hw->slice[slice].cc,
                              &s_ring[which * SLOTS_PER_HALF],
                              SLOTS_PER_HALF, false);
        dma_channel_set_irq1_enabled(s_ch[which], true);
    }

    /* Above the default 0x80, so a refill preempts everything else on
     * core 0 (hardware-notes.md §5.4). */
    irq_set_exclusive_handler(DMA_IRQ_1, audio_irq);
    irq_set_priority(DMA_IRQ_1, 0x40);
    irq_set_enabled(DMA_IRQ_1, true);

    pwm_set_enabled(slice, true);
    dma_channel_start(s_ch[0]);
    s_running = true;
}

void cabal_audio_set_mixer_callback(void (*callback)(uint8_t *stream, int len)) {
    mixer_callback = callback;
}

bool cabal_audio_init(void) {
    if (!s_running) start();
    return true;
}

/* The output keeps running, silent: the mixer is about to go. */
void cabal_audio_shutdown(void) {
    mixer_callback = NULL;
}

bool cabal_audio_is_initialized(void) {
    return s_running;
}

#ifndef PICOCALC_QUEST_AUDIO_STALL
#define PICOCALC_QUEST_AUDIO_STALL 0
#endif

/* Upstream's main loop calls this to feed its I2S driver; the IRQ pulls
 * here, so there is nothing to do. The scratch build that forces the
 * late path (§16 M5, EL §6.4) masks core 0's interrupts here for
 * PICOCALC_QUEST_AUDIO_STALL ms every five seconds. */
void cabal_audio_process_frame(void) {
#if PICOCALC_QUEST_AUDIO_STALL
    static uint32_t next_us;
    uint32_t now = time_us_32();
    if (next_us == 0) next_us = now + 5000000u;
    if ((int32_t)(now - next_us) >= 0) {
        next_us = now + 5000000u;
        uint32_t irq = save_and_disable_interrupts();
        busy_wait_us_32(PICOCALC_QUEST_AUDIO_STALL * 1000u);
        restore_interrupts(irq);
    }
#endif
}

bool cabal_audio_needs_samples(void) {
    return false;
}

void cabal_audio_set_volume(int volume) {
    if (volume < 0) volume = 0;
    if (volume > 255) volume = 255;
    s_volume = volume;
    s_gain = volume;
}

int cabal_audio_get_volume(void) {
    return s_volume;
}

void cabal_audio_set_enabled(bool enabled) {
    s_enabled = enabled;
}

bool cabal_audio_is_enabled(void) {
    return s_enabled;
}

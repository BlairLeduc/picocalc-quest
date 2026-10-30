/* core1.c — core 1's loop: the presenter, the keyboard, the southbridge
 * and the log (design.md §6.1, §7.3, §9.1, §14.3). */

#include "core1.h"

#include <stdio.h>

#include "hardware/dma.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#include "board.h"
#include "framepool.h"
#include "kbd.h"
#include "lcd.h"
#include "log.h"
#include "page.h"
#include "picocalc_input.h"
#include "sound.h"
#include "southbridge.h"
#include "video.h"

/* The battery is refreshed by the MCU every 20 s (hardware-notes.md §6);
 * once a second is plenty. The keyboard's poll, every KBD_POLL_US, is
 * what keeps the MCU's bus watchdog fed (§6.1). */
#define BAT_POLL_US   1000000u
#define HEARTBEAT_US  1000000u

core1_bringup_t g_core1 = { .sb_version = -1 };

#define RGB565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

/* Where the picture goes (§7.1): upstream's 320x240 framebuffer at y 40. */
#define FRAME_Y 40u
#define FRAME_W 320u
#define FRAME_H 240u

/* The log is pumped this often within a present, so a frame (some 19
 * ms) does not leave the UART idle for its length (§14.2). */
#define PUMP_ROWS 16u

#define STACK_PAINT 0xC1C1C1C1u
extern uint32_t __StackOneBottom, __StackOneTop;

volatile uint32_t g_core0_delay_us;
volatile uint32_t g_core0_update_us, g_core0_updates;

static volatile bool s_park, s_parked;

void core1_park(void) {
    s_park = true;
    uint32_t t0 = time_us_32();
    while (!s_parked && time_us_32() - t0 < 100000u) tight_loop_contents();
}

#if PICOCALC_QUEST_TEST_PATTERN
/* The corner-coded test pattern (EL §13.3, hardware-notes.md §10):
 * orientation, colour order and all four edges, by eye. Drawn at boot
 * only in a build that asks for it (M1's bring-up); otherwise the panel
 * stays black until the first frame.
 *
 *   - red square top left, green top right, blue bottom left, white
 *     bottom right, 32 pixels each;
 *   - a one-pixel white border round the whole panel;
 *   - in the picture's 240 rows: eight colour bars, white, yellow, cyan,
 *     green, magenta, red, blue, black, 40 pixels each, over a grey ramp
 *     of 32 steps in the last 40 rows, black on the left.
 */
static void test_pattern(void) {
    static const uint16_t bars[8] = {
        RGB565(255, 255, 255), RGB565(255, 255, 0), RGB565(0, 255, 255), RGB565(0, 255, 0),
        RGB565(255, 0, 255),   RGB565(255, 0, 0),   RGB565(0, 0, 255),   RGB565(0, 0, 0),
    };
    const uint16_t white = RGB565(255, 255, 255);
    const unsigned w = LCD_PANEL_W, h = LCD_PANEL_H, sq = 32u;

    for (unsigned i = 0; i < 8u; i++) lcd_fill(i * 40u, FRAME_Y, 40u, FRAME_H - 40u, bars[i]);
    for (unsigned i = 0; i < 32u; i++) {
        unsigned v = i * 255u / 31u;
        lcd_fill(i * 10u, FRAME_Y + FRAME_H - 40u, 10u, 40u, RGB565(v, v, v));
    }

    lcd_fill(0, 0, w, 1, white);
    lcd_fill(0, h - 1u, w, 1, white);
    lcd_fill(0, 0, 1, h, white);
    lcd_fill(w - 1u, 0, 1, h, white);

    lcd_fill(0, 0, sq, sq, RGB565(255, 0, 0));
    lcd_fill(w - sq, 0, sq, sq, RGB565(0, 255, 0));
    lcd_fill(0, h - sq, sq, sq, RGB565(0, 0, 255));
    lcd_fill(w - sq, h - sq, sq, sq, white);
}
#endif

/* ---- the presenter (§7.3) ----------------------------------------- */

typedef struct {
    uint32_t presents;
    uint32_t last_us;    /* the last present, wall time   */
    uint32_t max_us;     /* the longest since boot        */
} present_stats_t;

static present_stats_t s_present;
static uint16_t s_line[2][FRAME_W];

#if PICOCALC_QUEST_FB_READ == FB_READ_DMA
/* Rows of indices, copied out of the PSRAM a row ahead of the one being
 * expanded. */
static uint32_t s_idx[2][FRAME_W / 4u];
static int s_row_dma = -1;

static void row_dma_start(const uint8_t *src, unsigned slot) {
    dma_channel_config c = dma_channel_get_default_config((unsigned)s_row_dma);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, true);
    dma_channel_configure((unsigned)s_row_dma, &c, s_idx[slot], src, FRAME_W / 4u, true);
}
#endif

/* Every row of the frame, through its palette, into one line buffer while
 * the other is on the wire (hardware-notes.md §4.6). */
static void present(int i) {
    const uint8_t *px = video_pixels(i);
    const uint16_t *pal = video_palette(i);
    uint32_t t0 = time_us_32();

    lcd_blit_begin(0, FRAME_Y, FRAME_W, FRAME_H);
#if PICOCALC_QUEST_FB_READ == FB_READ_DMA
    row_dma_start(px, 0);
#endif
    for (unsigned row = 0; row < FRAME_H; row++) {
        uint16_t *line = s_line[row & 1u];
#if PICOCALC_QUEST_FB_READ == FB_READ_DMA
        dma_channel_wait_for_finish_blocking((unsigned)s_row_dma);
        if (row + 1u < FRAME_H) row_dma_start(px + (row + 1u) * FRAME_W, (row + 1u) & 1u);
        frame_expand_row((const uint8_t *)s_idx[row & 1u], pal, line, FRAME_W);
#else
        frame_expand_row(px + row * FRAME_W, pal, line, FRAME_W);
#endif
        lcd_blit_row(line, FRAME_W);
        if (row % PUMP_ROWS == PUMP_ROWS - 1u) log_pump();
    }
    lcd_blit_end();

    uint32_t us = time_us_32() - t0;
    s_present.presents++;
    s_present.last_us = us;
    if (us > s_present.max_us) s_present.max_us = us;
}

/* ---- core 1's stack ------------------------------------------------- */

void core1_paint_stack(void) {
    for (uint32_t *p = &__StackOneBottom; p < &__StackOneTop; p++) *p = STACK_PAINT;
}

/* The deepest core 1's stack has been, from the paint left untouched. */
static uint32_t stack_used(void) {
    const uint32_t *p = &__StackOneBottom;
    while (p < &__StackOneTop && *p == STACK_PAINT) p++;
    return (uint32_t)((uintptr_t)&__StackOneTop - (uintptr_t)p);
}

void core1_main(void) {
    /* 1. The southbridge first: a dead bus is the first thing to know
     *    about (hardware-notes.md §10). */
    g_core1.i2c_hz = sb_init();
    uint8_t r[2];
    uint32_t t0 = time_us_32();
    if (sb_read(SB_REG_VER, r) == SB_OK) g_core1.sb_version = r[1];
    g_core1.i2c_us = time_us_32() - t0;

    /* 2. The LCD, cleared before DISPON, then the pattern if asked for. */
    t0 = time_us_32();
    g_core1.spi_hz = lcd_init();
    g_core1.lcd_us = time_us_32() - t0;
#if PICOCALC_QUEST_TEST_PATTERN
    t0 = time_us_32();
    test_pattern();
    g_core1.pattern_us = time_us_32() - t0;
#endif

    board_temp_init();
#if PICOCALC_QUEST_FB_READ == FB_READ_DMA
    s_row_dma = dma_claim_unused_channel(true);
#endif

    __dmb();
    g_core1.ready = true;

    bool bands_cleared = false;
    page_t page = PAGE_NONE;
    int battery = -1, temp_c = board_temp_c();
    uint32_t now = time_us_32();
    uint32_t next_bat = now, next_kbd = now, next_beat = now + HEARTBEAT_US;
    uint32_t seconds = 0, loops = 0;
    uint32_t last_pub = 0, last_drop = 0, last_presents = 0, last_delay = g_core0_delay_us;
    uint32_t last_update = g_core0_update_us, last_updates = g_core0_updates;
    uint32_t last_taken = 0, last_wait = 0;
    uint32_t last_mix = g_sound.mix_us, last_halves = g_sound.halves;
    for (;;) {
        log_pump();
        loops++;
        if (s_park) {
            /* Between transactions: nothing of the southbridge's is in
             * flight here. */
            s_parked = true;
            for (;;) tight_loop_contents();
        }

        /* A page in place of the selector (§13.1) covers the panel and
         * stays; nothing is presented over it. */
        page_t wanted = page_wanted();
        if (wanted != page) {
            page = wanted;
            page_draw(page);
        }

        int frame = video_take();
        if (frame >= 0 && page != PAGE_NONE) {
            video_release(frame);
            frame = -1;
        }
        if (frame >= 0) {
            if (!bands_cleared) {
                /* The bands are black (§7.4): the test pattern's
                 * corners and border, if drawn, go with the first
                 * frame. */
                lcd_fill(0, 0, LCD_PANEL_W, FRAME_Y, 0);
                lcd_fill(0, FRAME_Y + FRAME_H, LCD_PANEL_W, LCD_PANEL_H - FRAME_Y - FRAME_H, 0);
                bands_cleared = true;
            }
            if (PICOCALC_QUEST_FB_READ != FB_READ_NONE) present(frame);
            video_release(frame);
        }

        /* The keyboard (§9.1): the MCU's FIFO every KBD_POLL_US, and in
         * the development build the UART's keys every loop (§14.1). */
        kbd_uart();
        now = time_us_32();
        if ((int32_t)(now - next_kbd) >= 0) {
            next_kbd = now + KBD_POLL_US;
            kbd_poll();
        }

        now = time_us_32();
        if ((int32_t)(now - next_bat) >= 0) {
            next_bat = now + BAT_POLL_US;
            battery = sb_read(SB_REG_BAT, r) == SB_OK ? r[1] : -1;
            temp_c = board_temp_c();
        }

        /* The heartbeat's skeleton (§14.3): core 0's counters join it as
         * the milestones bring them. */
        now = time_us_32();
        if ((int32_t)(now - next_beat) >= 0) {
            next_beat += HEARTBEAT_US;
            seconds++;
            uint32_t pub, drop, delay = g_core0_delay_us;
            uint32_t update = g_core0_update_us, updates = g_core0_updates;
            uint32_t taken = g_input_taken, wait = g_input_wait_sum_us;
            uint32_t mix = g_sound.mix_us, halves = g_sound.halves;
            uint32_t mix_max = g_sound.mix_max_us;
            g_sound.mix_max_us = 0;
            video_counts(&pub, &drop);
            /* Core 0's busy share: the second's wall time not in
             * cabal_delay (§14.3), in thousandths. */
            uint32_t idle = delay - last_delay;
            uint32_t busy = idle >= HEARTBEAT_US ? 0 : 1000u - idle / (HEARTBEAT_US / 1000u);
            /* Of the second, the share in upstream's screen update. */
            uint32_t upd = (update - last_update) / (HEARTBEAT_US / 1000u);
            /* And in the audio IRQ, which the busy share includes (§8.3). */
            uint32_t mixed = (mix - last_mix) / (HEARTBEAT_US / 1000u);
            printf("hb %lu: core 0 busy %lu.%lu%%, update %lu.%lu%% in %lu calls, "
                   "audio %lu.%lu%% in %lu halves (max %lu us), late %lu, frames %lu, presented %lu, dropped %lu, "
                   "present %lu us (max %lu), publish max %lu us, core 1 stack %lu B; "
                   "keys %lu (wait mean %lu max %lu us), ring full %lu, unknown %lu, "
                   "stray %lu; temp %d C, battery %d%%%s, i2c errors %lu, log dropped %lu, "
                   "core 1 loops %lu\n",
                   (unsigned long)seconds, (unsigned long)(busy / 10u),
                   (unsigned long)(busy % 10u), (unsigned long)(upd / 10u),
                   (unsigned long)(upd % 10u), (unsigned long)(updates - last_updates),
                   (unsigned long)(mixed / 10u), (unsigned long)(mixed % 10u),
                   (unsigned long)(halves - last_halves), (unsigned long)mix_max,
                   (unsigned long)g_sound.late,
                   (unsigned long)(pub - last_pub),
                   (unsigned long)(s_present.presents - last_presents),
                   (unsigned long)(drop - last_drop), (unsigned long)s_present.last_us,
                   (unsigned long)s_present.max_us, (unsigned long)g_video_publish_max_us,
                   (unsigned long)stack_used(), (unsigned long)(taken - last_taken),
                   (unsigned long)(taken == last_taken ? 0 : (wait - last_wait) / (taken - last_taken)),
                   (unsigned long)g_input_wait_max_us, (unsigned long)kbd_overflows(),
                   (unsigned long)g_input_unknown, (unsigned long)g_input_stray, temp_c,
                   battery < 0 ? -1 : battery & 0x7f,
                   battery >= 0 && (battery & 0x80) ? " charging" : "",
                   (unsigned long)sb_error_count(), (unsigned long)log_dropped(),
                   (unsigned long)loops);
            last_pub = pub;
            last_drop = drop;
            last_presents = s_present.presents;
            last_delay = delay;
            last_update = update;
            last_updates = updates;
            last_taken = taken;
            last_wait = wait;
            last_mix = mix;
            last_halves = halves;
            loops = 0;
        }

        /* Never sleep_us here (hardware-notes.md §9.7). */
        if (frame < 0) busy_wait_us_32(20);
    }
}

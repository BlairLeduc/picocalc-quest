/* sound.h — the PWM audio's counters, for the heartbeat (design.md §8.3,
 * §14.3). The output itself is upstream's cabal_audio_* API, in
 * audio_shim.c; this header is named apart from upstream's audio.h.
 */
#ifndef PICOCALC_QUEST_SOUND_H
#define PICOCALC_QUEST_SOUND_H

#include <stdint.h>

/* Kept by the refill IRQ on core 0; read anywhere, each a word. */
typedef struct {
    volatile uint32_t halves;      /* halves refilled                       */
    volatile uint32_t late;        /* refills that found the deadline missed */
    volatile uint32_t mix_us;      /* time in the IRQ since boot            */
    volatile uint32_t mix_max_us;  /* longest refill; the reader resets it  */
} sound_stats_t;

extern sound_stats_t g_sound;

/* Frames a half (§8.3), and the output's rate, both fixed at init. */
#define SOUND_FRAMES_PER_HALF 512u
unsigned cabal_audio_sample_rate(void);

/* 12 bits a compare value (§8.1); silence is the mid-point. */
#define SOUND_TOP 4095u
#define SOUND_MID ((SOUND_TOP + 1u) / 2u)
/* Upstream's volume, 0-255; 128 is unity gain. */
#define SOUND_UNITY 128

/* One of the mixer's int16 samples to a compare value: the gain, then 16
 * bits to 12 about the mid-point, saturated. */
static inline uint32_t sound_level(int32_t s, int32_t gain) {
    int32_t v = (int32_t)SOUND_MID + ((s * gain) >> 11);
    if (v < 0) v = 0;
    if (v > (int32_t)SOUND_TOP) v = SOUND_TOP;
    return (uint32_t)v;
}

/* A stereo frame to the slice's compare word: left in channel A's half,
 * right in B's (hardware-notes.md §5.1). */
static inline uint32_t sound_word(int16_t left, int16_t right, int32_t gain) {
    return sound_level(left, gain) | (sound_level(right, gain) << 16);
}

#endif /* PICOCALC_QUEST_SOUND_H */

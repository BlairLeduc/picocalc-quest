/* framepool.h — the core 0 -> core 1 frame handoff (design.md §7.2).
 *
 * Three of upstream's 320x240 indexed framebuffers, each in one of four
 * states. Core 0 fills one in cabal_update_screen() and publishes it;
 * core 1 takes the newest published one, presents it, and releases it.
 * Core 1 never reads a buffer core 0 may still be writing.
 *
 * The palette travels with the frame: core 0 copies the pending RGB565
 * table into the buffer's record before publishing it, so core 1 never
 * sees a palette half-written, and a palette change over the same pixels
 * is still a new frame.
 *
 * This file is the state machine and nothing else. It holds no lock:
 * the RP2350 has no compare-and-swap, and masking interrupts is not a
 * lock between cores (hardware-notes.md §9.5), so the port calls every
 * transition under one SIO spinlock (video_shim.c). Keeping the lock out
 * is what lets the transitions be tested on a host (EL §2.4).
 *
 * From pico-oric's snappool (D6), with the pixels in PSRAM by pointer
 * and the palette added.
 */
#ifndef PICOCALC_QUEST_FRAMEPOOL_H
#define PICOCALC_QUEST_FRAMEPOOL_H

#include <stdint.h>

#define FRAMEPOOL_COUNT 3

typedef enum {
    FRAME_FREE,       /* owned by nobody; core 0 may claim it        */
    FRAME_FILLING,    /* core 0 is writing it; core 1 must not look  */
    FRAME_READY,      /* complete, awaiting collection               */
    FRAME_RENDERING,  /* core 1 owns it                              */
} frame_state_t;

typedef struct {
    uint8_t      *pixels[FRAMEPOOL_COUNT];        /* fixed PSRAM slots    */
    uint16_t      palette[FRAMEPOOL_COUNT][256];  /* RGB565, per frame    */
    frame_state_t state[FRAMEPOOL_COUNT];
    uint32_t      published;   /* frames published                   */
    uint32_t      dropped;     /* superseded before core 1 took them */
} framepool_t;

/* Every buffer free, the counters zero, the palettes black. */
void framepool_init(framepool_t *p, uint8_t *const pixels[FRAMEPOOL_COUNT]);

/* Core 0: take a free buffer to fill. Returns its index, or -1 if none
 * is free — which three buffers make impossible while each side holds
 * at most one, so -1 is a bug, not back-pressure. */
int  framepool_claim(framepool_t *p);

/* Core 0: filling -> ready. Any older ready buffer is superseded and
 * freed on the spot, unpresented, so there is always a free buffer for
 * the next claim and core 0 never waits for core 1. */
void framepool_publish(framepool_t *p, int i);

/* Core 1: take the ready buffer, if there is one; ready -> rendering.
 * Returns its index or -1. */
int  framepool_take(framepool_t *p);

/* Core 1: rendering -> free. */
void framepool_release(framepool_t *p, int i);

/* Upstream's 0xRRGGBB to the panel's RGB565: the top bits of each. */
uint16_t frame_rgb565(uint32_t rgb888);

/* n indexed pixels through a palette into RGB565, n a multiple of 4.
 * src is read a word at a time, so a row in uncached PSRAM costs one
 * transfer per four pixels, not per pixel (§7.3). In SRAM on the
 * device: core 1 runs it for every row presented. */
void frame_expand_row(const uint8_t *src, const uint16_t *pal, uint16_t *dst, unsigned n);

#endif /* PICOCALC_QUEST_FRAMEPOOL_H */

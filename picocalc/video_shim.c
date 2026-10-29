/* video_shim.c — upstream's graphics_* and framebuffer calls over the
 * frame pool (design.md §5.3, §7.2).
 *
 * Upstream's cabal_update_screen() asks for the back buffer, redraws all
 * 240 rows of it, and swaps. Here the back buffer is the pool buffer core
 * 0 is filling, and the swap publishes it, with the palette, to core 1.
 * rp2350-minimal.cpp needs no change.
 */

#include <stdio.h>
#include <string.h>

#include "hardware/sync.h"
#include "hardware/xip_cache.h"
#include "pico/stdlib.h"

#include "HDMI.h"
#include "board_config.h"
#include "framepool.h"
#include "psram_allocator.h"
#include "video.h"

#define PSRAM_CACHED   0x11000000u
#define PSRAM_UNCACHED 0x15000000u

static framepool_t s_pool;
static spin_lock_t *s_lock;
static uint16_t s_pending[256];
static int s_filling = -1;
static int s_front = -1;
/* Set by core 0 once the pool exists: core 1 starts before the PSRAM is
 * checked, and looks for frames from the start. */
static volatile bool s_ready;

volatile uint32_t g_video_publish_max_us;

void graphics_set_palette(uint8_t i, uint32_t color888) {
    s_pending[i] = frame_rgb565(color888);
}

void video_init(void) {
    /* Upstream's two slots, and a third after them in the 512 KB its
     * allocator reserves below the mspace and no longer uses
     * (psram_allocator.c), so the mspace is the size it was (§12.2). */
    size_t slot = psram_framebuffer_size();
    uint8_t *px[FRAMEPOOL_COUNT] = {
        psram_get_framebuffer(),
        psram_get_framebuffer_back(),
        (uint8_t *)psram_get_framebuffer_back() + slot,
    };
    for (unsigned i = 0; i < FRAMEPOOL_COUNT; i++) memset(px[i], 0, CABAL_FRAMEBUFFER_SIZE);
#if PICOCALC_QUEST_FB_READ == FB_READ_UNCACHED || PICOCALC_QUEST_FB_READ == FB_READ_DMA
    xip_cache_clean_range((uintptr_t)px[0] - XIP_BASE, FRAMEPOOL_COUNT * slot);
#endif
    s_lock = spin_lock_init(spin_lock_claim_unused(true));
    framepool_init(&s_pool, px);
    __dmb();
    s_ready = true;
}

/* Core 0. Claims a buffer when it holds none: cabal_init_graphics()
 * asks once, and every cabal_update_screen() asks again before drawing. */
uint8_t *cabal_get_back_buffer(void) {
    if (s_filling < 0) {
        uint32_t irq = spin_lock_blocking(s_lock);
        s_filling = framepool_claim(&s_pool);
        spin_unlock(s_lock, irq);
        if (s_filling < 0) {
            /* Three buffers and one each make this a bug (framepool.h). */
            printf("video: no free frame buffer\n");
            return NULL;
        }
    }
    return s_pool.pixels[s_filling];
}

/* The buffer last published; upstream's main.c used it, nothing else
 * does. */
uint8_t *cabal_get_framebuffer(void) {
    int i = s_front >= 0 ? s_front : s_filling;
    return i >= 0 ? s_pool.pixels[i] : NULL;
}

void cabal_swap_buffers(void) {
    int i = s_filling;
    if (i < 0) return;
    uint32_t t0 = time_us_32();
    memcpy(s_pool.palette[i], s_pending, sizeof(s_pending));
#if PICOCALC_QUEST_FB_READ == FB_READ_UNCACHED || PICOCALC_QUEST_FB_READ == FB_READ_DMA
    /* Core 1 reads past the cache, so core 0's writes must be in the
     * PSRAM first. */
    xip_cache_clean_range((uintptr_t)s_pool.pixels[i] - XIP_BASE, CABAL_FRAMEBUFFER_SIZE);
#endif
    uint32_t irq = spin_lock_blocking(s_lock);
    framepool_publish(&s_pool, i);
    spin_unlock(s_lock, irq);
    s_front = i;
    s_filling = -1;
    uint32_t us = time_us_32() - t0;
    if (us > g_video_publish_max_us) g_video_publish_max_us = us;
}

int video_take(void) {
    if (!s_ready) return -1;
    __dmb();
    uint32_t irq = spin_lock_blocking(s_lock);
    int i = framepool_take(&s_pool);
    spin_unlock(s_lock, irq);
    return i;
}

void video_release(int i) {
    uint32_t irq = spin_lock_blocking(s_lock);
    framepool_release(&s_pool, i);
    spin_unlock(s_lock, irq);
}

const uint8_t *video_pixels(int i) {
    const uint8_t *p = s_pool.pixels[i];
#if PICOCALC_QUEST_FB_READ == FB_READ_UNCACHED || PICOCALC_QUEST_FB_READ == FB_READ_DMA
    p = (const uint8_t *)((uintptr_t)p - PSRAM_CACHED + PSRAM_UNCACHED);
#endif
    return p;
}

const uint16_t *video_palette(int i) {
    return s_pool.palette[i];
}

void video_counts(uint32_t *published, uint32_t *dropped) {
    *published = *dropped = 0;
    if (!s_ready) return;
    __dmb();
    uint32_t irq = spin_lock_blocking(s_lock);
    *published = s_pool.published;
    *dropped = s_pool.dropped;
    spin_unlock(s_lock, irq);
}

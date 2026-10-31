/* framepool.c — the core 0 -> core 1 frame handoff (design.md §7.2). */

#include "framepool.h"

#include <string.h>

/* Hot code goes in SRAM on the device, out of the XIP cache core 0's
 * engine needs (§7.3); the SDK's __not_in_flash_func, without the SDK. */
#ifdef PICOCALC_QUEST_HOST
#define FRAME_IN_RAM(name) name
#else
#define FRAME_IN_RAM(name) __attribute__((section(".time_critical." #name))) name
#endif

void framepool_init(framepool_t *p, uint8_t *const pixels[FRAMEPOOL_COUNT]) {
    memset(p, 0, sizeof(*p));
    /* Every row changed in frame 1, which no buffer holds yet. */
    for (unsigned r = 0; r < FRAMEPOOL_ROWS; r++) p->row_seq[r] = 1;
    for (unsigned i = 0; i < FRAMEPOOL_COUNT; i++) {
        p->pixels[i] = pixels[i];
        p->state[i] = FRAME_FREE;
    }
}

static int find(const framepool_t *p, frame_state_t s) {
    for (unsigned i = 0; i < FRAMEPOOL_COUNT; i++)
        if (p->state[i] == s) return (int)i;
    return -1;
}

int framepool_claim(framepool_t *p) {
    int i = find(p, FRAME_FREE);
    if (i >= 0) p->state[i] = FRAME_FILLING;
    return i;
}

void framepool_publish(framepool_t *p, int i) {
    if (i < 0 || i >= FRAMEPOOL_COUNT || p->state[i] != FRAME_FILLING) return;

    /* At most one buffer is ever ready, so "the newest ready one" is
     * just "the ready one", and core 1 needs no sequence numbers to pick
     * between them. */
    for (unsigned j = 0; j < FRAMEPOOL_COUNT; j++) {
        if (p->state[j] == FRAME_READY) {
            p->state[j] = FRAME_FREE;
            p->dropped++;
        }
    }
    p->state[i] = FRAME_READY;
    p->held[i] = ++p->published;
}

int framepool_take(framepool_t *p) {
    int i = find(p, FRAME_READY);
    if (i >= 0) p->state[i] = FRAME_RENDERING;
    return i;
}

void framepool_release(framepool_t *p, int i) {
    if (i >= 0 && i < FRAMEPOOL_COUNT && p->state[i] == FRAME_RENDERING) p->state[i] = FRAME_FREE;
}

void framepool_mark(framepool_t *p, int y0, int n) {
    int y1 = y0 + n;
    if (y0 < 0) y0 = 0;
    if (y1 > FRAMEPOOL_ROWS) y1 = FRAMEPOOL_ROWS;
    uint32_t next = p->published + 1u;
    for (int r = y0; r < y1; r++) p->row_seq[r] = next;
}

void framepool_cursor(framepool_t *p, int y0, int n, int moved) {
    if (n < 0) n = 0;
    if (!moved && y0 == p->cursor_y && n == p->cursor_n) return;
    framepool_mark(p, p->cursor_y, p->cursor_n);
    framepool_mark(p, y0, n);
    p->cursor_y = y0;
    p->cursor_n = n;
}

int framepool_stale(const framepool_t *p, int i, unsigned row) {
    return row < FRAMEPOOL_ROWS && p->row_seq[row] > p->held[i];
}

int framepool_changed(const framepool_t *p, unsigned row, uint32_t since) {
    return row < FRAMEPOOL_ROWS && p->row_seq[row] > since;
}

uint16_t frame_rgb565(uint32_t rgb888) {
    uint32_t r = (rgb888 >> 16) & 0xffu;
    uint32_t g = (rgb888 >> 8) & 0xffu;
    uint32_t b = rgb888 & 0xffu;
    return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

void FRAME_IN_RAM(frame_expand_row)(const uint8_t *src, const uint16_t *pal, uint16_t *dst,
                                    unsigned n) {
    for (unsigned i = 0; i < n; i += 4u) {
        uint32_t w;
        memcpy(&w, src + i, sizeof(w));    /* one load; the bytes in memory order */
        dst[i]      = pal[w & 0xffu];
        dst[i + 1u] = pal[(w >> 8) & 0xffu];
        dst[i + 2u] = pal[(w >> 16) & 0xffu];
        dst[i + 3u] = pal[w >> 24];
    }
}

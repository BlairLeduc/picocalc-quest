/* video.h — the frame pool between upstream's screen updates on core 0
 * and the presenter on core 1 (design.md §7.2, §7.3).
 *
 * Core 0 reaches the pool through upstream's own calls,
 * cabal_get_back_buffer() and cabal_swap_buffers(), which
 * video_shim.c implements; core 1 through the calls below.
 */
#ifndef PICOCALC_QUEST_VIDEO_H
#define PICOCALC_QUEST_VIDEO_H

#include <stdint.h>

/* How core 1 reads a frame out of the PSRAM (§7.3), chosen at build
 * time with -DPICOCALC_QUEST_FB_READ=n. M2 measured all four and kept
 * the DMA, which cost core 0 least:
 *
 *   0  through the cached XIP window, as core 0 wrote it;
 *   1  through the uncached alias, 0x15000000, the buffer's lines
 *      cleaned out of the cache by core 0 at publish;
 *   2  by DMA from the uncached alias into an SRAM row, cleaned the
 *      same way;
 *   3  not at all: core 1 takes each frame and gives it back unread,
 *      the control build the others are measured against (EL §12).
 */
#define FB_READ_CACHED   0
#define FB_READ_UNCACHED 1
#define FB_READ_DMA      2
#define FB_READ_NONE     3
#ifndef PICOCALC_QUEST_FB_READ
#define PICOCALC_QUEST_FB_READ FB_READ_DMA
#endif

/* The row stamps (framepool.h, §16 M8), each half its own lever so it
 * can be measured against a control build (EL §12):
 *
 *   PICOCALC_QUEST_ROW_COPY     1: core 0 refills only a buffer's stale
 *                               rows; 0: every row of every frame.
 *   PICOCALC_QUEST_ROW_PRESENT  1: core 1 sends only the rows changed
 *                               since the panel's frame, or all of them
 *                               when the palette changed; 0: all.
 */
#ifndef PICOCALC_QUEST_ROW_COPY
#define PICOCALC_QUEST_ROW_COPY 1
#endif
#ifndef PICOCALC_QUEST_ROW_PRESENT
#define PICOCALC_QUEST_ROW_PRESENT 1
#endif

/* Core 0, once the PSRAM heap is up and before upstream's cabal_init():
 * the three fixed slots below the mspace, cleared, and the pool. */
void video_init(void);

/* Core 1: the newest published frame, or -1; then give it back. */
int  video_take(void);
void video_release(int i);

/* The frame's pixels, 320x240 indexed, at the address core 1 reads them
 * by under PICOCALC_QUEST_FB_READ, and the RGB565 palette published with
 * them. Valid between video_take and video_release. */
const uint8_t  *video_pixels(int i);
const uint16_t *video_palette(int i);

/* The frame buffer i holds (1 the first), and whether a row changed
 * after frame `since` (0: always). Core 1, between take and release. */
uint32_t video_frame(int i);
int      video_row_changed(unsigned row, uint32_t since);

/* Frames core 0 has published, and those superseded before core 1 took
 * them, since boot. */
void video_counts(uint32_t *published, uint32_t *dropped);

/* Core 0's cost of a publish (the cache clean, when the read strategy
 * needs one): the longest since boot, in microseconds. */
extern volatile uint32_t g_video_publish_max_us;

/* Rows core 0 has refilled, since boot. */
extern volatile uint32_t g_video_rows_filled;

#endif /* PICOCALC_QUEST_VIDEO_H */

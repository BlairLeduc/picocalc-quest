/* picocalc_frame.h — the rows each frame changes, for upstream's screen
 * updates (design.md §7.2, §16 M8).
 *
 * The patched OSystem marks the rows of the game's screen each call
 * writes, and the patched cabal_update_screen() marks the cursor's rows
 * and refills only the stale rows of the buffer it fills. Rows here are
 * the frame's, 0 to 239, letterbox included. C linkage, for the C++
 * callers.
 */
#ifndef PICOCALC_QUEST_PICOCALC_FRAME_H
#define PICOCALC_QUEST_PICOCALC_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif

/* Core 0: rows y0 to y0 + n - 1 change in the next frame. */
void picocalc_frame_mark(int y0, int n);

/* Core 0: the cursor's rows in the next frame, n 0 for none, and whether
 * its drawing differs from the last frame's (framepool_cursor()). */
void picocalc_frame_cursor(int y0, int n, int moved);

/* Core 0, between cabal_get_back_buffer() and cabal_swap_buffers():
 * whether the buffer being filled needs the row refilled. Always 1 in a
 * build with PICOCALC_QUEST_ROW_COPY=0. */
int picocalc_frame_stale(int row);

#ifdef __cplusplus
}
#endif

#endif /* PICOCALC_QUEST_PICOCALC_FRAME_H */

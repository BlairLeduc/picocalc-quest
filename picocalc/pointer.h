/* pointer.h — the arrows as a mouse (design.md §9.3).
 *
 * The PicoCalc has no mouse, so in pointer mode the arrows move one. The
 * motion is computed from which arrows are held and for how long, not
 * from key events: one pixel on the press, then a speed that rises from
 * v0 to v1 over ramp_us while held, integrated, so the distance is the
 * same however often the engine polls and the MCU's repeats play no
 * part. Each axis runs on its own: Right held, then Up added, starts Up
 * from rest. An arrow pressed with Ctrl moves exactly one pixel and
 * nothing while held, for fine placement. The buttons are held for as
 * long as their keys are.
 *
 * The output is deltas in the game's pixels, screen-down positive, at
 * most POINTER_MAX_DELTA a call (upstream's poll clamps to that); what is
 * left over comes on the next call. Upstream keeps the absolute place
 * and clamps it to the screen, so an engine's warp needs nothing here.
 *
 * Pure code, with no Pico SDK, so the host test can drive it (§15).
 * Core 0 owns it, in input_shim.c. Times are time_us_32()'s, and wrap.
 */
#ifndef PICOCALC_QUEST_POINTER_H
#define PICOCALC_QUEST_POINTER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { PTR_LEFT, PTR_RIGHT, PTR_UP, PTR_DOWN } ptr_arrow_t;

#define PTR_BUTTON_LEFT  0x01u   /* upstream's bits: left 1, right 2 */
#define PTR_BUTTON_RIGHT 0x02u

#define POINTER_MAX_DELTA 127

/* The curve (D8, tuned by play-testing in M6), in game pixels a second. */
#define POINTER_V0      40u
#define POINTER_V1      240u
#define POINTER_RAMP_US 600000u

typedef struct {
    int8_t   dir;        /* -1, 0 or +1: left/up negative                */
    uint32_t since_us;   /* when this run in dir began                   */
    uint32_t sent;       /* pixels of the run counted into owed so far   */
} ptr_axis_t;

typedef struct {
    uint32_t   v0, v1, ramp_us;
    bool       held[4];  /* by ptr_arrow_t: held for motion, not nudges  */
    ptr_axis_t axis[2];  /* x, y                                         */
    int32_t    owed[2];  /* pixels earned and not yet given out          */
    uint32_t   last_us;  /* the last advance                             */
    uint8_t    buttons;  /* PTR_BUTTON_*                                 */
} pointer_t;

/* The curve as above, and nothing held. */
void pointer_init(pointer_t *p);

/* Let go of everything: no arrow, no button, nothing owed. The curve
 * stays. */
void pointer_reset(pointer_t *p);

/* An arrow's press or release at `at_us`. A nudge (pressed with Ctrl)
 * is one pixel on the press; its release does nothing. */
void pointer_arrow(pointer_t *p, ptr_arrow_t a, bool down, bool nudge, uint32_t at_us);

void pointer_button(pointer_t *p, uint8_t button, bool down);

/* The motion up to `now_us` not given out yet, each axis at most
 * POINTER_MAX_DELTA either way. True if either is not zero. */
bool pointer_motion(pointer_t *p, uint32_t now_us, int16_t *dx, int16_t *dy);

/* The pixels a run has covered `t_us` after its press: one, then the
 * curve's integral. For the test, and for the travel time. */
uint32_t pointer_run_px(const pointer_t *p, uint32_t t_us);

#endif /* PICOCALC_QUEST_POINTER_H */

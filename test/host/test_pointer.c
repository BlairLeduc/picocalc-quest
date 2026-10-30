/* test_pointer.c — the arrows as a mouse (design.md §9.3, §15).
 *
 * The curve's shape; the same distance however often it is polled;
 * diagonals, opposite arrows and a release mid-run; Ctrl's one-pixel
 * nudges; the clamp to upstream's largest delta, with the rest kept;
 * events counted from before the last poll; the buttons; and time_us_32
 * wrapping under a held arrow.
 */

#include <stdio.h>
#include <stdlib.h>

#include "pointer.h"

static int failures;

#define CHECK(cond, ...)                                    \
    do {                                                    \
        if (!(cond)) {                                      \
            failures++;                                     \
            fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
            fprintf(stderr, __VA_ARGS__);                   \
            fputc('\n', stderr);                            \
        }                                                   \
    } while (0)

/* Poll every `step` microseconds from `from` to `to`, summing the motion. */
static void poll_sum(pointer_t *p, uint32_t from, uint32_t to, uint32_t step, long *sx, long *sy) {
    for (uint32_t t = from; (int32_t)(to - t) > 0;) {
        t = (int32_t)(to - t) > (int32_t)step ? t + step : to;
        int16_t dx, dy;
        pointer_motion(p, t, &dx, &dy);
        CHECK(abs(dx) <= POINTER_MAX_DELTA && abs(dy) <= POINTER_MAX_DELTA, "delta %d,%d", dx, dy);
        *sx += dx;
        *sy += dy;
    }
}

static void test_curve(void) {
    pointer_t p;
    pointer_init(&p);
    CHECK(pointer_run_px(&p, 0) == 1, "one pixel on the press");
    /* 40 px/s rising to 240 over 0.6 s covers 84 px; then 240 px/s. */
    CHECK(pointer_run_px(&p, POINTER_RAMP_US) == 85, "end of ramp %u", pointer_run_px(&p, POINTER_RAMP_US));
    CHECK(pointer_run_px(&p, POINTER_RAMP_US + 1000000u) == 325, "a second later %u",
          pointer_run_px(&p, POINTER_RAMP_US + 1000000u));
    uint32_t last = 0;
    for (uint32_t t = 0; t < 3000000u; t += 1000u) {
        uint32_t px = pointer_run_px(&p, t);
        CHECK(px >= last, "never back at %u us", t);
        last = px;
    }
    /* Across a 320-pixel screen in about 1.6 s. */
    uint32_t t = 0;
    while (pointer_run_px(&p, t) < 320u) t += 1000u;
    printf("320 px in %u ms\n", t / 1000u);
    CHECK(t > 1400000u && t < 1800000u, "320 px in %u us", t);
}

static void test_poll_rate(void) {
    /* Right held 1.5 s: the same pixels at 1, 17 and 250 ms a poll. */
    static const uint32_t steps[] = { 1000u, 17000u, 250000u };
    uint32_t t0 = 5000u, t1 = t0 + 1500000u;
    for (unsigned i = 0; i < 3; i++) {
        pointer_t p;
        pointer_init(&p);
        long sx = 0, sy = 0;
        pointer_arrow(&p, PTR_RIGHT, true, false, t0);
        poll_sum(&p, t0, t1, steps[i], &sx, &sy);
        pointer_arrow(&p, PTR_RIGHT, false, false, t1);
        poll_sum(&p, t1, t1 + 1000000u, steps[i], &sx, &sy);
        long want = (long)pointer_run_px(&p, 1500000u);
        CHECK(sx == want && sy == 0, "step %u: %ld,%ld, want %ld,0", steps[i], sx, sy, want);
    }
}

static void test_tap(void) {
    pointer_t p;
    pointer_init(&p);
    long sx = 0, sy = 0;
    pointer_arrow(&p, PTR_UP, true, false, 100u);
    pointer_arrow(&p, PTR_UP, false, false, 100u);
    poll_sum(&p, 100u, 200000u, 10000u, &sx, &sy);
    CHECK(sx == 0 && sy == -1, "a tap is one pixel: %ld,%ld", sx, sy);
}

static void test_axes(void) {
    pointer_t p;
    pointer_init(&p);
    long sx = 0, sy = 0;
    /* Right, then Down 0.3 s later: each axis its own run. */
    pointer_arrow(&p, PTR_RIGHT, true, false, 0);
    poll_sum(&p, 0, 300000u, 10000u, &sx, &sy);
    pointer_arrow(&p, PTR_DOWN, true, false, 300000u);
    poll_sum(&p, 300000u, 600000u, 10000u, &sx, &sy);
    CHECK(sx == (long)pointer_run_px(&p, 600000u), "x %ld", sx);
    CHECK(sy == (long)pointer_run_px(&p, 300000u), "y %ld", sy);

    /* Left added to Right: x stops. Right let go: Left runs from rest. */
    sx = sy = 0;
    pointer_arrow(&p, PTR_LEFT, true, false, 600000u);
    pointer_arrow(&p, PTR_DOWN, false, false, 600000u);
    poll_sum(&p, 600000u, 900000u, 10000u, &sx, &sy);
    CHECK(sx == 0 && sy == 0, "opposites cancel: %ld,%ld", sx, sy);
    pointer_arrow(&p, PTR_RIGHT, false, false, 900000u);
    poll_sum(&p, 900000u, 1000000u, 10000u, &sx, &sy);
    CHECK(sx == -(long)pointer_run_px(&p, 100000u), "left from rest: %ld", sx);
}

static void test_nudge(void) {
    pointer_t p;
    pointer_init(&p);
    long sx = 0, sy = 0;
    pointer_arrow(&p, PTR_LEFT, true, true, 0);
    poll_sum(&p, 0, 2000000u, 10000u, &sx, &sy);
    pointer_arrow(&p, PTR_LEFT, false, true, 2000000u);
    pointer_arrow(&p, PTR_DOWN, true, true, 2000000u);
    pointer_arrow(&p, PTR_DOWN, false, true, 2000000u);
    pointer_arrow(&p, PTR_DOWN, true, true, 2000000u);
    poll_sum(&p, 2000000u, 3000000u, 10000u, &sx, &sy);
    CHECK(sx == -1 && sy == 2, "nudges: %ld,%ld", sx, sy);

    /* A nudge while Right runs adds its pixel and leaves the run. */
    sx = sy = 0;
    pointer_arrow(&p, PTR_RIGHT, true, false, 3000000u);
    pointer_arrow(&p, PTR_RIGHT, true, true, 3100000u);
    poll_sum(&p, 3000000u, 3500000u, 10000u, &sx, &sy);
    CHECK(sx == (long)pointer_run_px(&p, 500000u) + 1, "run and nudge: %ld", sx);
}

static void test_clamp(void) {
    pointer_t p;
    pointer_init(&p);
    /* Two seconds of Down with no poll: given out 127 at a time. */
    pointer_arrow(&p, PTR_DOWN, true, false, 0);
    int16_t dx, dy;
    pointer_motion(&p, 2000000u, &dx, &dy);
    CHECK(dy == POINTER_MAX_DELTA, "clamped: %d", dy);
    pointer_arrow(&p, PTR_DOWN, false, false, 2000000u);
    long sy = dy;
    for (int i = 0; i < 10; i++) {
        pointer_motion(&p, 2000000u, &dx, &dy);
        sy += dy;
    }
    CHECK(sy == (long)pointer_run_px(&p, 2000000u), "the rest kept: %ld", sy);
}

static void test_late_event(void) {
    pointer_t p;
    pointer_init(&p);
    long sx = 0, sy = 0;
    /* Polled at 0.5 s; the release, queued at 0.4 s, popped after. */
    pointer_arrow(&p, PTR_RIGHT, true, false, 0);
    poll_sum(&p, 0, 500000u, 500000u, &sx, &sy);
    pointer_arrow(&p, PTR_RIGHT, false, false, 400000u);
    poll_sum(&p, 500000u, 900000u, 10000u, &sx, &sy);
    CHECK(sx == (long)pointer_run_px(&p, 500000u), "to the poll, no further: %ld", sx);

    /* A press queued before a poll that saw nothing moving runs from
     * its press. */
    sx = sy = 0;
    poll_sum(&p, 900000u, 1000000u, 100000u, &sx, &sy);
    pointer_arrow(&p, PTR_UP, true, false, 950000u);
    poll_sum(&p, 1000000u, 1200000u, 10000u, &sx, &sy);
    CHECK(sy == -(long)pointer_run_px(&p, 250000u), "from its press: %ld", sy);
}

static void test_wrap(void) {
    pointer_t p;
    pointer_init(&p);
    long sx = 0, sy = 0;
    uint32_t t0 = 0xFFFFFFFFu - 300000u;
    pointer_arrow(&p, PTR_LEFT, true, false, t0);
    poll_sum(&p, t0, t0 + 1000000u, 20000u, &sx, &sy);
    CHECK(sx == -(long)pointer_run_px(&p, 1000000u), "across the wrap: %ld", sx);
}

static void test_buttons(void) {
    pointer_t p;
    pointer_init(&p);
    pointer_button(&p, PTR_BUTTON_LEFT, true);
    pointer_button(&p, PTR_BUTTON_RIGHT, true);
    pointer_button(&p, PTR_BUTTON_LEFT, false);
    CHECK(p.buttons == PTR_BUTTON_RIGHT, "buttons %u", p.buttons);
    pointer_arrow(&p, PTR_UP, true, false, 0);
    p.v1 = 100u;
    pointer_reset(&p);
    int16_t dx, dy;
    CHECK(p.buttons == 0 && !pointer_motion(&p, 1000000u, &dx, &dy), "reset lets go");
    CHECK(p.v1 == 100u, "reset keeps the curve");
}

int main(void) {
    test_curve();
    test_poll_rate();
    test_tap();
    test_axes();
    test_nudge();
    test_clamp();
    test_late_event();
    test_wrap();
    test_buttons();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("test_pointer: ok\n");
    return 0;
}

/* pointer.c — the arrows as a mouse (design.md §9.3). */

#include "pointer.h"

#include <string.h>

void pointer_init(pointer_t *p) {
    memset(p, 0, sizeof *p);
    p->v0 = POINTER_V0;
    p->v1 = POINTER_V1;
    p->ramp_us = POINTER_RAMP_US;
}

void pointer_reset(pointer_t *p) {
    uint32_t v0 = p->v0, v1 = p->v1, ramp = p->ramp_us;
    memset(p, 0, sizeof *p);
    p->v0 = v0;
    p->v1 = v1;
    p->ramp_us = ramp;
}

uint32_t pointer_run_px(const pointer_t *p, uint32_t t_us) {
    /* In millionths of a pixel: speed in pixels a second times
     * microseconds. The speed rises linearly over the ramp, so its
     * integral there is v0 t + (v1 - v0) t^2 / 2T. */
    uint64_t t = t_us, ramp = p->ramp_us, rise = p->v1 > p->v0 ? p->v1 - p->v0 : 0;
    uint64_t d;
    if (t <= ramp) {
        d = p->v0 * t + (ramp ? rise * t * t / (2u * ramp) : 0);
    } else {
        d = p->v0 * ramp + rise * ramp / 2u + p->v1 * (t - ramp);
    }
    return 1u + (uint32_t)(d / 1000000u);
}

/* Count what each moving axis has covered up to `now` into owed. */
static void advance(pointer_t *p, uint32_t now) {
    for (unsigned i = 0; i < 2u; i++) {
        ptr_axis_t *a = &p->axis[i];
        if (a->dir == 0) continue;
        uint32_t want = pointer_run_px(p, now - a->since_us);
        p->owed[i] += a->dir * (int32_t)(want - a->sent);
        a->sent = want;
    }
    p->last_us = now;
}

/* A time before the last advance, while an axis moves, is counted from
 * the advance: that time has been given out already. */
static uint32_t not_before(const pointer_t *p, uint32_t t) {
    bool moving = p->axis[0].dir != 0 || p->axis[1].dir != 0;
    return moving && (int32_t)(t - p->last_us) < 0 ? p->last_us : t;
}

void pointer_arrow(pointer_t *p, ptr_arrow_t arrow, bool down, bool nudge, uint32_t at_us) {
    uint32_t now = not_before(p, at_us);
    advance(p, now);

    unsigned i = arrow == PTR_LEFT || arrow == PTR_RIGHT ? 0u : 1u;
    int8_t sign = arrow == PTR_RIGHT || arrow == PTR_DOWN ? 1 : -1;
    if (nudge) {
        if (down) p->owed[i] += sign;
        return;
    }
    p->held[arrow] = down;

    ptr_axis_t *a = &p->axis[i];
    int8_t dir = (int8_t)((i == 0 ? p->held[PTR_RIGHT] : p->held[PTR_DOWN]) -
                          (i == 0 ? p->held[PTR_LEFT] : p->held[PTR_UP]));
    if (dir == a->dir) return;
    /* A new run, from rest: its first pixel at once, from its press. */
    a->dir = dir;
    a->since_us = now;
    a->sent = 0;
    advance(p, now);
}

void pointer_button(pointer_t *p, uint8_t button, bool down) {
    if (down) p->buttons |= button;
    else p->buttons &= (uint8_t)~button;
}

static int16_t take(int32_t *owed) {
    int32_t d = *owed;
    if (d > POINTER_MAX_DELTA) d = POINTER_MAX_DELTA;
    if (d < -POINTER_MAX_DELTA) d = -POINTER_MAX_DELTA;
    *owed -= d;
    return (int16_t)d;
}

bool pointer_motion(pointer_t *p, uint32_t now_us, int16_t *dx, int16_t *dy) {
    advance(p, not_before(p, now_us));
    *dx = take(&p->owed[0]);
    *dy = take(&p->owed[1]);
    return *dx != 0 || *dy != 0;
}

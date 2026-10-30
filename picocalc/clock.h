/* clock.h — the date and time, from the clock the shell keeps (design.md
 * §13.2).
 *
 * The shell keeps UTC in the RP2350's POWMAN timer, with the local offset
 * in a checked record in POWMAN scratch 0-2 (picocalc_shell.h). This
 * reads it on every call, so it stays right however long a game runs.
 * With no clock set, the time is upstream's made-up one: 1 January 2025
 * plus the time since boot, so games that read the clock still see it
 * advance.
 *
 * The same time stamps what is written to the card, through FatFs's
 * get_fattime(); with no clock set, FatFs's own fixed date.
 *
 * The calendar arithmetic is pure, and has a host test (§15). C linkage,
 * for upstream's OSystem, which is C++.
 */
#ifndef PICOCALC_QUEST_CLOCK_H
#define PICOCALC_QUEST_CLOCK_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Broken-down local time, its fields as struct tm and ScummVM's TimeDate
 * have them. */
typedef struct {
    int tm_sec;    /* 0-59                 */
    int tm_min;    /* 0-59                 */
    int tm_hour;   /* 0-23                 */
    int tm_mday;   /* 1-31                 */
    int tm_mon;    /* 0-11                 */
    int tm_year;   /* years since 1900     */
    int tm_wday;   /* 0-6, Sunday is 0     */
} clock_tm_t;

/* The fallback's start: 2025-01-01 00:00:00, in seconds since 1970. */
#define CLOCK_FALLBACK_EPOCH 1735689600LL

/* Seconds since 1970 (negative before it) to calendar fields. */
void clock_civil(int64_t s, clock_tm_t *t);

/* The shell's UTC milliseconds and offset in minutes to local time. */
void clock_local(int64_t utc_ms, int offset_min, clock_tm_t *t);

/* Upstream's made-up time: the fallback's start plus the uptime. */
void clock_fallback(uint64_t uptime_ms, clock_tm_t *t);

/* Calendar fields to FatFs's packed time stamp: years since 1980 in bits
 * 31-25, month 1-12, day, hours, minutes, and seconds halved in 4-0.
 * Before 1980, which FAT cannot hold, is 1980-01-01 00:00:00; after
 * 2107, the end of 2107. */
uint32_t clock_fattime(const clock_tm_t *t);

#ifndef PICOCALC_QUEST_HOST
/* Local time now. Returns true when it is the shell's clock, false when
 * it is the fallback. */
bool picocalc_clock_now(clock_tm_t *t);
#endif

#ifdef __cplusplus
}
#endif

#endif /* PICOCALC_QUEST_CLOCK_H */

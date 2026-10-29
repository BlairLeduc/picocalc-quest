/* clock.c — the date and time, from the clock the shell keeps (design.md
 * §13.2). */

#include "clock.h"

#ifndef PICOCALC_QUEST_HOST
#include "pico/time.h"

#include "picocalc_shell.h"
#endif

/* Floor division, so times before 1970 land on the right day. */
static int64_t floor_div(int64_t a, int64_t b) {
    int64_t q = a / b;
    return (a % b != 0 && (a < 0) != (b < 0)) ? q - 1 : q;
}

void clock_civil(int64_t s, clock_tm_t *t) {
    int64_t days = floor_div(s, 86400);
    int64_t rem = s - days * 86400;
    t->tm_hour = (int)(rem / 3600);
    t->tm_min = (int)(rem / 60 % 60);
    t->tm_sec = (int)(rem % 60);
    /* 1970-01-01 was a Thursday. */
    t->tm_wday = (int)(((days + 4) % 7 + 7) % 7);

    /* Days to the Gregorian date: Howard Hinnant's civil_from_days, with
     * the year counted from March so that the leap day falls last. */
    int64_t z = days + 719468;
    int64_t era = floor_div(z, 146097);
    int64_t doe = z - era * 146097;                                  /* [0, 146096] */
    int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;   /* [0, 399] */
    int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);           /* [0, 365] */
    int64_t mp = (5 * doy + 2) / 153;                                /* [0, 11], March first */
    int64_t mday = doy - (153 * mp + 2) / 5 + 1;
    int64_t mon = mp < 10 ? mp + 2 : mp - 10;                        /* [0, 11], January first */
    int64_t year = yoe + era * 400 + (mon <= 1);
    t->tm_mday = (int)mday;
    t->tm_mon = (int)mon;
    t->tm_year = (int)(year - 1900);
}

void clock_local(int64_t utc_ms, int offset_min, clock_tm_t *t) {
    clock_civil(floor_div(utc_ms, 1000) + (int64_t)offset_min * 60, t);
}

void clock_fallback(uint64_t uptime_ms, clock_tm_t *t) {
    clock_civil(CLOCK_FALLBACK_EPOCH + (int64_t)(uptime_ms / 1000u), t);
}

#ifndef PICOCALC_QUEST_HOST
bool picocalc_clock_now(clock_tm_t *t) {
    int64_t utc_ms;
    int offset_min;
    if (picocalc_shell_time(&utc_ms, &offset_min)) {
        clock_local(utc_ms, offset_min, t);
        return true;
    }
    clock_fallback(to_ms_since_boot(get_absolute_time()), t);
    return false;
}
#endif

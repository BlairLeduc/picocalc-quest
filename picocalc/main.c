/* main.c — the PicoCalc's main(), in place of upstream's src/main.c
 * (design.md §5.3, §13.1).
 *
 * The boot order of §13.1: clocks (§11.1); the log and stdio (§14.2);
 * the banner; core 1, which brings up the southbridge and the LCD and
 * draws the test pattern; PSRAM, checked before the heap goes on it
 * (§11.3); then upstream's cabal_init() and cabal_main(). The card,
 * audio and the frame pool come in later milestones (§16).
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "board.h"
#include "board_config.h"
#include "clock.h"
#include "core1.h"
#include "log.h"
#include "picocalc_quest_version.h"
#include "picocalc_shell.h"
#include "psram_allocator.h"
#include "psram_check.h"
#include "psram_init.h"

void cabal_init(void);
int cabal_main(void);
void cabal_enable_psram_heap(void);
void cabal_report_previous_crash(void);

/* Upstream's two PSRAM framebuffer slots, as its main.c uses them. Its
 * swap also handed the new front buffer to HDMI (graphics_set_buffer);
 * nothing presents yet, so that hand-off is gone until M2's pool
 * publishes the buffer to core 1 (§7.2). */
static uint8_t *framebuffer[2];
static int front;

uint8_t *cabal_get_framebuffer(void) {
    return framebuffer[front];
}

uint8_t *cabal_get_back_buffer(void) {
    return framebuffer[1 - front];
}

void cabal_swap_buffers(void) {
    front = 1 - front;
}

/* The shell's clock, read on each side of the move to 300 MHz (§13.2):
 * the timer has its own tick source, and M1 checks the clock change
 * leaves it alone. */
typedef struct {
    bool     set;
    int64_t  utc_ms;
    int      offset_min;
    uint64_t uptime_us;
} shell_reading_t;

static shell_reading_t read_shell_clock(void) {
    shell_reading_t r = { 0 };
    r.uptime_us = time_us_64();
    r.set = picocalc_shell_time(&r.utc_ms, &r.offset_min);
    return r;
}

static void log_shell_clock(const shell_reading_t *before, const shell_reading_t *after) {
    if (!before->set || !after->set) {
        clock_tm_t t;
        clock_fallback(to_ms_since_boot(get_absolute_time()), &t);
        printf("  time         : the shell's clock is %s; the fallback, "
               "%04d-%02d-%02d %02d:%02d:%02d\n",
               before->set != after->set ? "set on one side of the clock change only" : "not set",
               t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
        return;
    }
    clock_tm_t t;
    clock_local(after->utc_ms, after->offset_min, &t);
    int off = after->offset_min < 0 ? -after->offset_min : after->offset_min;
    printf("  time         : the shell's, %04d-%02d-%02d %02d:%02d:%02d UTC%c%d:%02d\n",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec,
           after->offset_min < 0 ? '-' : '+', off / 60, off % 60);
    printf("  time         : across the clock change it moved %lld ms in %llu us of uptime\n",
           (long long)(after->utc_ms - before->utc_ms),
           (unsigned long long)(after->uptime_us - before->uptime_us));
}

static void log_psram(bool ok, const psram_check_t *r) {
    if (ok) {
        printf("  psram        : 8 MB at 0x11000000, CS GP%u; %lu words round trip in %lu us\n",
               get_psram_pin(), (unsigned long)r->words, (unsigned long)r->us);
    } else {
        printf("  psram        : FAILED its check: %lu of %lu words bad, the first at +0x%06lx "
               "(read 0x%08lx, wrote 0x%08lx); not booting into it\n",
               (unsigned long)r->bad, (unsigned long)r->words, (unsigned long)r->first_bad,
               (unsigned long)r->got, (unsigned long)r->want);
    }
}

int main(void) {
    shell_reading_t before = read_shell_clock();
    bool clocks_ok = board_init_clocks();
    shell_reading_t after = read_shell_clock();

    /* The UART's divider is worked out from the clock that stays. */
    log_init();
    printf("\npicocalc-quest %s (FRANK Quest %s)\n", PICOCALC_QUEST_VERSION, CABAL_VERSION);
    if (!clocks_ok) {
        printf("  WARNING: clk_sys did not reach %u MHz; the SPI rates will not be the "
               "ones this build assumes\n", BOARD_MHZ);
    }
    log_shell_clock(&before, &after);

    /* Upstream's PSRAM init, at the clock that stays (§11.3). Before
     * core 1: it takes the QMI out of its memory-mapped mode for a
     * moment, and core 1 runs from flash. */
    psram_init(get_psram_pin());
    board_info_t info;
    board_identify(&info);
    board_log_banner(&info);

    /* Core 1 brings up the southbridge and the panel; from here on, core
     * 1 drains the log. */
    log_start_ring();
    multicore_launch_core1(core1_main);
    while (!g_core1.ready) tight_loop_contents();
    __dmb();
    printf("  southbridge  : i2c %lu Hz, version %d, a register read %lu us\n",
           (unsigned long)g_core1.i2c_hz, g_core1.sb_version, (unsigned long)g_core1.i2c_us);
    printf("  lcd          : spi %lu Hz, init %lu us, test pattern %lu us\n",
           (unsigned long)g_core1.spi_hz, (unsigned long)g_core1.lcd_us,
           (unsigned long)g_core1.pattern_us);

    /* The check before anything is stored in the PSRAM. */
    psram_check_t check;
    bool psram_ok = psram_check(&check);
    log_psram(psram_ok, &check);
    cabal_report_previous_crash();

    if (!psram_ok) {
        /* Upstream's whole heap is in the PSRAM: booting into a bad one
         * would fail somewhere far from the cause. Core 1 shows it and
         * keeps the heartbeat going. */
        g_psram_failed = true;
        for (;;) tight_loop_contents();
    }

    psram_set_sram_mode(0);
    cabal_enable_psram_heap();

    framebuffer[0] = psram_get_framebuffer();
    framebuffer[1] = psram_get_framebuffer_back();
    memset(framebuffer[0], 0, CABAL_FRAMEBUFFER_SIZE);
    memset(framebuffer[1], 0, CABAL_FRAMEBUFFER_SIZE);

    cabal_init();
    return cabal_main();
}

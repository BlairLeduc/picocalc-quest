/* main.c — the PicoCalc's main(), in place of upstream's src/main.c
 * (design.md §5.3, §13.1).
 *
 * The boot order of §13.1: clocks (§11.1); the log and stdio (§14.2);
 * the banner; core 1, which brings up the southbridge and the LCD and
 * draws the test pattern; PSRAM, checked before the heap goes on it
 * (§11.3); the frame pool, which core 1 presents from (§7.2); the card,
 * mounted and checked for /quest (§10); then upstream's cabal_init() and
 * cabal_main(). A failure on the way shows a page naming it (page.h).
 * Audio comes in M5 (§16).
 */

#include <stdint.h>
#include <stdio.h>

#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "board.h"
#include "board_config.h"
#include "clock.h"
#include "core1.h"
#include "frank_quest_fs.h"
#include "log.h"
#include "page.h"
#include "picocalc_quest_version.h"
#include "picocalc_shell.h"
#include "psram_allocator.h"
#include "psram_check.h"
#include "psram_init.h"
#include "video.h"

void cabal_init(void);
int cabal_main(void);
void cabal_enable_psram_heap(void);
void cabal_report_previous_crash(void);

/* Core 0's time in upstream's delay, for the heartbeat's busy share
 * (§14.3): the link wraps cabal_delay, so upstream's OSystem calls this
 * and the file stays unpatched. */
void __real_cabal_delay(uint32_t ms);
void __wrap_cabal_delay(uint32_t ms) {
    uint32_t t0 = time_us_32();
    __real_cabal_delay(ms);
    g_core0_delay_us += time_us_32() - t0;
}

/* Upstream's scan of /quest, timed for M3 (§16). The link wraps it by its C++ name, so the selector stays
 * unpatched; QuestGame is opaque here. */
int __real__Z22frank_quest_scan_gamesP9QuestGamei(void *out, int max_out);
int __wrap__Z22frank_quest_scan_gamesP9QuestGamei(void *out, int max_out) {
    uint32_t t0 = time_us_32();
    int n = __real__Z22frank_quest_scan_gamesP9QuestGamei(out, max_out);
    printf("picocalc-quest: /quest scanned in %lu ms, %d game(s)\n",
           (unsigned long)((time_us_32() - t0) / 1000u), n);
    return n;
}

/* The card's state must hold this long to count: the slot is
 * push-to-eject, and its switch bounces through an insertion
 * (hardware-notes.md §7.1). */
#define CARD_SETTLE_US 500000u

void picocalc_restart(void);

/* Wait until the card has been `in` (or out) for CARD_SETTLE_US. */
static void wait_for_card(bool in) {
    uint32_t since = time_us_32();
    for (;;) {
        if (picocalc_shell_card_present() != in) since = time_us_32();
        else if (time_us_32() - since >= CARD_SETTLE_US) return;
        busy_wait_us_32(10000);
    }
}

/* Show the page, and start again once a card is in that was not before:
 * after an insertion for an empty slot, after the card comes out and
 * goes back in otherwise. The restart is the program's own, which both
 * loaders start again (picocalc_shell.h). */
static void card_failed(page_t p) {
    page_show(p);
    if (p != PAGE_NO_CARD) wait_for_card(false);
    wait_for_card(true);
    printf("picocalc-quest: a card went in; starting again\n");
    picocalc_restart();
}

void picocalc_restart(void) {
    busy_wait_us_32(100000);          /* the log's last line out */
    /* Never reset with a southbridge transaction in flight: it can leave
     * the MCU dead until a power cycle (southbridge.h). */
    core1_park();
    watchdog_reboot(0, 0, 0);
    for (;;) tight_loop_contents();
}

/* The card, before upstream's cabal_init() mounts it (§10.1, §13.1):
 * mounted here, so the time is measured and a failure has its page;
 * upstream's own call then finds it mounted. */
static void check_card(void) {
    if (!picocalc_shell_card_present()) {
        printf("  card         : the slot is empty\n");
        card_failed(PAGE_NO_CARD);
    }
    uint32_t t0 = time_us_32();
    CabalFsResult r = cabal_fs_init();
    uint32_t ms = (time_us_32() - t0) / 1000u;
    if (r != CABAL_FS_OK) {
        printf("  card         : did not mount (%d), after %lu ms\n", (int)r, (unsigned long)ms);
        card_failed(PAGE_CARD_FAILED);
    }
    printf("  card         : mounted in %lu ms\n", (unsigned long)ms);
    if (!cabal_path_exists("/quest")) {
        printf("  card         : no /quest\n");
        card_failed(PAGE_NO_QUEST);
    }
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
    core1_paint_stack();
    multicore_launch_core1(core1_main);
    while (!g_core1.ready) tight_loop_contents();
    __dmb();
    printf("  southbridge  : i2c %lu Hz, version %d, a register read %lu us%s\n",
           (unsigned long)g_core1.i2c_hz, g_core1.sb_version, (unsigned long)g_core1.i2c_us,
           g_core1.sb_version < 0 ? "; NOT ANSWERING: switch the PicoCalc off and on" : "");
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
        page_show(PAGE_NO_PSRAM);
        for (;;) tight_loop_contents();
    }

    psram_set_sram_mode(0);
    cabal_enable_psram_heap();

    video_init();
    check_card();

    cabal_init();
    return cabal_main();
}

/* core1.c — core 1's loop: the panel, the southbridge and the log
 * (design.md §6.1, §14.3). */

#include "core1.h"

#include <stdio.h>

#include "hardware/sync.h"
#include "pico/stdlib.h"

#include "board.h"
#include "lcd.h"
#include "log.h"
#include "southbridge.h"

/* The battery is refreshed by the MCU every 20 s (hardware-notes.md §6),
 * but a transaction a second also keeps its bus watchdog fed (§6.1)
 * until M3 polls the keyboard. */
#define BAT_POLL_US   1000000u
#define HEARTBEAT_US  1000000u

core1_bringup_t g_core1 = { .sb_version = -1 };
volatile bool g_psram_failed;

#define RGB565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

/* Where the picture goes (§7.1): upstream's 320x240 framebuffer at y 40. */
#define FRAME_Y 40u
#define FRAME_H 240u

/* The corner-coded test pattern (EL §13.3, hardware-notes.md §10):
 * orientation, colour order and all four edges, by eye.
 *
 *   - red square top left, green top right, blue bottom left, white
 *     bottom right, 32 pixels each;
 *   - a one-pixel white border round the whole panel;
 *   - in the picture's 240 rows: eight colour bars, white, yellow, cyan,
 *     green, magenta, red, blue, black, 40 pixels each, over a grey ramp
 *     of 32 steps in the last 40 rows, black on the left.
 */
static void test_pattern(void) {
    static const uint16_t bars[8] = {
        RGB565(255, 255, 255), RGB565(255, 255, 0), RGB565(0, 255, 255), RGB565(0, 255, 0),
        RGB565(255, 0, 255),   RGB565(255, 0, 0),   RGB565(0, 0, 255),   RGB565(0, 0, 0),
    };
    const uint16_t white = RGB565(255, 255, 255);
    const unsigned w = LCD_PANEL_W, h = LCD_PANEL_H, sq = 32u;

    for (unsigned i = 0; i < 8u; i++) lcd_fill(i * 40u, FRAME_Y, 40u, FRAME_H - 40u, bars[i]);
    for (unsigned i = 0; i < 32u; i++) {
        unsigned v = i * 255u / 31u;
        lcd_fill(i * 10u, FRAME_Y + FRAME_H - 40u, 10u, 40u, RGB565(v, v, v));
    }

    lcd_fill(0, 0, w, 1, white);
    lcd_fill(0, h - 1u, w, 1, white);
    lcd_fill(0, 0, 1, h, white);
    lcd_fill(w - 1u, 0, 1, h, white);

    lcd_fill(0, 0, sq, sq, RGB565(255, 0, 0));
    lcd_fill(w - sq, 0, sq, sq, RGB565(0, 255, 0));
    lcd_fill(0, h - sq, sq, sq, RGB565(0, 0, 255));
    lcd_fill(w - sq, h - sq, sq, sq, white);
}

void core1_main(void) {
    /* 1. The southbridge first: a dead bus is the first thing to know
     *    about (hardware-notes.md §10). */
    g_core1.i2c_hz = sb_init();
    uint8_t r[2];
    uint32_t t0 = time_us_32();
    if (sb_read(SB_REG_VER, r) == SB_OK) g_core1.sb_version = r[1];
    g_core1.i2c_us = time_us_32() - t0;

    /* 2. The LCD, cleared before DISPON, then the pattern. */
    t0 = time_us_32();
    g_core1.spi_hz = lcd_init();
    g_core1.lcd_us = time_us_32() - t0;
    t0 = time_us_32();
    test_pattern();
    g_core1.pattern_us = time_us_32() - t0;

    board_temp_init();

    __dmb();
    g_core1.ready = true;

    bool painted_fault = false;
    int battery = -1, temp_c = board_temp_c();
    uint32_t now = time_us_32();
    uint32_t next_bat = now, next_beat = now + HEARTBEAT_US;
    uint32_t seconds = 0, loops = 0;
    for (;;) {
        log_pump();
        loops++;

        if (g_psram_failed && !painted_fault) {
            lcd_fill(0, FRAME_Y, LCD_PANEL_W, FRAME_H, RGB565(255, 0, 0));
            painted_fault = true;
        }

        now = time_us_32();
        if ((int32_t)(now - next_bat) >= 0) {
            next_bat = now + BAT_POLL_US;
            battery = sb_read(SB_REG_BAT, r) == SB_OK ? r[1] : -1;
            temp_c = board_temp_c();
        }

        /* The heartbeat's skeleton (§14.3): core 0's counters join it as
         * the milestones bring them. */
        now = time_us_32();
        if ((int32_t)(now - next_beat) >= 0) {
            next_beat += HEARTBEAT_US;
            seconds++;
            printf("hb %lu: temp %d C, battery %d%%%s, i2c errors %lu, log dropped %lu, "
                   "core 1 loops %lu\n",
                   (unsigned long)seconds, temp_c, battery < 0 ? -1 : battery & 0x7f,
                   battery >= 0 && (battery & 0x80) ? " charging" : "",
                   (unsigned long)sb_error_count(), (unsigned long)log_dropped(),
                   (unsigned long)loops);
            loops = 0;
        }

        /* Never sleep_us here (hardware-notes.md §9.7). */
        busy_wait_us_32(20);
    }
}

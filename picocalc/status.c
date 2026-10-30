/* status.c — the status band below the picture (design.md §7.4). */

#include "status.h"

#include "hardware/sync.h"

#include "font8x10.h"
#include "lcd.h"
#include "page.h"

#define RGB565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

/* The band: rows 280 to 319, under the picture at 40 (§7.1). */
#define BAND_Y 280u
#define BAND_H 40u

static const char *volatile s_title, *volatile s_help;
static volatile uint32_t s_seq;

void status_show(const char *title, const char *help) {
    s_title = title;
    s_help = help;
    __dmb();
    s_seq++;
}

void status_service(uint32_t now_us, bool covered) {
    static uint32_t seen, until;
    static bool shown;

    uint32_t seq = s_seq;
    if (covered) {
        /* The page has drawn over the band; whatever was asked for in
         * the meantime is done with. */
        seen = seq;
        shown = false;
        return;
    }
    if (seq != seen) {
        seen = seq;
        __dmb();
        const char *title = s_title, *help = s_help;
        const uint16_t bg = 0;
        lcd_fill(0, BAND_Y, LCD_PANEL_W, BAND_H, bg);
        page_line(BAND_Y + 5u, title ? title : "", RGB565(255, 200, 0), bg);
        page_line(BAND_Y + 5u + FONT_H + 4u, help ? help : "", RGB565(160, 160, 160), bg);
        until = now_us + STATUS_SHOW_US;
        shown = true;
    } else if (shown && (int32_t)(now_us - until) >= 0) {
        lcd_fill(0, BAND_Y, LCD_PANEL_W, BAND_H, 0);
        shown = false;
    }
}

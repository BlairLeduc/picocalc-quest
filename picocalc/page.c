/* page.c — the pages shown in place of the selector (design.md §13.1). */

#include "page.h"

#include <string.h>

#include "font8x10.h"
#include "lcd.h"

#define RGB565(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

#define COLS (LCD_PANEL_W / FONT_W)    /* 40 */
#define TOP  100u                      /* the first line's y */
#define MESSAGE_LINES 8u

static volatile page_t s_wanted;
static const char *volatile s_message;
static uint16_t s_row[2][LCD_PANEL_W];

typedef struct {
    const char *title;
    const char *lines[6];
} page_text_t;

/* Each line at most COLS characters. */
static const page_text_t s_pages[] = {
    [PAGE_NO_PSRAM] = { "The PSRAM failed its check.", {
        "This needs a Pimoroni Pico Plus 2 or",
        "Plus 2 W, which have 8 MB of PSRAM.",
        "",
        "Turn the PicoCalc off and on again to",
        "try once more.",
    } },
    [PAGE_NO_CARD] = { "No SD card.", {
        "Put in a card with games in /quest.",
        "The PicoCalc starts again when it",
        "goes in.",
    } },
    [PAGE_CARD_FAILED] = { "The SD card did not mount.", {
        "It needs to be FAT or exFAT.",
        "",
        "Take the card out and put it back to",
        "try again.",
    } },
    [PAGE_NO_QUEST] = { "No /quest folder on the card.", {
        "Put each game in a folder of its own",
        "under /quest, named as the README",
        "says.",
        "",
        "Take the card out and put it back to",
        "try again.",
    } },
    /* The message goes between the title and these lines. */
    [PAGE_GAME_ERROR] = { "The game stopped with an error.", {
        "",
        "Press any key to go back to the list.",
    } },
};

void page_show(page_t p) {
    s_wanted = p;
}

void page_show_message(page_t p, const char *message) {
    s_message = message;
    s_wanted = p;
}

page_t page_wanted(void) {
    return s_wanted;
}

void page_line(unsigned y, const char *s, uint16_t fg, uint16_t bg) {
    unsigned n = (unsigned)strlen(s);
    if (n > COLS) n = COLS;
    unsigned x0 = (LCD_PANEL_W - n * FONT_W) / 2u;

    lcd_blit_begin(0, y, LCD_PANEL_W, FONT_H);
    for (unsigned r = 0; r < FONT_H; r++) {
        /* Two buffers, so this row is drawn while the last is on the
         * wire (lcd.h). */
        uint16_t *row = s_row[r & 1u];
        for (unsigned x = 0; x < LCD_PANEL_W; x++) row[x] = bg;
        for (unsigned i = 0; i < n; i++) {
            uint8_t c = (uint8_t)s[i];
            uint8_t bits = c < FONT_GLYPHS ? font_8x10[c * FONT_H + r] : 0;
            for (unsigned b = 0; b < FONT_W; b++) {
                if (bits & (0x80u >> b)) row[x0 + i * FONT_W + b] = fg;
            }
        }
        lcd_blit_row(row, LCD_PANEL_W);
    }
    lcd_blit_end();
}

void page_draw(page_t p) {
    const uint16_t bg = RGB565(0, 0, 0);
    lcd_fill(0, 0, LCD_PANEL_W, LCD_PANEL_H, bg);
    if (p == PAGE_NONE || p >= sizeof s_pages / sizeof s_pages[0]) return;

    const page_text_t *t = &s_pages[p];
    unsigned y = TOP;
    page_line(y, "picocalc-quest", RGB565(128, 128, 128), bg);
    y += 2u * FONT_H;
    page_line(y, t->title, RGB565(255, 200, 0), bg);
    y += 2u * FONT_H;
    /* The message, broken at spaces into lines of at most COLS, at
     * most MESSAGE_LINES of them. */
    const char *m = p == PAGE_GAME_ERROR ? s_message : NULL;
    if (m != NULL && *m == '\0') m = "It gave no reason.";
    for (unsigned n = 0; m != NULL && *m != '\0' && n < MESSAGE_LINES; n++) {
        while (*m == ' ' || *m == '\n') m++;
        size_t len = strcspn(m, "\n");
        if (len > COLS) {
            len = COLS;
            while (len > 0 && m[len] != ' ') len--;
            if (len == 0) len = COLS;
        }
        char line[COLS + 1];
        memcpy(line, m, len);
        line[len] = '\0';
        page_line(y, line, RGB565(255, 255, 255), bg);
        y += FONT_H + 2u;
        m += len;
    }
    for (unsigned i = 0; i < sizeof t->lines / sizeof t->lines[0] && t->lines[i]; i++) {
        page_line(y, t->lines[i], RGB565(255, 255, 255), bg);
        y += FONT_H + 2u;
    }
}

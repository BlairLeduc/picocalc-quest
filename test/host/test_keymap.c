/* test_keymap.c — the southbridge's key events to the engine's (design.md
 * §9.2, §15).
 *
 * The set of codes the MCU can send is written out here again from its
 * keymap (Code/picocalc_keyboard/keyboard.ino at clockworkpi/PicoCalc
 * f915198), apart from keymap.c's tables, and every code is checked: the
 * MCU's map to exactly one physical key and give a down and a matching
 * up; every other code is refused. Then the retranslated releases, Insert
 * under Alt, stray releases, repeats, the ASCII the engines are given,
 * and the bound chords against the ones the MCU swallows (HW §6.3).
 */

#include <stdio.h>
#include <string.h>

#include "keymap.h"

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

/* Each key of the MCU's kbd_entries and btn_entries: the code it sends,
 * and the code it sends with Shift (0 if Shift gives the same, or for a
 * letter, its capital). Modifiers are listed apart. */
static const struct { uint8_t chr, symb; } s_keys[] = {
    { 0x85, 0x90 }, { 0x84, 0x89 }, { 0x83, 0x88 }, { 0x82, 0x87 }, { 0x81, 0x86 },
    { '`', '~' },   { '3', '#' },   { '2', '@' },
    { 0x08, 0 },    { 0xD4, 0xD5 }, { 0xC1, 0 },    { 0x09, 0xD2 }, { 0xB1, 0xD0 },
    { '4', '$' },   { 'e', 'E' },   { 'w', 'W' },
    { 'p', 'P' },   { '=', '+' },   { '-', '_' },   { '\\', '|' },  { '/', '?' },
    { 'r', 'R' },   { 's', 'S' },   { '1', '!' },
    { 0x0A, 0xD1 }, { '8', '*' },   { '7', '&' },   { '6', '^' },   { '5', '%' },
    { 'f', 'F' },   { 'x', 'X' },   { 'q', 'Q' },
    { '.', '>' },   { 'i', 'I' },   { 'u', 'U' },   { 'y', 'Y' },   { 't', 'T' },
    { 'v', 'V' },   { ';', ':' },   { 'a', 'A' },
    { 'l', 'L' },   { 'k', 'K' },   { 'j', 'J' },   { 'h', 'H' },   { 'g', 'G' },
    { 'c', 'C' },   { '\'', '"' },  { 'z', 'Z' },
    { 'o', 'O' },   { ',', '<' },   { 'm', 'M' },   { 'n', 'N' },   { 'b', 'B' },
    { 'd', 'D' },   { ' ', 0 },
    { '0', ')' },   { '9', '(' },   { ']', '}' },   { '[', '{' },
    { 0xB7, 0 },    { 0xB5, 0xD6 }, { 0xB6, 0xD7 }, { 0xB4, 0 },
};
static const uint8_t s_modifiers[] = { 0xA1, 0xA2, 0xA3, 0xA5 };

static keymap_t g_km;

static km_result_t feed(uint8_t state, uint8_t code, km_event_t *ev) {
    km_event_t scratch;
    return keymap_feed(&g_km, state, code, ev ? ev : &scratch);
}

static bool sendable(uint8_t code) {
    for (unsigned i = 0; i < sizeof s_keys / sizeof s_keys[0]; i++) {
        if (s_keys[i].chr == code || (s_keys[i].symb && s_keys[i].symb == code)) return true;
    }
    for (unsigned i = 0; i < sizeof s_modifiers; i++) {
        if (s_modifiers[i] == code) return true;
    }
    return code == MCU_POWER || code == MCU_SYM;
}

static void test_every_code(void) {
    for (unsigned c = 0; c < 256; c++) {
        uint8_t code = (uint8_t)c;
        keymap_reset(&g_km);
        km_event_t down, up;
        km_result_t r = feed(MCU_PRESSED, code, &down);
        if (!sendable(code)) {
            CHECK(r == KM_UNKNOWN, "code 0x%02x is not the MCU's, but was accepted", c);
            CHECK(keymap_physical(code, false) == 0 && keymap_physical(code, true) == 0,
                  "code 0x%02x is not the MCU's, but has a physical key", c);
            continue;
        }
        CHECK(keymap_physical(code, false) != 0, "code 0x%02x has no physical key", c);
        if (code == MCU_CAPS || code == MCU_POWER || code == MCU_SYM) {
            CHECK(r == KM_CONSUMED, "code 0x%02x should be accepted and not passed on", c);
            continue;
        }
        CHECK(r == KM_EVENT && down.down && !down.repeat && down.keycode != 0,
              "code 0x%02x: a press gives a down", c);
        r = feed(MCU_RELEASED, code, &up);
        CHECK(r == KM_EVENT && !up.down && up.keycode == down.keycode && up.ascii == down.ascii,
              "code 0x%02x: its release gives the matching up", c);
        CHECK(feed(MCU_RELEASED, code, NULL) == KM_STRAY, "code 0x%02x: a second release is stray", c);
    }

    /* A key's codes, plain and shifted, are one physical key, and no two
     * keys share one. */
    unsigned n = sizeof s_keys / sizeof s_keys[0];
    for (unsigned i = 0; i < n; i++) {
        uint8_t p = keymap_physical(s_keys[i].chr, false);
        CHECK(p == s_keys[i].chr, "0x%02x names its own key, not 0x%02x", s_keys[i].chr, p);
        if (s_keys[i].symb) {
            CHECK(keymap_physical(s_keys[i].symb, false) == p,
                  "0x%02x is Shift on 0x%02x", s_keys[i].symb, s_keys[i].chr);
        }
        for (unsigned j = i + 1; j < n; j++) {
            CHECK(keymap_physical(s_keys[j].chr, false) != p,
                  "0x%02x and 0x%02x are one key", s_keys[i].chr, s_keys[j].chr);
        }
    }
    CHECK(feed(0, 'a', NULL) == KM_UNKNOWN && feed(4, 'a', NULL) == KM_UNKNOWN,
          "states other than 1-3 are refused");
}

static void expect(km_result_t r, const km_event_t *e, bool down, bool repeat,
                   int keycode, int ascii, int flags, const char *what) {
    CHECK(r == KM_EVENT, "%s: no event (%d)", what, r);
    if (r != KM_EVENT) return;
    CHECK(e->down == down && e->repeat == repeat, "%s: down %d repeat %d", what, e->down,
          e->repeat);
    CHECK(e->keycode == keycode, "%s: keycode %d, want %d", what, e->keycode, keycode);
    CHECK(e->ascii == ascii, "%s: ascii %d, want %d", what, e->ascii, ascii);
    CHECK(e->flags == flags, "%s: flags %d, want %d", what, e->flags, flags);
}

static void test_retranslated_releases(void) {
    km_event_t e;
    keymap_reset(&g_km);

    /* Shift let go first: press `A`, release `a`. */
    expect(feed(MCU_PRESSED, MCU_SHIFT_L, &e), &e, true, false, KM_KEY_LSHIFT, 0, KM_MOD_SHIFT, "shift");
    expect(feed(MCU_PRESSED, 'A', &e), &e, true, false, 'a', 'A', KM_MOD_SHIFT, "shift A");
    expect(feed(MCU_RELEASED, MCU_SHIFT_L, &e), &e, false, false, KM_KEY_LSHIFT, 0, KM_MOD_SHIFT, "shift up");
    expect(feed(MCU_RELEASED, 'a', &e), &e, false, false, 'a', 'A', KM_MOD_SHIFT, "a up is A's up");

    /* Press `!`, release `1`. */
    feed(MCU_PRESSED, MCU_SHIFT_R, NULL);
    expect(feed(MCU_PRESSED, '!', &e), &e, true, false, '!', '!', KM_MOD_SHIFT, "!");
    feed(MCU_RELEASED, MCU_SHIFT_R, NULL);
    expect(feed(MCU_RELEASED, '1', &e), &e, false, false, '!', '!', KM_MOD_SHIFT, "1 up is !'s up");

    /* The keys that exist only as alternates: Home is Shift+Tab, and so on. */
    static const struct { uint8_t press, release; int keycode; } alt[] = {
        { MCU_HOME, MCU_TAB, KM_KEY_HOME },       { MCU_END, MCU_DEL, KM_KEY_END },
        { MCU_PGUP, MCU_UP, KM_KEY_PAGEUP },      { MCU_PGDN, MCU_DOWN, KM_KEY_PAGEDOWN },
        { MCU_BREAK, MCU_ESC, KM_KEY_PAUSE },     { MCU_F1 + 5, MCU_F1, KM_KEY_F1 + 5 },
        { MCU_F10, MCU_F1 + 4, KM_KEY_F1 + 9 },   { MCU_INSERT, MCU_ENTER, KM_KEY_INSERT },
    };
    for (unsigned i = 0; i < sizeof alt / sizeof alt[0]; i++) {
        keymap_reset(&g_km);
        feed(MCU_PRESSED, MCU_SHIFT_L, NULL);
        CHECK(feed(MCU_PRESSED, alt[i].press, &e) == KM_EVENT && e.keycode == alt[i].keycode,
              "0x%02x: down %d", alt[i].press, e.keycode);
        feed(MCU_RELEASED, MCU_SHIFT_L, NULL);
        CHECK(feed(MCU_RELEASED, alt[i].release, &e) == KM_EVENT && !e.down &&
                  e.keycode == alt[i].keycode,
              "0x%02x released as 0x%02x: up %d", alt[i].press, alt[i].release, e.keycode);
    }
}

static void test_insert(void) {
    km_event_t e;

    /* Alt+I is Insert. Let go of Alt with I still down, and the repeats
     * and the release come as `i` (HW §6.2): all of them are Insert's. */
    keymap_reset(&g_km);
    feed(MCU_PRESSED, MCU_ALT, NULL);
    expect(feed(MCU_PRESSED, MCU_INSERT, &e), &e, true, false, KM_KEY_INSERT, KM_KEY_INSERT,
           KM_MOD_ALT, "alt+i");
    feed(MCU_RELEASED, MCU_ALT, NULL);
    expect(feed(MCU_PRESSED, 'i', &e), &e, true, true, KM_KEY_INSERT, KM_KEY_INSERT, KM_MOD_ALT,
           "i repeat");
    expect(feed(MCU_RELEASED, 'i', &e), &e, false, false, KM_KEY_INSERT, KM_KEY_INSERT,
           KM_MOD_ALT, "i up");
    CHECK(feed(MCU_RELEASED, MCU_ENTER, NULL) == KM_STRAY, "Enter was never down");

    /* Alt+I released under Alt, as Insert. */
    keymap_reset(&g_km);
    feed(MCU_PRESSED, MCU_ALT, NULL);
    feed(MCU_PRESSED, MCU_INSERT, NULL);
    expect(feed(MCU_RELEASED, MCU_INSERT, &e), &e, false, false, KM_KEY_INSERT, KM_KEY_INSERT,
           KM_MOD_ALT, "insert up under alt");

    /* Shift+Enter is Insert on Enter's key. */
    keymap_reset(&g_km);
    feed(MCU_PRESSED, MCU_SHIFT_L, NULL);
    feed(MCU_PRESSED, MCU_INSERT, NULL);
    expect(feed(MCU_RELEASED, MCU_INSERT, &e), &e, false, false, KM_KEY_INSERT, KM_KEY_INSERT,
           KM_MOD_SHIFT, "insert up under shift");
    CHECK(feed(MCU_RELEASED, 'i', NULL) == KM_STRAY, "I was never down");
}

static void test_stray_and_repeat(void) {
    km_event_t e;
    keymap_reset(&g_km);

    /* Alt, then a release of Space with no press (HW §6.2). */
    feed(MCU_PRESSED, MCU_ALT, NULL);
    feed(MCU_RELEASED, MCU_ALT, NULL);
    CHECK(feed(MCU_RELEASED, ' ', NULL) == KM_STRAY, "a release with no press is dropped");

    /* Holding j: a press, then more presses, then one release. */
    expect(feed(MCU_PRESSED, 'j', &e), &e, true, false, 'j', 'j', 0, "j");
    for (int i = 0; i < 16; i++) {
        expect(feed(MCU_PRESSED, 'j', &e), &e, true, true, 'j', 'j', 0, "j repeat");
    }
    expect(feed(MCU_RELEASED, 'j', &e), &e, false, false, 'j', 'j', 0, "j up");

    /* Holding F2: holds, not presses. */
    expect(feed(MCU_PRESSED, MCU_F1 + 1, &e), &e, true, false, KM_KEY_F1 + 1, KM_ASCII_F1 + 1, 0,
           "F2");
    expect(feed(MCU_HELD, MCU_F1 + 1, &e), &e, true, true, KM_KEY_F1 + 1, KM_ASCII_F1 + 1, 0,
           "F2 held");

    /* A hold whose press was lost is a press. */
    expect(feed(MCU_HELD, MCU_ESC, &e), &e, true, false, KM_KEY_ESCAPE, 27, 0, "esc held");

    /* A modifier's holds give the engine nothing. */
    feed(MCU_PRESSED, MCU_CTRL, NULL);
    CHECK(feed(MCU_HELD, MCU_CTRL, NULL) == KM_CONSUMED, "ctrl held");
    CHECK(keymap_mods(&g_km) == KM_MOD_CTRL, "ctrl is still down");

    keymap_reset(&g_km);
    CHECK(keymap_mods(&g_km) == 0, "reset lets go of everything");
    CHECK(feed(MCU_RELEASED, 'j', NULL) == KM_STRAY, "reset lets go of every key");
}

static void test_ascii(void) {
    km_event_t e;
    static const struct { uint8_t mod, code; int keycode, ascii; const char *what; } t[] = {
        { 0, 'a', 'a', 'a', "a" },
        { 0, 'A', 'a', 'A', "A under Caps Lock" },
        { MCU_SHIFT_R, 'A', 'a', 'A', "Shift+A" },
        { MCU_CTRL, 'c', 'c', 3, "Ctrl+C" },
        { MCU_ALT, 'M', 'm', 'm', "Alt+M, sent as a capital" },
        { 0, '?', '?', '?', "?" },
        { 0, ' ', ' ', ' ', "space" },
        { 0, MCU_ENTER, KM_KEY_RETURN, 13, "Enter" },
        { 0, MCU_BACKSPACE, KM_KEY_BACKSPACE, 8, "Backspace" },
        { 0, MCU_TAB, KM_KEY_TAB, 9, "Tab" },
        { 0, MCU_ESC, KM_KEY_ESCAPE, 27, "Esc" },
        { 0, MCU_DEL, KM_KEY_DELETE, 127, "Del" },
        { 0, MCU_F1, KM_KEY_F1, KM_ASCII_F1, "F1" },
        { 0, MCU_F1 + 8, KM_KEY_F1 + 8, KM_ASCII_F1 + 8, "F9" },
        { 0, MCU_F10, KM_KEY_F1 + 9, KM_ASCII_F1 + 9, "F10" },
        { 0, MCU_UP, KM_KEY_UP, KM_KEY_UP, "Up" },
        { 0, MCU_LEFT, KM_KEY_LEFT, KM_KEY_LEFT, "Left" },
        { 0, MCU_DOWN, KM_KEY_DOWN, KM_KEY_DOWN, "Down" },
        { 0, MCU_RIGHT, KM_KEY_RIGHT, KM_KEY_RIGHT, "Right" },
        { 0, MCU_PGDN, KM_KEY_PAGEDOWN, KM_KEY_PAGEDOWN, "PgDn" },
    };
    for (unsigned i = 0; i < sizeof t / sizeof t[0]; i++) {
        keymap_reset(&g_km);
        uint8_t flags = 0;
        if (t[i].mod) {
            feed(MCU_PRESSED, t[i].mod, NULL);
            flags = keymap_mods(&g_km);
        }
        expect(feed(MCU_PRESSED, t[i].code, &e), &e, true, false, t[i].keycode, t[i].ascii, flags,
               t[i].what);
    }
}

static void test_chords(void) {
    /* No bound chord needs one the MCU swallows (HW §6.3): Shift with
     * Left, Right, Space or Backspace sends nothing; Alt with , . Space B
     * is the MCU's own; and Alt turns I into Insert. */
    for (unsigned i = 0; i < keymap_chord_count; i++) {
        const km_chord_t *c = &keymap_chords[i];
        if (c->mods & KM_MOD_SHIFT) {
            CHECK(c->key != MCU_LEFT && c->key != MCU_RIGHT && c->key != ' ' &&
                      c->key != MCU_BACKSPACE,
                  "%s needs Shift with a key it swallows", c->what);
        }
        if (c->mods & KM_MOD_ALT) {
            CHECK(c->key != ',' && c->key != '.' && c->key != ' ' && c->key != 'b' &&
                      c->key != 'i',
                  "%s needs Alt with a key the MCU takes", c->what);
        }
        CHECK(keymap_physical(c->key, false) == c->key, "%s names a key by its physical code",
              c->what);
    }

    /* Ctrl+Alt+Del arrives as upstream's quit detector wants it: Ctrl
     * and Alt as LCTRL and LALT downs, then Delete. */
    km_event_t e;
    keymap_reset(&g_km);
    expect(feed(MCU_PRESSED, MCU_CTRL, &e), &e, true, false, KM_KEY_LCTRL, 0, KM_MOD_CTRL, "ctrl");
    expect(feed(MCU_PRESSED, MCU_ALT, &e), &e, true, false, KM_KEY_LALT, 0,
           KM_MOD_CTRL | KM_MOD_ALT, "alt");
    expect(feed(MCU_PRESSED, MCU_DEL, &e), &e, true, false, KM_KEY_DELETE, 127,
           KM_MOD_CTRL | KM_MOD_ALT, "del");
}

int main(void) {
    test_every_code();
    test_retranslated_releases();
    test_insert();
    test_stray_and_repeat();
    test_ascii();
    test_chords();
    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("test_keymap: ok\n");
    return 0;
}

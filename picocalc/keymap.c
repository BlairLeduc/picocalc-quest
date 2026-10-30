/* keymap.c — the southbridge's key events to the engine's (design.md
 * §9.1, §9.2).
 *
 * The codes and their shifted alternates are read from the MCU's keymap,
 * Code/picocalc_keyboard/keyboard.ino at clockworkpi/PicoCalc
 * f91519806d4b2e0a62c4638a9f695cd5162c5479. The ASCII each key gives
 * follows ScummVM's SDL backend (SdlEventSource::mapKey), which the
 * engines were written against.
 */

#include "keymap.h"

#include <string.h>

/* Shifted punctuation and the key it is on (keyboard.ino's kbd_entries
 * and btn_entries, chr then symb). */
static const struct { char shifted, base; } s_alternates[] = {
    { '~', '`' },  { '#', '3' },  { '@', '2' },  { '$', '4' },  { '+', '=' },
    { '_', '-' },  { '|', '\\' }, { '?', '/' },  { '!', '1' },  { '*', '8' },
    { '&', '7' },  { '^', '6' },  { '%', '5' },  { '>', '.' },  { ':', ';' },
    { '"', '\'' }, { '<', ',' },  { ')', '0' },  { '(', '9' },  { '}', ']' },
    { '{', '[' },
};

static const char s_unshifted[] = "`=-\\/.;',[] ";

const km_chord_t keymap_chords[] = {
    { KM_MOD_CTRL | KM_MOD_ALT, MCU_DEL, "quit the game to the selector (upstream's, D9)" },
    { KM_MOD_ALT, 'm', "pointer or keys mode for the arrows (§9.3)" },
};
const unsigned keymap_chord_count = sizeof keymap_chords / sizeof keymap_chords[0];

uint8_t keymap_physical(uint8_t code, bool alt) {
    if (code >= 'A' && code <= 'Z') return (uint8_t)(code + ('a' - 'A'));
    if ((code >= 'a' && code <= 'z') || (code >= '0' && code <= '9')) return code;
    if (code != 0 && strchr(s_unshifted, code)) return code;
    for (unsigned i = 0; i < sizeof s_alternates / sizeof s_alternates[0]; i++) {
        if ((uint8_t)s_alternates[i].shifted == code) return (uint8_t)s_alternates[i].base;
    }
    switch (code) {
    case MCU_BACKSPACE: case MCU_TAB: case MCU_ENTER: case MCU_ESC: case MCU_DEL:
    case MCU_LEFT: case MCU_UP: case MCU_DOWN: case MCU_RIGHT:
    case MCU_ALT: case MCU_SHIFT_L: case MCU_SHIFT_R: case MCU_SYM: case MCU_CTRL:
    case MCU_CAPS: case MCU_POWER:
        return code;
    case MCU_F1: case MCU_F1 + 1: case MCU_F1 + 2: case MCU_F1 + 3: case MCU_F1 + 4:
        return code;
    case MCU_F1 + 5: case MCU_F1 + 6: case MCU_F1 + 7: case MCU_F1 + 8:
        return (uint8_t)(code - 5u);               /* F6-F9 are Shift+F1-F4 */
    case MCU_F10:   return MCU_F1 + 4u;            /* Shift+F5 */
    case MCU_BREAK: return MCU_ESC;
    case MCU_HOME:  return MCU_TAB;
    case MCU_END:   return MCU_DEL;
    case MCU_PGUP:  return MCU_UP;
    case MCU_PGDN:  return MCU_DOWN;
    case MCU_INSERT: return alt ? (uint8_t)'i' : MCU_ENTER;
    default:        return 0;
    }
}

static bool is_modifier(uint8_t phys) {
    return phys == MCU_ALT || phys == MCU_SHIFT_L || phys == MCU_SHIFT_R || phys == MCU_CTRL;
}

uint8_t keymap_mods(const keymap_t *km) {
    uint8_t m = 0;
    if (km->key[MCU_SHIFT_L].down || km->key[MCU_SHIFT_R].down) m |= KM_MOD_SHIFT;
    if (km->key[MCU_CTRL].down) m |= KM_MOD_CTRL;
    if (km->key[MCU_ALT].down) m |= KM_MOD_ALT;
    return m;
}

/* The engine's code and ASCII for a key as the MCU sent it at its press. */
static void translate(uint8_t code, uint8_t mods, uint16_t *keycode, uint16_t *ascii) {
    if ((code >= 'a' && code <= 'z') || (code >= 'A' && code <= 'Z')) {
        /* The MCU sends capitals for Shift and Caps Lock, and leaves
         * letters in capitals under Alt (keyboard.ino's transition_to),
         * so with Alt only Shift decides. Ctrl gives the control
         * character, as SDL does. */
        uint8_t lower = (uint8_t)(code | 0x20u);
        *keycode = lower;
        if (mods & KM_MOD_CTRL) *ascii = (uint16_t)(lower & 0x1Fu);
        else if (mods & KM_MOD_SHIFT) *ascii = (uint16_t)(lower & ~0x20u);
        else if (mods & KM_MOD_ALT) *ascii = lower;
        else *ascii = code;
        return;
    }
    if (code >= 0x20u && code < 0x7Fu) {
        *keycode = *ascii = code;
        return;
    }
    if (code >= MCU_F1 && code <= MCU_F1 + 8u) {
        *keycode = (uint16_t)(KM_KEY_F1 + (code - MCU_F1));
        *ascii = (uint16_t)(KM_ASCII_F1 + (code - MCU_F1));
        return;
    }
    switch (code) {
    case MCU_F10:       *keycode = KM_KEY_F1 + 9;    *ascii = KM_ASCII_F1 + 9; return;
    case MCU_ENTER:     *keycode = KM_KEY_RETURN;    *ascii = 13;  return;
    case MCU_BACKSPACE: *keycode = KM_KEY_BACKSPACE; *ascii = 8;   return;
    case MCU_TAB:       *keycode = KM_KEY_TAB;       *ascii = 9;   return;
    case MCU_ESC:       *keycode = KM_KEY_ESCAPE;    *ascii = 27;  return;
    case MCU_DEL:       *keycode = KM_KEY_DELETE;    *ascii = 127; return;
    case MCU_BREAK:     *keycode = KM_KEY_PAUSE;     *ascii = 0;   return;
    case MCU_ALT:       *keycode = KM_KEY_LALT;      *ascii = 0;   return;
    case MCU_SHIFT_L:   *keycode = KM_KEY_LSHIFT;    *ascii = 0;   return;
    case MCU_SHIFT_R:   *keycode = KM_KEY_RSHIFT;    *ascii = 0;   return;
    case MCU_CTRL:      *keycode = KM_KEY_LCTRL;     *ascii = 0;   return;
    /* The arrows and the editing keys: SDL gives the key code as ASCII. */
    case MCU_UP:        *keycode = *ascii = KM_KEY_UP;       return;
    case MCU_DOWN:      *keycode = *ascii = KM_KEY_DOWN;     return;
    case MCU_RIGHT:     *keycode = *ascii = KM_KEY_RIGHT;    return;
    case MCU_LEFT:      *keycode = *ascii = KM_KEY_LEFT;     return;
    case MCU_INSERT:    *keycode = *ascii = KM_KEY_INSERT;   return;
    case MCU_HOME:      *keycode = *ascii = KM_KEY_HOME;     return;
    case MCU_END:       *keycode = *ascii = KM_KEY_END;      return;
    case MCU_PGUP:      *keycode = *ascii = KM_KEY_PAGEUP;   return;
    case MCU_PGDN:      *keycode = *ascii = KM_KEY_PAGEDOWN; return;
    default:            *keycode = *ascii = 0;               return;
    }
}

void keymap_reset(keymap_t *km) {
    memset(km, 0, sizeof *km);
}

km_result_t keymap_feed(keymap_t *km, uint8_t state, uint8_t code, km_event_t *out) {
    if (state < MCU_PRESSED || state > MCU_RELEASED) return KM_UNKNOWN;
    uint8_t phys = keymap_physical(code, km->key[MCU_ALT].down);
    if (phys == 0) return KM_UNKNOWN;
    /* Accepted and not passed on (§9.2): Caps Lock, which the MCU
     * applies to the letters itself; the power button; Symbol. */
    if (phys == MCU_CAPS || phys == MCU_POWER || phys == MCU_SYM) return KM_CONSUMED;

    km_held_t *h = &km->key[phys];
    if (state == MCU_RELEASED) {
        if (!h->down) return KM_STRAY;
        h->down = false;
        *out = (km_event_t){ false, false, h->keycode, h->ascii, h->flags, phys };
        return KM_EVENT;
    }

    if (h->down) {
        /* A repeat: the MCU's auto-repeat comes as more presses, and as
         * holds for the keys that are not text (§6.2). Modifiers send
         * holds as long as they are down; the engine wants one down. */
        if (is_modifier(phys)) return KM_CONSUMED;
        *out = (km_event_t){ true, true, h->keycode, h->ascii, h->flags, phys };
        return KM_EVENT;
    }

    /* A press, or a hold whose press was lost: either way the key is
     * down from here. A modifier counts in its own flags. */
    h->down = true;
    h->flags = keymap_mods(km);
    /* A key the PicoCalc has only as another's alternate (F6-F10, Home,
     * End, Page Up and Down, Break, Insert) comes without the modifier
     * that reaches it: the engine wants F7, not Shift+F7, which SCI
     * takes for another key (found in M7). Shifted punctuation keeps its
     * Shift, as SDL gives it. */
    if (code >= 0x80u && phys != code) {
        h->flags &= (uint8_t)~(code == MCU_INSERT && phys != MCU_ENTER ? KM_MOD_ALT : KM_MOD_SHIFT);
    }
    translate(code, h->flags, &h->keycode, &h->ascii);
    *out = (km_event_t){ true, false, h->keycode, h->ascii, h->flags, phys };
    return KM_EVENT;
}

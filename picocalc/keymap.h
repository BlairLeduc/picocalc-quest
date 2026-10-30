/* keymap.h — the southbridge's key events to the engine's (design.md
 * §9.1, §9.2).
 *
 * The MCU sends [state, code] pairs: pressed 1, held 2, released 3, with
 * Shift already applied and some keys only reachable as another's
 * shifted alternate (hardware-notes.md §6.2, §6.3). The engine wants key
 * down and up with ScummVM's key codes, its ASCII and modifier flags,
 * and an up that matches its down.
 *
 * So the map keeps the set of physical keys held, each with the event
 * its press sent. Everything about a held key is fixed at its press:
 * repeats send the same down again, and the release sends the matching
 * up, however the MCU translates the release (press `A` / release `a`,
 * press Home / release Tab). A release of a key that is not down is
 * dropped.
 *
 * Pure code, with no Pico SDK, so the host test can drive it (§15).
 * Core 0 owns the map; core 1 only queues the MCU's pairs (kbd.h).
 */
#ifndef PICOCALC_QUEST_KEYMAP_H
#define PICOCALC_QUEST_KEYMAP_H

#include <stdbool.h>
#include <stdint.h>

/* The MCU's codes (hardware-notes.md §6.2; keyboard.h in the MCU source,
 * clockworkpi/PicoCalc f915198). Printable keys are ASCII. */
#define MCU_BACKSPACE 0x08u
#define MCU_TAB       0x09u
#define MCU_ENTER     0x0Au
#define MCU_F1        0x81u   /* F1-F5 0x81-0x85; F6-F9 0x86-0x89 */
#define MCU_F10       0x90u
#define MCU_POWER     0x91u   /* the power button, a press with no release */
#define MCU_ALT       0xA1u
#define MCU_SHIFT_L   0xA2u
#define MCU_SHIFT_R   0xA3u
#define MCU_SYM       0xA4u   /* defined, never sent */
#define MCU_CTRL      0xA5u
#define MCU_ESC       0xB1u
#define MCU_LEFT      0xB4u
#define MCU_UP        0xB5u
#define MCU_DOWN      0xB6u
#define MCU_RIGHT     0xB7u
#define MCU_CAPS      0xC1u
#define MCU_BREAK     0xD0u   /* Shift+Esc */
#define MCU_INSERT    0xD1u   /* Shift+Enter, or Alt+I */
#define MCU_HOME      0xD2u   /* Shift+Tab */
#define MCU_DEL       0xD4u
#define MCU_END       0xD5u   /* Shift+Del */
#define MCU_PGUP      0xD6u   /* Shift+Up */
#define MCU_PGDN      0xD7u   /* Shift+Down */

#define MCU_PRESSED   1u
#define MCU_HELD      2u
#define MCU_RELEASED  3u

/* ScummVM's key codes (common/keyboard.h), which upstream's CABAL_KEY_*
 * match. */
#define KM_KEY_BACKSPACE 8
#define KM_KEY_TAB       9
#define KM_KEY_RETURN    13
#define KM_KEY_PAUSE     19
#define KM_KEY_ESCAPE    27
#define KM_KEY_DELETE    127
#define KM_KEY_UP        273
#define KM_KEY_DOWN      274
#define KM_KEY_RIGHT     275
#define KM_KEY_LEFT      276
#define KM_KEY_INSERT    277
#define KM_KEY_HOME      278
#define KM_KEY_END       279
#define KM_KEY_PAGEUP    280
#define KM_KEY_PAGEDOWN  281
#define KM_KEY_F1        282   /* F1-F10 are 282-291 */
#define KM_KEY_RSHIFT    303
#define KM_KEY_LSHIFT    304
#define KM_KEY_LCTRL     306
#define KM_KEY_LALT      308
#define KM_ASCII_F1      315   /* ScummVM's ASCII for F1-F9 */

/* Upstream's CABAL_MOD_* (rp2350-minimal.h). */
#define KM_MOD_SHIFT 0x01u
#define KM_MOD_CTRL  0x02u
#define KM_MOD_ALT   0x04u

typedef struct {
    bool     down;      /* key down, or key up                       */
    bool     repeat;    /* a down for a key that is already down     */
    uint16_t keycode;   /* KM_KEY_*, or the lowercase ASCII          */
    uint16_t ascii;     /* as ScummVM's SDL backend would give it    */
    uint8_t  flags;     /* KM_MOD_*, as they were at the press       */
    uint8_t  key;       /* the physical key, keymap_physical()'s name */
} km_event_t;

typedef enum {
    KM_EVENT,     /* *out holds an event for the engine                    */
    KM_CONSUMED,  /* accepted, nothing for the engine: a modifier's hold,
                     Caps Lock, the power button                           */
    KM_STRAY,     /* a release of a key that is not down; dropped (§6.2)   */
    KM_UNKNOWN,   /* a state or code the MCU's keymap does not send        */
} km_result_t;

typedef struct {
    uint16_t keycode;
    uint16_t ascii;
    uint8_t  flags;
    bool     down;
} km_held_t;

typedef struct {
    km_held_t key[256];   /* by physical key, keymap_physical() */
} keymap_t;

void keymap_reset(keymap_t *km);

/* One MCU pair in; at most one engine event out. */
km_result_t keymap_feed(keymap_t *km, uint8_t state, uint8_t code, km_event_t *out);

/* The physical key a code comes from, named by the code that key sends
 * unshifted (letters in lower case): `!` is `1`, Home is Tab, F6 is F1.
 * Insert is Enter's alternate, or I's under Alt (hardware-notes.md
 * §6.2), so it needs Alt's state. 0 for a code the MCU does not send. */
uint8_t keymap_physical(uint8_t code, bool alt);

/* The modifiers held now, KM_MOD_*. */
uint8_t keymap_mods(const keymap_t *km);

/* A chord the port binds (§9.5): modifiers held, then a physical key. */
typedef struct {
    uint8_t     mods;
    uint8_t     key;    /* keymap_physical()'s name for it */
    const char *what;
} km_chord_t;

extern const km_chord_t keymap_chords[];
extern const unsigned   keymap_chord_count;

#endif /* PICOCALC_QUEST_KEYMAP_H */

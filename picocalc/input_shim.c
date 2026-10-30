/* input_shim.c — what upstream's input code calls, on a PicoCalc that has
 * no PS/2 port (design.md §5.3, §9).
 *
 * Keys come from the southbridge's ring (kbd.h) through the key map
 * (keymap.h) into the patched cabal_poll_event(). ps2kbd_init and
 * ps2kbd_flush keep their upstream names: the patch leaves their calls
 * in place, and the flush after a game drops what is queued and lets go
 * of every key.
 *
 * During a game the arrows are a mouse or arrow keys (§9.3): in pointer
 * mode they and `[` `]` go to the pointer (pointer.h), which upstream's
 * poll reads through ps2_mouse_get_state; Alt+M switches modes. Where a
 * key goes is decided at its press and kept to its release, so a mode
 * change with a key held cannot strand its up. In the selector every key
 * goes to it. The button keys, the pointer's speed and each engine's
 * starting mode are the settings' (settings.h, §10.3).
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "kbd.h"
#include "keymap.h"
#include "picocalc_input.h"
#include "pointer.h"
#include "ps2.h"
#include "ps2kbd_wrapper.h"
#include "settings.h"
#include "status.h"

/* main.c: park core 1 and reboot (§13.1). */
void picocalc_restart(void);

static keymap_t s_keymap;
static pointer_t s_pointer;

typedef enum { MODE_SELECTOR, MODE_KEYS, MODE_POINTER } input_mode_t;
static input_mode_t s_mode = MODE_SELECTOR;

/* Where each physical key's events go, set at its press. */
typedef enum { ROUTE_ENGINE, ROUTE_MODE, ROUTE_ARROW, ROUTE_BUTTON } route_t;
static uint8_t s_route[256];

/* An arrow's press, until the pointer first moves for it: the time core
 * 1 read it, for the latency to the engine (§16 M6). */
static bool s_latency_pending;
static uint32_t s_latency_from;

volatile uint32_t g_input_taken;
volatile uint32_t g_input_wait_sum_us;
volatile uint32_t g_input_wait_max_us;
volatile uint32_t g_input_unknown;
volatile uint32_t g_input_stray;

volatile uint32_t g_pointer_latency_max_us;

/* The band's help for each mode, with the button keys in it. */
static char s_help_pointer[48], s_help_keys[48];

void ps2kbd_init(void) {
    keymap_reset(&s_keymap);
    pointer_init(&s_pointer);
    s_pointer.v0 = POINTER_V0 * g_settings.pointer_speed / 100u;
    s_pointer.v1 = POINTER_V1 * g_settings.pointer_speed / 100u;
    snprintf(s_help_pointer, sizeof s_help_pointer, "%c left button  %c right  Ctrl+arrow: 1 px",
             g_settings.left_button, g_settings.right_button);
    snprintf(s_help_keys, sizeof s_help_keys, "%c and %c type themselves",
             g_settings.left_button, g_settings.right_button);
}

void ps2kbd_flush(void) {
    kbd_flush();
    keymap_reset(&s_keymap);
    pointer_reset(&s_pointer);
    memset(s_route, ROUTE_ENGINE, sizeof s_route);
    s_latency_pending = false;
}

static void show_mode(void) {
    if (s_mode == MODE_POINTER) {
        status_show("Arrows: pointer  (Alt+M: keys)", s_help_pointer);
    } else {
        status_show("Arrows: keys  (Alt+M: pointer)", s_help_keys);
    }
}

void picocalc_input_game(int engine) {
    pointer_reset(&s_pointer);
    s_latency_pending = false;
    if (engine == PICOCALC_ENGINE_NONE) {
        s_mode = MODE_SELECTOR;
        return;
    }
    /* By default AGI's arrows are keys, every other engine's the
     * pointer (settings.c). */
    bool pointer = engine > 0 && engine < SETTINGS_ENGINES && g_settings.pointer_mode[engine];
    s_mode = pointer ? MODE_POINTER : MODE_KEYS;
    show_mode();
}

static route_t route_for(const km_event_t *e) {
    if (s_mode == MODE_SELECTOR) return ROUTE_ENGINE;
    if (e->keycode == 'm' && e->flags == KM_MOD_ALT) return ROUTE_MODE;
    if (s_mode != MODE_POINTER) return ROUTE_ENGINE;
    if (e->keycode >= KM_KEY_UP && e->keycode <= KM_KEY_LEFT) return ROUTE_ARROW;
    if (e->keycode == g_settings.left_button || e->keycode == g_settings.right_button) {
        return ROUTE_BUTTON;
    }
    return ROUTE_ENGINE;
}

static ptr_arrow_t arrow_of(uint16_t keycode) {
    switch (keycode) {
    case KM_KEY_UP:   return PTR_UP;
    case KM_KEY_DOWN: return PTR_DOWN;
    case KM_KEY_LEFT: return PTR_LEFT;
    default:          return PTR_RIGHT;
    }
}

/* A key routed away from the engine: true if it is used up here. */
static bool take_key(route_t route, const km_event_t *e, uint32_t queued) {
    switch (route) {
    case ROUTE_ENGINE:
        return false;
    case ROUTE_MODE:
        if (e->down && !e->repeat) {
            s_mode = s_mode == MODE_POINTER ? MODE_KEYS : MODE_POINTER;
            show_mode();
#if PICOCALC_QUEST_UART
            printf("input: arrows are %s\n", s_mode == MODE_POINTER ? "the pointer" : "keys");
#endif
        }
        return true;
    case ROUTE_ARROW: {
        if (e->repeat) return true;
        ptr_arrow_t a = arrow_of(e->keycode);
        bool nudge = (e->flags & KM_MOD_CTRL) != 0;
        pointer_arrow(&s_pointer, a, e->down, nudge, queued);
        if (e->down) {
            s_latency_pending = true;
            s_latency_from = queued;
        }
#if PICOCALC_QUEST_UART
        static uint32_t pressed_at[4];
        static const char *const names[] = { "left", "right", "up", "down" };
        if (e->down) {
            pressed_at[a] = queued;
        } else if (!nudge) {
            uint32_t held = queued - pressed_at[a];
            printf("pointer: %s held %lu ms, %lu px\n", names[a], (unsigned long)(held / 1000u),
                   (unsigned long)pointer_run_px(&s_pointer, held));
        }
#endif
        return true;
    }
    case ROUTE_BUTTON:
        if (!e->repeat) {
            pointer_button(&s_pointer,
                           e->keycode == g_settings.left_button ? PTR_BUTTON_LEFT : PTR_BUTTON_RIGHT,
                           e->down);
        }
        return true;
    }
    return false;
}

#if PICOCALC_QUEST_UART
/* Every key, as the MCU sent it and as the engine gets it, so a run
 * through the whole keyboard can be checked from the log (§16 M3). */
static void log_key(uint8_t state, uint8_t code, km_result_t r, const km_event_t *e,
                    uint32_t wait_us) {
    static const char *const states[] = { "?", "pressed", "held", "released" };
    char shown[4] = "";
    if (code >= 0x20u && code < 0x7Fu) snprintf(shown, sizeof shown, " %c", code);
    printf("key %s 0x%02x%s (%lu us): ", states[state <= 3u ? state : 0u], code, shown,
           (unsigned long)wait_us);
    switch (r) {
    case KM_EVENT:
        printf("%s%s %d ascii %d flags %d\n", e->down ? "down" : "up",
               e->repeat ? " (repeat)" : "", e->keycode, e->ascii, e->flags);
        break;
    case KM_CONSUMED: printf("taken, not passed on\n"); break;
    case KM_STRAY:    printf("dropped, the key was not down\n"); break;
    case KM_UNKNOWN:  printf("UNKNOWN to the key map\n"); break;
    }
}
#endif

int picocalc_input_key(int *pressed, int *keycode, int *ascii, int *flags) {
    uint8_t state, code;
    uint32_t queued;
    if (kbd_uart_restart()) {
        printf("picocalc-quest: restart asked for over the UART\n");
        picocalc_restart();
    }
    while (kbd_pop(&state, &code, &queued)) {
        uint32_t wait = time_us_32() - queued;
        g_input_taken++;
        g_input_wait_sum_us += wait;
        if (wait > g_input_wait_max_us) g_input_wait_max_us = wait;

        km_event_t e;
        km_result_t r = keymap_feed(&s_keymap, state, code, &e);
#if PICOCALC_QUEST_UART
        log_key(state, code, r, &e, wait);
#endif
        if (r == KM_UNKNOWN) g_input_unknown++;
        if (r == KM_STRAY) g_input_stray++;
        if (r != KM_EVENT) continue;
        if (e.down && !e.repeat) s_route[e.key] = (uint8_t)route_for(&e);
        if (take_key((route_t)s_route[e.key], &e, queued)) continue;
        /* Upstream's event manager makes its own repeats of a held key,
         * as ScummVM does everywhere; the MCU's, passed on as more
         * presses, doubled them and restarted its timer, and SCI0 takes
         * a second press of an arrow as stop (§9.1, M5). */
        if (e.repeat) continue;

        *pressed = e.down;
        *keycode = e.keycode;
        *ascii = e.ascii;
        *flags = e.flags;
        return 1;
    }
    return 0;
}

/* Upstream's poll reads the mouse here when no key is waiting: the
 * pointer's motion, with y up, as a PS/2 mouse gives it. */
bool ps2_mouse_get_state(int16_t *dx, int16_t *dy, int8_t *wheel, uint8_t *buttons) {
    int16_t x, y;
    uint32_t now = time_us_32();
    bool moved = pointer_motion(&s_pointer, now, &x, &y);
    *dx = x;
    *dy = (int16_t)-y;
    *wheel = 0;
    *buttons = s_pointer.buttons;
    if (moved && s_latency_pending) {
        s_latency_pending = false;
        uint32_t latency = now - s_latency_from;
        if (latency > g_pointer_latency_max_us) g_pointer_latency_max_us = latency;
#if PICOCALC_QUEST_UART
        printf("pointer: key to motion %lu us\n", (unsigned long)latency);
#endif
    }
    return moved;
}

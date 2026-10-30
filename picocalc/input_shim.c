/* input_shim.c — what upstream's input code calls, on a PicoCalc that has
 * no PS/2 port (design.md §5.3, §9).
 *
 * Keys come from the southbridge's ring (kbd.h) through the key map
 * (keymap.h) into the patched cabal_poll_event(). ps2kbd_init and
 * ps2kbd_flush keep their upstream names: the patch leaves their calls
 * in place, and the flush after a game drops what is queued and lets go
 * of every key. There is no mouse until the pointer (§9.3, M6).
 */

#include <stdio.h>

#include "pico/stdlib.h"

#include "kbd.h"
#include "keymap.h"
#include "picocalc_input.h"
#include "ps2.h"
#include "ps2kbd_wrapper.h"

/* main.c: park core 1 and reboot (§13.1). */
void picocalc_restart(void);

static keymap_t s_keymap;

volatile uint32_t g_input_taken;
volatile uint32_t g_input_wait_sum_us;
volatile uint32_t g_input_wait_max_us;
volatile uint32_t g_input_unknown;
volatile uint32_t g_input_stray;

void ps2kbd_init(void) {
    keymap_reset(&s_keymap);
}

void ps2kbd_flush(void) {
    kbd_flush();
    keymap_reset(&s_keymap);
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

/* The mouse block of upstream's poll stays until the pointer replaces
 * it (M6): no motion, no buttons. */
bool ps2_mouse_get_state(int16_t *dx, int16_t *dy, int8_t *wheel, uint8_t *buttons) {
    *dx = 0;
    *dy = 0;
    *wheel = 0;
    *buttons = 0;
    return false;
}

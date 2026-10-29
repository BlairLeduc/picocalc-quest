/* fatal.c — when upstream's error() ends the program (design.md §13.1).
 *
 * ScummVM's error() logs its message, then OSystem::fatalError() calls
 * exit(), which the SDK ends in a breakpoint: with the probe attached the
 * core halts, and the timer with it; without it, a fault. Either way the
 * panel froze on the last frame. The patched logMessage() hands the
 * message here, and the link wraps exit(), so the error shows as a page
 * and any key restarts to the selector. Engine state is past saving by
 * then, so a restart is the way back.
 */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "page.h"
#include "picocalc_input.h"

void picocalc_restart(void);

static char s_message[PAGE_MESSAGE_MAX];

/* Core 0, from the patched OSystem_RP2350::logMessage(): the last error
 * logged. */
void picocalc_fatal_message(const char *message) {
    strncpy(s_message, message, sizeof s_message - 1);
    s_message[sizeof s_message - 1] = '\0';
}

void __wrap_exit(int status) {
    printf("picocalc-quest: exit(%d); the error page, then a restart on a key\n", status);
    page_show_message(PAGE_GAME_ERROR, s_message);

    /* Keys already in the ring were meant for the game. */
    int pressed, keycode, ascii, flags;
    busy_wait_us_32(100000);
    while (picocalc_input_key(&pressed, &keycode, &ascii, &flags)) {}
    for (;;) {
        if (picocalc_input_key(&pressed, &keycode, &ascii, &flags) && pressed) break;
        busy_wait_us_32(10000);
    }
    picocalc_restart();
}

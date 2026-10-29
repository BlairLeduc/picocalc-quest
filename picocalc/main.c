/* main.c — the PicoCalc's main(), in place of upstream's src/main.c
 * (design.md §5.3, §13.1).
 *
 * M0: upstream's start-up less its clock, HDMI and USB console, so that
 * the whole engine set is linked and its size can be measured. The clocks
 * and rail (§11.1), the log ring and banner (§14.2) and the PSRAM verify
 * (§11.3) come in M1; the three-buffer pool (§7.2) in M2.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"

#include "board_config.h"
#include "psram_allocator.h"
#include "psram_init.h"

void cabal_init(void);
int cabal_main(void);
void cabal_enable_psram_heap(void);

/* Upstream's two PSRAM framebuffer slots, as its main.c uses them. Its
 * swap also handed the new front buffer to HDMI (graphics_set_buffer);
 * nothing presents in M0, so that hand-off is gone until M2's pool
 * publishes the buffer to core 1 (§7.2). */
static uint8_t *framebuffer[2];
static int front;

uint8_t *cabal_get_framebuffer(void) {
    return framebuffer[front];
}

uint8_t *cabal_get_back_buffer(void) {
    return framebuffer[1 - front];
}

void cabal_swap_buffers(void) {
    front = 1 - front;
}

int main(void) {
    stdio_init_all();
    printf("\npicocalc-quest (FRANK Quest %s)\n", CABAL_VERSION);

    psram_init(get_psram_pin());
    psram_set_sram_mode(0);
    cabal_enable_psram_heap();

    framebuffer[0] = psram_get_framebuffer();
    framebuffer[1] = psram_get_framebuffer_back();
    memset(framebuffer[0], 0, CABAL_FRAMEBUFFER_SIZE);
    memset(framebuffer[1], 0, CABAL_FRAMEBUFFER_SIZE);

    cabal_init();
    return cabal_main();
}

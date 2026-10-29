/* input_shim.c — what upstream's input code calls, on a PicoCalc that has
 * no PS/2 port (design.md §5.3, §9).
 *
 * ps2kbd_init and ps2kbd_flush stay: they will start and flush the
 * southbridge's event queue (M3). The rest are M0's stand-ins for calls
 * that M3's patch to rp2350-minimal.cpp removes; until then there are no
 * keys and no mouse.
 */

#include "ps2.h"
#include "ps2kbd_wrapper.h"

void ps2kbd_init(void) {
}

void ps2kbd_flush(void) {
}

void ps2kbd_tick(void) {
}

int ps2kbd_get_key_ext(int *pressed, unsigned char *key, uint8_t *hid_code) {
    (void)pressed;
    (void)key;
    (void)hid_code;
    return 0;
}

bool ps2_mouse_pio_init(PIO pio, uint mouse_clk) {
    (void)pio;
    (void)mouse_clk;
    return false;
}

bool ps2_mouse_init_device(void) {
    return false;
}

bool ps2_mouse_get_state(int16_t *dx, int16_t *dy, int8_t *wheel, uint8_t *buttons) {
    *dx = 0;
    *dy = 0;
    *wheel = 0;
    *buttons = 0;
    return false;
}

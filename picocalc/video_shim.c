/* video_shim.c — upstream's graphics_* over the PicoCalc's LCD (design.md
 * §5.3, §7).
 *
 * M0: the palette is kept as RGB565 in the pending table, and nothing is
 * presented. The frame pool, which carries the palette with each frame,
 * and the presenter on core 1 come in M2 (§7.2, §7.3).
 */

#include <stdint.h>

#include "HDMI.h"

static uint16_t pending_palette[256];

void graphics_set_palette(uint8_t i, uint32_t color888) {
    uint32_t r = (color888 >> 16) & 0xff;
    uint32_t g = (color888 >> 8) & 0xff;
    uint32_t b = color888 & 0xff;
    pending_palette[i] = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

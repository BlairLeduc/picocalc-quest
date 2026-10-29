/* core1.h — core 1's loop: the panel, the southbridge and the log
 * (design.md §6.1).
 *
 * Bring-up of the southbridge and the LCD and the corner-coded test
 * pattern (M1); then the presenter (M2, §7.3), the log pump and a
 * heartbeat once a second. The keyboard comes in M3.
 */
#ifndef PICOCALC_QUEST_CORE1_H
#define PICOCALC_QUEST_CORE1_H

#include <stdbool.h>
#include <stdint.h>

/* What core 1 found at bring-up, for core 0's banner. Written before
 * `ready` is set, and read only after core 0 sees it. */
typedef struct {
    volatile bool ready;
    uint32_t      i2c_hz;        /* rate the I2C peripheral took        */
    int           sb_version;    /* -1 if the southbridge did not answer */
    uint32_t      i2c_us;        /* one register read                   */
    uint32_t      spi_hz;        /* the LCD's, as configured            */
    uint32_t      lcd_us;        /* lcd_init, reset to DISPON           */
    uint32_t      pattern_us;    /* drawing the test pattern            */
} core1_bringup_t;

extern core1_bringup_t g_core1;

/* Set by core 0 when the PSRAM fails its check (§11.3): core 1 paints
 * the picture's area red, until M3's error pages can name it. */
extern volatile bool g_psram_failed;

/* Core 0's time in upstream's cabal_delay since boot, in microseconds,
 * kept by core 0 (main.c) for the heartbeat's busy share (§14.3). */
extern volatile uint32_t g_core0_delay_us;

/* Core 0, before launching core 1: fill its stack with a pattern, so the
 * heartbeat can report how deep it has gone (§12.1). */
void core1_paint_stack(void);

void core1_main(void);

#endif /* PICOCALC_QUEST_CORE1_H */

/* audio_shim.c — upstream's cabal_audio_* over the PicoCalc's PWM audio
 * (design.md §5.3, §8).
 *
 * M0: silent. Init reports no audio, and the mixer callback is kept but
 * never called. The PWM ring and the mixer in its refill IRQ come in M5.
 */

#include <stddef.h>

#include "audio.h"

void cabal_audio_set_mixer_callback(void (*callback)(uint8_t *stream, int len));

static void (*mixer_callback)(uint8_t *stream, int len);

void cabal_audio_set_mixer_callback(void (*callback)(uint8_t *stream, int len)) {
    mixer_callback = callback;
}

bool cabal_audio_init(void) {
    return false;
}

void cabal_audio_shutdown(void) {
    mixer_callback = NULL;
}

void cabal_audio_process_frame(void) {
}

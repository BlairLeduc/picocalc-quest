/* audio_shim.c — upstream's cabal_audio_* over the PicoCalc's PWM audio
 * (design.md §5.3, §8).
 *
 * Until M5: silent. Init reports no audio. The mixer still runs, into a
 * scratch buffer that is thrown away, at the rate it was created at, so
 * sounds advance and end on time: a CD title's intro waits for its voice
 * lines to finish, and with the mixer never called they never did (M4).
 * The PWM ring and the mixer in its refill IRQ come in M5.
 */

#include <stddef.h>
#include <stdint.h>

#include "pico/time.h"

#include "audio.h"

/* Upstream's initBackend() creates the mixer at this rate (§8.2). */
#define MIX_RATE 44100u
/* Frames per call to the mixer: 16-bit stereo, 2 KB of scratch. */
#define MIX_CHUNK 512u
/* After a long stall (a load, say), catch up at most this much. */
#define MIX_MAX_OWED (MIX_RATE / 10u)

void cabal_audio_set_mixer_callback(void (*callback)(uint8_t *stream, int len));

static void (*mixer_callback)(uint8_t *stream, int len);
static int16_t s_scratch[2 * MIX_CHUNK];
static uint32_t s_last_us, s_frac;
static bool s_started;

void cabal_audio_set_mixer_callback(void (*callback)(uint8_t *stream, int len)) {
    mixer_callback = callback;
    s_started = false;
}

bool cabal_audio_init(void) {
    return false;
}

void cabal_audio_shutdown(void) {
    mixer_callback = NULL;
}

/* Core 0, from upstream's updateScreen() and delayMillis(): mix the
 * frames the time since the last call is owed. */
void cabal_audio_process_frame(void) {
    if (mixer_callback == NULL) return;
    uint32_t now = time_us_32();
    if (!s_started) {
        s_started = true;
        s_last_us = now;
        s_frac = 0;
        return;
    }
    uint64_t owed = (uint64_t)(now - s_last_us) * MIX_RATE + s_frac;
    s_last_us = now;
    s_frac = (uint32_t)(owed % 1000000u);
    uint32_t frames = (uint32_t)(owed / 1000000u);
    if (frames > MIX_MAX_OWED) frames = MIX_MAX_OWED;
    while (frames > 0) {
        uint32_t n = frames < MIX_CHUNK ? frames : MIX_CHUNK;
        mixer_callback((uint8_t *)s_scratch, (int)(n * 4u));
        frames -= n;
    }
}

/* test_sound.c — the mixer's samples to PWM compare words (design.md
 * §8.2, §15).
 *
 * Every int16 sample at every volume against a model in doubles: the
 * gain, 16 bits to 12, floor, about the mid-point, saturated.
 */

#include <math.h>
#include <stdio.h>

#include "sound.h"

static int failures;

#define CHECK(cond, ...)                                    \
    do {                                                    \
        if (!(cond)) {                                      \
            failures++;                                     \
            fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
            fprintf(stderr, __VA_ARGS__);                   \
            fputc('\n', stderr);                            \
        }                                                   \
    } while (0)

static long model(int s, int gain) {
    double v = 2048.0 + floor((double)s * gain / 128.0 / 16.0);
    if (v < 0.0) v = 0.0;
    if (v > 4095.0) v = 4095.0;
    return (long)v;
}

static void test_levels(void) {
    for (int gain = 0; gain <= 255; gain++) {
        unsigned prev = 0;
        for (int s = -32768; s <= 32767; s++) {
            unsigned got = sound_level(s, gain);
            if ((long)got != model(s, gain)) {
                CHECK(0, "sample %d at volume %d -> %u, want %ld", s, gain, got, model(s, gain));
                return;
            }
            CHECK(got >= prev, "sample %d at volume %d: not monotonic", s, gain);
            prev = got;
        }
    }
    CHECK(sound_level(0, SOUND_UNITY) == SOUND_MID, "silence is the mid-point");
    CHECK(sound_level(-32768, SOUND_UNITY) == 0, "full scale down at unity is 0");
    CHECK(sound_level(32767, SOUND_UNITY) == SOUND_TOP, "full scale up at unity is TOP");
    CHECK(sound_level(32767, 0) == SOUND_MID && sound_level(-32768, 0) == SOUND_MID,
          "volume 0 is silence");
    CHECK(sound_level(20000, 255) == SOUND_TOP && sound_level(-20000, 255) == 0,
          "past unity saturates");
}

static void test_words(void) {
    CHECK(sound_word(0, 0, SOUND_UNITY) == (SOUND_MID | (SOUND_MID << 16)), "silence");
    CHECK(sound_word(32767, -32768, SOUND_UNITY) == SOUND_TOP, "left in A, right in B");
    CHECK(sound_word(-32768, 32767, SOUND_UNITY) == (SOUND_TOP << 16), "right in B, left in A");
}

int main(void) {
    test_levels();
    test_words();
    if (failures) {
        fprintf(stderr, "test_sound: %d failure(s)\n", failures);
        return 1;
    }
    printf("test_sound: ok\n");
    return 0;
}

/* picocalc_input.h — the PicoCalc's keys for upstream's cabal_poll_event()
 * (design.md §5.3, §9.1).
 *
 * The patched poll in rp2350-minimal.cpp calls picocalc_input_key() in
 * place of the PS/2 driver, and passes what it gets to upstream's
 * Ctrl+Alt+Del detector and on to the engine. C linkage, for the C++
 * caller.
 */
#ifndef PICOCALC_QUEST_PICOCALC_INPUT_H
#define PICOCALC_QUEST_PICOCALC_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Core 0: the next key event for the engine, if any. `pressed` is 1 for
 * a down (a repeat included) and 0 for an up; `keycode` is ScummVM's,
 * `ascii` as ScummVM's SDL backend gives it, `flags` upstream's
 * CABAL_MOD_*. Returns 1 with an event, 0 without. */
int picocalc_input_key(int *pressed, int *keycode, int *ascii, int *flags);

/* The engines, as upstream's QuestEngine numbers them
 * (frank_quest_selector.h), and none for the selector. */
#define PICOCALC_ENGINE_NONE  0
#define PICOCALC_ENGINE_SCUMM 1
#define PICOCALC_ENGINE_SCI   2
#define PICOCALC_ENGINE_KYRA  3
#define PICOCALC_ENGINE_GOB   4
#define PICOCALC_ENGINE_AGI   5

/* Core 0, before a game starts and after it ends: the arrows' mode for
 * that engine (§9.3), shown in the status band; with NONE, every key
 * goes to the selector. */
void picocalc_input_game(int engine);

/* The longest from an arrow's read by core 1 to the pointer's first
 * motion given to the engine, since boot (§16 M6). */
extern volatile uint32_t g_pointer_latency_max_us;

/* For the heartbeat (§14.3), written by core 0: events taken from the
 * ring, the sum and the longest of their waits from core 1's read of
 * the MCU's FIFO to the engine's poll, and the MCU's events the key map
 * refused (codes it does not know) or dropped (releases of keys not
 * down). */
extern volatile uint32_t g_input_taken;
extern volatile uint32_t g_input_wait_sum_us;
extern volatile uint32_t g_input_wait_max_us;
extern volatile uint32_t g_input_unknown;
extern volatile uint32_t g_input_stray;

#ifdef __cplusplus
}
#endif

#endif /* PICOCALC_QUEST_PICOCALC_INPUT_H */

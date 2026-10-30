/* picocalc_exit.h — leaving from the selector (design.md §13.3).
 *
 * Esc in the selector goes back to the shell when UF2 Loader started the
 * port and a card is in (picocalc_shell.h), and otherwise powers the
 * PicoCalc off through the southbridge, if its firmware can. The
 * selector's legend names which, decided as it is drawn. C linkage, for
 * upstream's selector, which is C++.
 */
#ifndef PICOCALC_QUEST_PICOCALC_EXIT_H
#define PICOCALC_QUEST_PICOCALC_EXIT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Core 0: the legend's words for Esc, "ESC: Shell" or "ESC: Power off",
 * or NULL when it can do neither. */
const char *picocalc_exit_label(void);

/* Core 0, in the selector, with no file open: leave. Returns only if it
 * cannot, having said why in the status band. */
void picocalc_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* PICOCALC_QUEST_PICOCALC_EXIT_H */

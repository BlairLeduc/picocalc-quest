/* status.h — the status band below the picture (design.md §7.4).
 *
 * The 40 rows under upstream's 320x240 are outside anything it draws.
 * Core 0 puts a message there, a title and a line of help, for a few
 * seconds: the arrows' mode when it changes (§9.3), a settings line not
 * used (§10.3), and why the selector cannot exit (§13.3). Core 1, which
 * owns the LCD, draws it once when it changes and clears it when its
 * time is up (EL §5.4).
 */
#ifndef PICOCALC_QUEST_STATUS_H
#define PICOCALC_QUEST_STATUS_H

#include <stdbool.h>
#include <stdint.h>

#define STATUS_SHOW_US 4000000u
/* For what is easy to miss and matters: a settings line not used. */
#define STATUS_LONG_US 10000000u

/* Core 0: show `title` over `help` (either may be NULL) for
 * STATUS_SHOW_US. Both are read when core 1 draws them, so they must
 * stay: string literals, or static buffers not written again. */
void status_show(const char *title, const char *help);

/* The same, for `us`. */
void status_show_for(const char *title, const char *help, uint32_t us);

/* Core 1, every loop: draw or clear the band if it is due. Nothing is
 * drawn while `covered` (a page is up). */
void status_service(uint32_t now_us, bool covered);

#endif /* PICOCALC_QUEST_STATUS_H */

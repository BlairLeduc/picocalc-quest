/* settings.h — /quest/picocalc.ini, this port's settings (design.md
 * §10.3).
 *
 * A text file of `key = value` lines, read once at boot on core 0, after
 * the card is mounted and before upstream's cabal_init(). Every default
 * is in settings_defaults() (EL §8.7). Keys and the words `pointer` and
 * `keys` are in any case; blank lines, and lines whose first character
 * is # or ;, are comments; so is the rest of a value from a # or ; that
 * follows a space. A value's first character never starts a comment, so
 * `left_button = #` is the # key. A key given twice
 * takes the later value. A line that cannot be used is named, with why,
 * and the rest of the file still applies:
 *
 *     left_button   = ]          one printable key; a letter in any case
 *     right_button  = [
 *     pointer_speed = 100        per cent of the curve of §9.3, 25-400
 *     volume        = 100        per cent of the mixer's level, 0-200
 *     agi_mode      = keys       what the arrows start as, by engine:
 *     sci_mode      = pointer    pointer or keys (§9.3)
 *     scumm_mode    = pointer
 *     kyra_mode     = pointer
 *     gob_mode      = pointer
 *     opl           = mame       the AdLib emulator, mame or dosbox (§8.3)
 *     opl.dott      = dosbox     the same for one game, by its folder in
 *                                /quest; up to SETTINGS_OPL_GAMES of them
 *
 * The user writes it; nothing here writes it (a menu that does is
 * deferred, §19).
 *
 * The parser is pure code, with no Pico SDK, so the host test can drive
 * it (§15).
 */
#ifndef PICOCALC_QUEST_SETTINGS_H
#define PICOCALC_QUEST_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SETTINGS_PATH "/quest/picocalc.ini"

#define SETTINGS_SPEED_MIN  25u
#define SETTINGS_SPEED_MAX  400u
#define SETTINGS_VOLUME_MAX 200u

/* The OPL (AdLib) emulators, by upstream's opl_driver names. */
enum { SETTINGS_OPL_DOSBOX, SETTINGS_OPL_MAME };

/* Games with their own OPL emulator, and their folder names' length. */
#define SETTINGS_OPL_GAMES  16
#define SETTINGS_FOLDER_MAX 24

typedef struct {
    char    folder[SETTINGS_FOLDER_MAX];   /* lower case, NUL-terminated */
    uint8_t opl;
} settings_opl_game_t;

/* By the engine numbers of picocalc_input.h, NONE (0) unused. */
#define SETTINGS_ENGINES 6

typedef struct {
    uint8_t  left_button;     /* the key's code: printable ASCII, letters lower case */
    uint8_t  right_button;
    uint16_t pointer_speed;   /* per cent of the curve                 */
    uint16_t volume;          /* per cent of the mixer's level         */
    bool     pointer_mode[SETTINGS_ENGINES];   /* the arrows start as the pointer */
    uint8_t  opl;             /* SETTINGS_OPL_*                        */
    uint8_t  opl_games;       /* entries in use in opl_game            */
    settings_opl_game_t opl_game[SETTINGS_OPL_GAMES];
} settings_t;

/* A line not used: its number from 1, where it is in the text (without
 * its line end), and why, a short literal. */
typedef struct {
    unsigned    line;
    size_t      at, len;
    const char *why;
} settings_error_t;

void settings_defaults(settings_t *s);

/* Apply the text's lines over what is in `s`. Returns the number of
 * lines not used; the first `max_errors` of them are described in
 * `errors`. */
unsigned settings_parse(settings_t *s, const char *text, size_t len,
                        settings_error_t *errors, unsigned max_errors);

/* The OPL emulator for the game in /quest/<folder>: its own, if it has
 * one, otherwise the default. The folder in any case. */
unsigned settings_opl_for(const settings_t *s, const char *folder);

/* Upstream's opl_driver for a SETTINGS_OPL_*: "db" or "mame". */
const char *settings_opl_driver(unsigned opl);

/* The mixer's gain for a volume, upstream's 0-255 with 128 for 100 %. */
int settings_gain(unsigned volume);

#ifndef PICOCALC_QUEST_HOST
/* The settings in use, from settings_load(). */
extern settings_t g_settings;

/* Core 0, once the card is mounted: read SETTINGS_PATH, if it is there,
 * into g_settings, logging every line not used. */
void settings_load(void);

/* Core 0, as the selector first shows: name the first line not used, if
 * any, in the status band (§7.4). */
void settings_announce(void);
#endif

#endif /* PICOCALC_QUEST_SETTINGS_H */

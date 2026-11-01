/* test_settings.c — /quest/picocalc.ini (design.md §10.3, §15).
 *
 * The defaults; every key with good values and with bad; comments, case,
 * spacing, CRLF, a BOM and a last line with no end; a bad line named with
 * its number and text and the rest of the file applied; a bad value
 * leaving the setting as it was; the two buttons on one key; more bad
 * lines than there is room to describe; the volume's gain; and the OPL
 * emulator.
 */

#include <stdio.h>
#include <string.h>

#include "settings.h"

static int failures;

#define CHECK(cond, ...)                                  \
    do {                                                  \
        if (!(cond)) {                                    \
            failures++;                                   \
            fprintf(stderr, "FAIL line %d: ", __LINE__);  \
            fprintf(stderr, __VA_ARGS__);                 \
            fputc('\n', stderr);                          \
        }                                                 \
    } while (0)

/* The engine numbers of picocalc_input.h. */
enum { SCUMM = 1, SCI, KYRA, GOB, AGI };

static settings_error_t errs[4];

static unsigned parse(settings_t *s, const char *text) {
    settings_defaults(s);
    memset(errs, 0, sizeof errs);
    return settings_parse(s, text, strlen(text), errs, 4);
}

/* The settings, as a string, to compare whole. */
static const char *show(const settings_t *s) {
    static char buf[96];
    snprintf(buf, sizeof buf, "%c%c %u %u %d%d%d%d%d", s->left_button, s->right_button,
             s->pointer_speed, s->volume, s->pointer_mode[SCUMM], s->pointer_mode[SCI],
             s->pointer_mode[KYRA], s->pointer_mode[GOB], s->pointer_mode[AGI]);
    return buf;
}

#define DEFAULTS "][ 100 100 11110"

static void expect(const char *text, unsigned bad, const char *want) {
    settings_t s;
    unsigned got = parse(&s, text);
    CHECK(got == bad, "\"%s\": %u bad line(s), want %u", text, got, bad);
    CHECK(strcmp(show(&s), want) == 0, "\"%s\": got %s, want %s", text, show(&s), want);
}

/* The first error's line, why, and text. */
static void expect_error(const char *text, unsigned index, unsigned line, const char *why,
                         const char *line_text) {
    settings_t s;
    parse(&s, text);
    const settings_error_t *e = &errs[index];
    CHECK(e->line == line, "\"%s\": error %u on line %u, want %u", text, index, e->line, line);
    CHECK(e->why && strcmp(e->why, why) == 0, "\"%s\": error %u why \"%s\", want \"%s\"", text,
          index, e->why ? e->why : "(null)", why);
    CHECK(e->len == strlen(line_text) && memcmp(text + e->at, line_text, e->len) == 0,
          "\"%s\": error %u text \"%.*s\", want \"%s\"", text, index, (int)e->len, text + e->at,
          line_text);
}

int main(void) {
    settings_t s;
    settings_defaults(&s);
    CHECK(strcmp(show(&s), DEFAULTS) == 0, "defaults %s", show(&s));

    /* Nothing, comments and blank lines change nothing. */
    expect("", 0, DEFAULTS);
    expect("\n\n   \n\t\n", 0, DEFAULTS);
    expect("# a comment\n; another\n   # indented\n", 0, DEFAULTS);

    /* Every key. */
    expect("left_button = a\nright_button = b\n", 0, "ab 100 100 11110");
    expect("pointer_speed = 25\n", 0, "][ 25 100 11110");
    expect("pointer_speed = 400\n", 0, "][ 400 100 11110");
    expect("volume = 0\n", 0, "][ 100 0 11110");
    expect("volume = 200\n", 0, "][ 100 200 11110");
    expect("scumm_mode = keys\n", 0, "][ 100 100 01110");
    expect("sci_mode = keys\n", 0, "][ 100 100 10110");
    expect("kyra_mode = keys\n", 0, "][ 100 100 11010");
    expect("gob_mode = keys\n", 0, "][ 100 100 11100");
    expect("agi_mode = pointer\n", 0, "][ 100 100 11111");

    /* Case, spacing, line ends, a BOM, no last line end. */
    expect("LEFT_BUTTON=Q", 0, "q[ 100 100 11110");
    expect("  Volume\t=\t50  \r\nAGI_MODE = POINTER\r\n", 0, "][ 100 50 11111");
    expect("\xEF\xBB\xBFvolume = 75\n", 0, "][ 100 75 11110");
    /* Comments after a value, and keys that are comment characters. */
    expect("volume = 80   # quieter\n", 0, "][ 100 80 11110");
    expect("volume = 80 ; quieter\n", 0, "][ 100 80 11110");
    expect("left_button = #\nright_button = ;\n", 0, "#; 100 100 11110");
    expect("left_button = #  # the hash key\n", 0, "#[ 100 100 11110");
    expect("left_button = # the hash key\n", 1, DEFAULTS);
    expect("left_button = =\n", 0, "=[ 100 100 11110");
    /* The later of two wins; swapping the buttons is not a clash. */
    expect("volume = 10\nvolume = 20\n", 0, "][ 100 20 11110");
    expect("left_button = [\nright_button = ]\n", 0, "[] 100 100 11110");

    /* Bad lines: named, the rest applied, the setting left as it was. */
    expect("volume = 50\nvolme = 3\npointer_speed = 200\n", 1, "][ 200 50 11110");
    expect_error("volume = 50\nvolme = 3\npointer_speed = 200\n", 0, 2, "no such setting",
                 "volme = 3");
    expect_error("a\r\nvolume\r\n", 0, 1, "no = in it", "a");
    expect_error("a\r\nvolume\r\n", 1, 2, "no = in it", "volume");
    expect_error("\n = 3\n", 0, 2, "no name before the =", " = 3");
    expect_error("volume =   \n", 0, 1, "no value after the =", "volume =   ");
    expect_error("volume = # none\n", 0, 1, "a number, 0 to 200", "volume = # none");
    expect("volume = 201\n", 1, DEFAULTS);
    expect("volume = -1\n", 1, DEFAULTS);
    expect("volume = 5x\n", 1, DEFAULTS);
    expect("volume = 99999999\n", 1, DEFAULTS);
    expect_error("volume = 201\n", 0, 1, "a number, 0 to 200", "volume = 201");
    expect("pointer_speed = 24\n", 1, DEFAULTS);
    expect("pointer_speed = 401\n", 1, DEFAULTS);
    expect_error("pointer_speed = 0\n", 0, 1, "a number, 25 to 400", "pointer_speed = 0");
    expect("scumm_mode = mouse\n", 1, DEFAULTS);
    expect_error("scumm_mode = mouse\n", 0, 1, "pointer or keys", "scumm_mode = mouse");
    expect("left_button = ab\n", 1, DEFAULTS);
    expect("left_button = \x7f\n", 1, DEFAULTS);
    expect_error("left_button = ab\n", 0, 1, "one printable key", "left_button = ab");
    /* A value with a space in it is not a comment's start. */
    expect("volume = 5 0\n", 1, DEFAULTS);

    /* The buttons on one key: the later line is not used. */
    expect("left_button = [\n", 1, DEFAULTS);
    expect_error("left_button = [\n", 0, 1, "the other button's key", "left_button = [");
    expect("left_button = x\nright_button = X\n", 1, "x[ 100 100 11110");
    expect_error("left_button = x\nright_button = X\n", 0, 2, "the other button's key",
                 "right_button = X");
    expect("right_button = x\nleft_button = x\n", 1, "]x 100 100 11110");
    /* Right on left's default, left never set: right goes back. */
    expect("right_button = ]\n", 1, DEFAULTS);
    /* Both on [ : right's default is [ too, so it takes left's. */
    expect("left_button = [\nright_button = [\n", 1, "[] 100 100 11110");

    /* More bad lines than room: all counted, the first described. */
    {
        const char *text = "a\nb\nc\nd\ne\nf\nvolume = 1\n";
        unsigned n = parse(&s, text);
        CHECK(n == 6, "six bad lines, got %u", n);
        CHECK(errs[3].line == 4, "the fourth error on line 4, got %u", errs[3].line);
        CHECK(s.volume == 1, "the good line after them applied, volume %u", s.volume);
    }

    /* The OPL emulator: MAME's by default, either by name in any case,
     * a bad name leaving it as it was. */
    {
        settings_t s;
        CHECK(parse(&s, "") == 0 && s.opl == SETTINGS_OPL_MAME, "opl default %u", s.opl);
        CHECK(parse(&s, "opl = dosbox\n") == 0 && s.opl == SETTINGS_OPL_DOSBOX, "opl dosbox %u",
              s.opl);
        CHECK(parse(&s, "OPL = DOSBox # lighter\n") == 0 && s.opl == SETTINGS_OPL_DOSBOX,
              "opl DOSBox %u", s.opl);
        CHECK(parse(&s, "opl = dosbox\nopl = mame\n") == 0 && s.opl == SETTINGS_OPL_MAME,
              "opl twice %u", s.opl);
        CHECK(parse(&s, "opl = dosbox\nopl = nuked\n") == 1 && s.opl == SETTINGS_OPL_DOSBOX,
              "opl bad kept %u", s.opl);
        CHECK(strcmp(settings_opl_driver(SETTINGS_OPL_DOSBOX), "db") == 0, "driver db");
        CHECK(strcmp(settings_opl_driver(SETTINGS_OPL_MAME), "mame") == 0, "driver mame");
    }
    /* A game's own: by folder in any case, the default for the rest; a
     * later line for the same folder replaces it; bad ones not used. */
    {
        settings_t s;
        CHECK(parse(&s, "opl.dott = dosbox\n") == 0 && s.opl_games == 1, "opl.dott");
        CHECK(settings_opl_for(&s, "dott") == SETTINGS_OPL_DOSBOX, "dott dosbox");
        CHECK(settings_opl_for(&s, "DOTT") == SETTINGS_OPL_DOSBOX, "DOTT dosbox");
        CHECK(settings_opl_for(&s, "atlantis") == SETTINGS_OPL_MAME, "atlantis default");
        CHECK(settings_opl_for(&s, "dot") == SETTINGS_OPL_MAME, "a prefix is not the folder");
        CHECK(parse(&s, "opl = dosbox\nOPL.Atlantis = MAME\n") == 0, "two lines");
        CHECK(settings_opl_for(&s, "atlantis") == SETTINGS_OPL_MAME, "atlantis mame");
        CHECK(settings_opl_for(&s, "kyr1") == SETTINGS_OPL_DOSBOX, "kyr1 the default");
        CHECK(parse(&s, "opl.dott = dosbox\nopl.DOTT = mame\n") == 0 && s.opl_games == 1 &&
              settings_opl_for(&s, "dott") == SETTINGS_OPL_MAME, "replaced, one entry");
        CHECK(parse(&s, "opl.dott = nuked\n") == 1 && s.opl_games == 0, "bad value not used");
        CHECK(parse(&s, "opl. = dosbox\n") == 1, "no folder");
        CHECK(parse(&s, "opl.a/b = dosbox\n") == 1, "a path is not a folder");
        CHECK(parse(&s, "opl.abcdefghijklmnopqrstuvwx = dosbox\n") == 1, "24 characters");
        CHECK(parse(&s, "opl.abcdefghijklmnopqrstuvw = dosbox\n") == 0 &&
              settings_opl_for(&s, "abcdefghijklmnopqrstuvw") == SETTINGS_OPL_DOSBOX, "23 characters");
        char text[17 * 20 + 1] = "";
        for (int i = 0; i < 17; i++) {
            char line[20];
            snprintf(line, sizeof line, "opl.g%02d = dosbox\n", i);
            strcat(text, line);
        }
        CHECK(parse(&s, text) == 1 && s.opl_games == SETTINGS_OPL_GAMES, "16 games, the 17th not used");
        CHECK(settings_opl_for(&s, "g15") == SETTINGS_OPL_DOSBOX && settings_opl_for(&s, "g16") == SETTINGS_OPL_MAME,
              "the 17th has the default");
    }
    expect_error("opl.dott = db\n", 0, 1, "dosbox or mame", "opl.dott = db");
    expect("opl = mame\n", 0, DEFAULTS);
    expect_error("opl = db\n", 0, 1, "dosbox or mame", "opl = db");

    /* The volume to upstream's gain, 128 at 100 %. */
    CHECK(settings_gain(0) == 0, "gain(0) = %d", settings_gain(0));
    CHECK(settings_gain(100) == 128, "gain(100) = %d", settings_gain(100));
    CHECK(settings_gain(50) == 64, "gain(50) = %d", settings_gain(50));
    CHECK(settings_gain(199) == 255, "gain(199) = %d", settings_gain(199));
    CHECK(settings_gain(200) == 255, "gain(200) = %d", settings_gain(200));

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("test_settings: ok\n");
    return 0;
}

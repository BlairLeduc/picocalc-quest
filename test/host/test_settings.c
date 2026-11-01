/* test_settings.c — /quest/picocalc.ini (design.md §10.3, §15).
 *
 * The defaults; every key with good values and with bad; comments, case,
 * spacing, CRLF, a BOM and a last line with no end; a bad line named with
 * its number and text and the rest of the file applied; a bad value
 * leaving the setting as it was; the two buttons on one key; more bad
 * lines than there is room to describe; the OPL emulator; subtitles;
 * and the volumes, talkspeed and tempo.
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
             s->pointer_speed, s->number[SETTINGS_MUSIC_VOLUME], s->pointer_mode[SCUMM], s->pointer_mode[SCI],
             s->pointer_mode[KYRA], s->pointer_mode[GOB], s->pointer_mode[AGI]);
    return buf;
}

#define DEFAULTS "][ 100 192 11110"

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
    expect("left_button = a\nright_button = b\n", 0, "ab 100 192 11110");
    expect("pointer_speed = 25\n", 0, "][ 25 192 11110");
    expect("pointer_speed = 400\n", 0, "][ 400 192 11110");
    expect("music_volume = 0\n", 0, "][ 100 0 11110");
    expect("music_volume = 255\n", 0, "][ 100 255 11110");
    expect("scumm_mode = keys\n", 0, "][ 100 192 01110");
    expect("sci_mode = keys\n", 0, "][ 100 192 10110");
    expect("kyra_mode = keys\n", 0, "][ 100 192 11010");
    expect("gob_mode = keys\n", 0, "][ 100 192 11100");
    expect("agi_mode = pointer\n", 0, "][ 100 192 11111");

    /* Case, spacing, line ends, a BOM, no last line end. */
    expect("LEFT_BUTTON=Q", 0, "q[ 100 192 11110");
    expect("  Music_Volume\t=\t50  \r\nAGI_MODE = POINTER\r\n", 0, "][ 100 50 11111");
    expect("\xEF\xBB\xBFmusic_volume = 75\n", 0, "][ 100 75 11110");
    /* Comments after a value, and keys that are comment characters. */
    expect("music_volume = 80   # quieter\n", 0, "][ 100 80 11110");
    expect("music_volume = 80 ; quieter\n", 0, "][ 100 80 11110");
    expect("left_button = #\nright_button = ;\n", 0, "#; 100 192 11110");
    expect("left_button = #  # the hash key\n", 0, "#[ 100 192 11110");
    expect("left_button = # the hash key\n", 1, DEFAULTS);
    expect("left_button = =\n", 0, "=[ 100 192 11110");
    /* The later of two wins; swapping the buttons is not a clash. */
    expect("music_volume = 10\nmusic_volume = 20\n", 0, "][ 100 20 11110");
    expect("left_button = [\nright_button = ]\n", 0, "[] 100 192 11110");

    /* Bad lines: named, the rest applied, the setting left as it was. */
    expect("music_volume = 50\nvolme = 3\npointer_speed = 200\n", 1, "][ 200 50 11110");
    expect_error("music_volume = 50\nvolme = 3\npointer_speed = 200\n", 0, 2, "no such setting",
                 "volme = 3");
    expect_error("a\r\nvolume\r\n", 0, 1, "no = in it", "a");
    expect_error("a\r\nvolume\r\n", 1, 2, "no = in it", "volume");
    expect_error("\n = 3\n", 0, 2, "no name before the =", " = 3");
    expect_error("music_volume =   \n", 0, 1, "no value after the =", "music_volume =   ");
    expect_error("music_volume = # none\n", 0, 1, "a number, 0 to 255", "music_volume = # none");
    expect("music_volume = 256\n", 1, DEFAULTS);
    expect("music_volume = -1\n", 1, DEFAULTS);
    expect("music_volume = 5x\n", 1, DEFAULTS);
    expect("music_volume = 99999999\n", 1, DEFAULTS);
    expect_error("music_volume = 256\n", 0, 1, "a number, 0 to 255", "music_volume = 256");
    expect("pointer_speed = 24\n", 1, DEFAULTS);
    expect("pointer_speed = 401\n", 1, DEFAULTS);
    expect_error("pointer_speed = 0\n", 0, 1, "a number, 25 to 400", "pointer_speed = 0");
    expect("scumm_mode = mouse\n", 1, DEFAULTS);
    expect_error("scumm_mode = mouse\n", 0, 1, "pointer or keys", "scumm_mode = mouse");
    expect("left_button = ab\n", 1, DEFAULTS);
    expect("left_button = \x7f\n", 1, DEFAULTS);
    expect_error("left_button = ab\n", 0, 1, "one printable key", "left_button = ab");
    /* A value with a space in it is not a comment's start. */
    expect("music_volume = 5 0\n", 1, DEFAULTS);

    /* The buttons on one key: the later line is not used. */
    expect("left_button = [\n", 1, DEFAULTS);
    expect_error("left_button = [\n", 0, 1, "the other button's key", "left_button = [");
    expect("left_button = x\nright_button = X\n", 1, "x[ 100 192 11110");
    expect_error("left_button = x\nright_button = X\n", 0, 2, "the other button's key",
                 "right_button = X");
    expect("right_button = x\nleft_button = x\n", 1, "]x 100 192 11110");
    /* Right on left's default, left never set: right goes back. */
    expect("right_button = ]\n", 1, DEFAULTS);
    /* Both on [ : right's default is [ too, so it takes left's. */
    expect("left_button = [\nright_button = [\n", 1, "[] 100 192 11110");

    /* More bad lines than room: all counted, the first described. */
    {
        const char *text = "a\nb\nc\nd\ne\nf\nmusic_volume = 1\n";
        unsigned n = parse(&s, text);
        CHECK(n == 6, "six bad lines, got %u", n);
        CHECK(errs[3].line == 4, "the fourth error on line 4, got %u", errs[3].line);
        CHECK(s.number[SETTINGS_MUSIC_VOLUME] == 1, "the good line after them applied, music %u", s.number[SETTINGS_MUSIC_VOLUME]);
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
        CHECK(parse(&s, "opl.dott = dosbox\n") == 0 && s.games == 1, "opl.dott");
        CHECK(settings_opl_for(&s, "dott") == SETTINGS_OPL_DOSBOX, "dott dosbox");
        CHECK(settings_opl_for(&s, "DOTT") == SETTINGS_OPL_DOSBOX, "DOTT dosbox");
        CHECK(settings_opl_for(&s, "atlantis") == SETTINGS_OPL_MAME, "atlantis default");
        CHECK(settings_opl_for(&s, "dot") == SETTINGS_OPL_MAME, "a prefix is not the folder");
        CHECK(parse(&s, "opl = dosbox\nOPL.Atlantis = MAME\n") == 0, "two lines");
        CHECK(settings_opl_for(&s, "atlantis") == SETTINGS_OPL_MAME, "atlantis mame");
        CHECK(settings_opl_for(&s, "kyr1") == SETTINGS_OPL_DOSBOX, "kyr1 the default");
        CHECK(parse(&s, "opl.dott = dosbox\nopl.DOTT = mame\n") == 0 && s.games == 1 &&
              settings_opl_for(&s, "dott") == SETTINGS_OPL_MAME, "replaced, one entry");
        CHECK(parse(&s, "opl.dott = nuked\n") == 1 && s.games == 0, "bad value not used");
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
        CHECK(parse(&s, text) == 1 && s.games == SETTINGS_GAMES, "16 games, the 17th not used");
        CHECK(settings_opl_for(&s, "g15") == SETTINGS_OPL_DOSBOX && settings_opl_for(&s, "g16") == SETTINGS_OPL_MAME,
              "the 17th has the default");
    }
    expect_error("opl.dott = db\n", 0, 1, "dosbox or mame", "opl.dott = db");
    expect("opl = mame\n", 0, DEFAULTS);
    expect_error("opl = db\n", 0, 1, "dosbox or mame", "opl = db");

    /* Subtitles: on by default, as upstream's launchers had them; on or
     * off in any case; a game's own by folder, the default for the rest,
     * sharing the game's entry with its OPL emulator. */
    {
        settings_t s;
        CHECK(parse(&s, "") == 0 && s.subtitles, "subtitles default on");
        CHECK(parse(&s, "subtitles = off\n") == 0 && !s.subtitles, "subtitles off");
        CHECK(parse(&s, "Subtitles = OFF ; quiet\n") == 0 && !s.subtitles, "subtitles OFF");
        CHECK(parse(&s, "subtitles = off\nsubtitles = yes\n") == 1 && !s.subtitles,
              "subtitles bad kept");
        CHECK(parse(&s, "subtitles.dig = off\n") == 0 && s.games == 1, "subtitles.dig");
        CHECK(!settings_subtitles_for(&s, "dig") && !settings_subtitles_for(&s, "DIG"), "dig off");
        CHECK(settings_subtitles_for(&s, "ft"), "ft the default");
        CHECK(settings_opl_for(&s, "dig") == SETTINGS_OPL_MAME, "dig's opl the default");
        CHECK(parse(&s, "subtitles = off\nsubtitles.ft = on\n") == 0 &&
              settings_subtitles_for(&s, "ft") && !settings_subtitles_for(&s, "dott"),
              "one game on, the rest off");
        CHECK(parse(&s, "opl.dott = dosbox\nsubtitles.DOTT = off\n") == 0 && s.games == 1 &&
              settings_opl_for(&s, "dott") == SETTINGS_OPL_DOSBOX &&
              !settings_subtitles_for(&s, "dott"), "one entry, both settings");
        CHECK(parse(&s, "subtitles.dig = no\n") == 1 && s.games == 0, "bad value not used");
        CHECK(parse(&s, "subtitles. = off\n") == 1, "no folder");
    }
    expect_error("subtitles.dig = no\n", 0, 1, "on or off", "subtitles.dig = no");
    expect("subtitles = on\n", 0, DEFAULTS);

    /* The numbers: ScummVM's defaults, ranges and names, a game's own by
     * folder, sharing the game's entry; a bad one leaves the setting. */
    {
        settings_t s;
        parse(&s, "");
        CHECK(s.number[SETTINGS_MUSIC_VOLUME] == 192 && s.number[SETTINGS_SFX_VOLUME] == 192 &&
              s.number[SETTINGS_SPEECH_VOLUME] == 192, "volumes' default");
        CHECK(s.number[SETTINGS_TALKSPEED] == 60 && s.number[SETTINGS_TEMPO] == 100, "scumm's default");
        CHECK(parse(&s, "music_volume = 1\nsfx_volume = 2\nspeech_volume = 3\n"
                        "talkspeed = 4\ntempo = 50\n") == 0 &&
              s.number[SETTINGS_MUSIC_VOLUME] == 1 && s.number[SETTINGS_SFX_VOLUME] == 2 &&
              s.number[SETTINGS_SPEECH_VOLUME] == 3 && s.number[SETTINGS_TALKSPEED] == 4 &&
              s.number[SETTINGS_TEMPO] == 50, "each its own");
        CHECK(parse(&s, "speech_volume = 0\nsfx_volume = 255\ntalkspeed = 255\ntempo = 200\n") == 0,
              "the ends of the ranges");
        CHECK(parse(&s, "tempo = 49\ntempo = 201\ntalkspeed = 256\nsfx_volume = 256\n") == 4 &&
              s.number[SETTINGS_TEMPO] == 100 && s.number[SETTINGS_TALKSPEED] == 60, "out of range");
        /* The master volume is gone, and is named rather than ignored. */
        CHECK(parse(&s, "volume = 80\n") == 1, "volume is no setting");
        CHECK(parse(&s, "tempos = 100\ntempo. = 100\nmusic_volume_x = 1\n") == 3, "near names");

        CHECK(parse(&s, "music_volume = 100\nMusic_Volume.KYR1 = 255\ntempo.dott = 150\n"
                        "talkspeed.dig = 200\nopl.dott = dosbox\n") == 0 && s.games == 3, "three games");
        CHECK(settings_number_for(&s, "kyr1", SETTINGS_MUSIC_VOLUME) == 255, "kyr1's music");
        CHECK(settings_number_for(&s, "KYR1", SETTINGS_SFX_VOLUME) == 192, "kyr1's sfx the default");
        CHECK(settings_number_for(&s, "ft", SETTINGS_MUSIC_VOLUME) == 100, "ft's music the default");
        CHECK(settings_number_for(&s, "dott", SETTINGS_TEMPO) == 150, "dott's tempo");
        CHECK(settings_number_for(&s, "dig", SETTINGS_TALKSPEED) == 200, "dig's talkspeed");
        CHECK(settings_number_for(&s, "dig", SETTINGS_TEMPO) == 100, "dig's tempo the default");
        CHECK(settings_opl_for(&s, "dott") == SETTINGS_OPL_DOSBOX, "one entry with its opl");
        CHECK(parse(&s, "sfx_volume.ft = 0\n") == 0 && settings_number_for(&s, "ft", SETTINGS_SFX_VOLUME) == 0,
              "a game's own 0 is not unset");
        CHECK(parse(&s, "tempo.dott = 300\n") == 1 && s.games == 0, "bad value, no entry");
        CHECK(parse(&s, "tempo.a/b = 100\n") == 1, "a path is not a folder");
        CHECK(strcmp(settings_number_name(SETTINGS_TEMPO), "tempo") == 0, "a name");
    }
    expect_error("tempo = 300\n", 0, 1, "a number, 50 to 200", "tempo = 300");
    expect_error("talkspeed.dig = x\n", 0, 1, "a number, 0 to 255", "talkspeed.dig = x");
    expect_error("volume = 80\n", 0, 1, "no such setting", "volume = 80");

    if (failures) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    printf("test_settings: ok\n");
    return 0;
}

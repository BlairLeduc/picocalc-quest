/* settings.c — /quest/picocalc.ini, this port's settings (design.md
 * §10.3). */

#include "settings.h"

#include <string.h>

#ifndef PICOCALC_QUEST_HOST
#include <stdio.h>

#include "frank_quest_fs.h"
#include "status.h"
#endif

/* The engine numbers of picocalc_input.h, which is not pure. */
enum { ENGINE_SCUMM = 1, ENGINE_SCI, ENGINE_KYRA, ENGINE_GOB, ENGINE_AGI };

void settings_defaults(settings_t *s) {
    memset(s, 0, sizeof *s);
    /* D8, as play-tested in M6. */
    s->left_button = ']';
    s->right_button = '[';
    s->pointer_speed = 100;
    s->volume = 100;
    /* AGI's hero walks with the arrows; every other engine is pointed
     * and clicked. SCI0 takes both, and starts as a pointer (§9.3). */
    s->pointer_mode[ENGINE_SCUMM] = true;
    s->pointer_mode[ENGINE_SCI] = true;
    s->pointer_mode[ENGINE_KYRA] = true;
    s->pointer_mode[ENGINE_GOB] = true;
    s->pointer_mode[ENGINE_AGI] = false;
}

int settings_gain(unsigned volume) {
    unsigned gain = (volume * 128u + 50u) / 100u;
    return gain > 255u ? 255 : (int)gain;
}

static bool is_space(char c) {
    return c == ' ' || c == '\t';
}

static char lower(char c) {
    return c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c;
}

/* A word against a lower-case name, in any case. */
static bool same(const char *p, size_t n, const char *name) {
    if (strlen(name) != n) return false;
    for (size_t i = 0; i < n; i++) {
        if (lower(p[i]) != name[i]) return false;
    }
    return true;
}

/* A whole number in [lo, hi], digits only. */
static bool number(const char *p, size_t n, unsigned lo, unsigned hi, uint16_t *out) {
    if (n == 0 || n > 5) return false;
    unsigned v = 0;
    for (size_t i = 0; i < n; i++) {
        if (p[i] < '0' || p[i] > '9') return false;
        v = v * 10u + (unsigned)(p[i] - '0');
    }
    if (v < lo || v > hi) return false;
    *out = (uint16_t)v;
    return true;
}

static const char *button(const char *p, size_t n, uint8_t *out) {
    if (n != 1 || p[0] <= ' ' || p[0] > '~') return "one printable key";
    *out = (uint8_t)lower(p[0]);
    return NULL;
}

static const char *mode(const char *p, size_t n, bool *pointer) {
    if (same(p, n, "pointer")) *pointer = true;
    else if (same(p, n, "keys")) *pointer = false;
    else return "pointer or keys";
    return NULL;
}

static const struct {
    const char *name;
    int         engine;
} s_modes[] = {
    { "agi_mode", ENGINE_AGI },   { "sci_mode", ENGINE_SCI }, { "scumm_mode", ENGINE_SCUMM },
    { "kyra_mode", ENGINE_KYRA }, { "gob_mode", ENGINE_GOB },
};

/* One key and value onto s: NULL if used, otherwise why not. `which` is
 * 1 or 2 for a button, so the two can be checked against each other at
 * the end. */
static const char *apply(settings_t *s, const char *k, size_t kn, const char *v, size_t vn,
                         int *which) {
    *which = 0;
    if (same(k, kn, "left_button")) {
        *which = 1;
        return button(v, vn, &s->left_button);
    }
    if (same(k, kn, "right_button")) {
        *which = 2;
        return button(v, vn, &s->right_button);
    }
    if (same(k, kn, "pointer_speed")) {
        return number(v, vn, SETTINGS_SPEED_MIN, SETTINGS_SPEED_MAX, &s->pointer_speed)
                   ? NULL : "a number, 25 to 400";
    }
    if (same(k, kn, "volume")) {
        return number(v, vn, 0, SETTINGS_VOLUME_MAX, &s->volume) ? NULL : "a number, 0 to 200";
    }
    for (size_t i = 0; i < sizeof s_modes / sizeof s_modes[0]; i++) {
        if (same(k, kn, s_modes[i].name)) return mode(v, vn, &s->pointer_mode[s_modes[i].engine]);
    }
    return "no such setting";
}

static void bad(unsigned *count, settings_error_t *errors, unsigned max, unsigned line,
                size_t at, size_t len, const char *why) {
    if (*count < max) errors[*count] = (settings_error_t){ line, at, len, why };
    (*count)++;
}

unsigned settings_parse(settings_t *s, const char *text, size_t len,
                        settings_error_t *errors, unsigned max_errors) {
    unsigned count = 0, line = 0;
    /* The last line that set each button, to name if they end up the
     * same key. */
    unsigned button_line[3] = { 0 };
    size_t button_at[3] = { 0 }, button_len[3] = { 0 };

    size_t pos = 0;
    if (len >= 3 && memcmp(text, "\xEF\xBB\xBF", 3) == 0) pos = 3;   /* a UTF-8 BOM */
    while (pos < len) {
        line++;
        size_t at = pos, end = pos;
        while (end < len && text[end] != '\n') end++;
        pos = end + 1;
        size_t n = end - at;
        if (n > 0 && text[at + n - 1] == '\r') n--;

        const char *p = text + at, *e = p + n;
        while (p < e && is_space(*p)) p++;
        if (p == e || *p == '#' || *p == ';') continue;

        const char *eq = memchr(p, '=', (size_t)(e - p));
        if (eq == NULL) {
            bad(&count, errors, max_errors, line, at, n, "no = in it");
            continue;
        }
        const char *k = p, *ke = eq;
        while (ke > k && is_space(ke[-1])) ke--;
        const char *v = eq + 1;
        while (v < e && is_space(*v)) v++;
        /* A comment: # or ; after a space, within the value. */
        const char *ve = v;
        while (ve < e && !((*ve == '#' || *ve == ';') && ve > v && is_space(ve[-1]))) ve++;
        while (ve > v && is_space(ve[-1])) ve--;
        if (ke == k) {
            bad(&count, errors, max_errors, line, at, n, "no name before the =");
            continue;
        }
        if (ve == v) {
            bad(&count, errors, max_errors, line, at, n, "no value after the =");
            continue;
        }

        /* Applied to a copy, so a bad value leaves the setting as it was. */
        settings_t t = *s;
        int which;
        const char *why = apply(&t, k, (size_t)(ke - k), v, (size_t)(ve - v), &which);
        if (why != NULL) {
            bad(&count, errors, max_errors, line, at, n, why);
            continue;
        }
        *s = t;
        if (which) {
            button_line[which] = line;
            button_at[which] = at;
            button_len[which] = n;
        }
    }

    /* Both buttons on one key: the later line is not used, and that
     * button keeps its default, or if that is the other's key too, the
     * other's default. */
    if (s->left_button == s->right_button) {
        settings_t d;
        settings_defaults(&d);
        int later = button_line[1] > button_line[2] ? 1 : 2;
        if (later == 1) s->left_button = d.left_button != s->right_button ? d.left_button : d.right_button;
        else s->right_button = d.right_button != s->left_button ? d.right_button : d.left_button;
        if (button_line[later] != 0) {
            bad(&count, errors, max_errors, button_line[later], button_at[later], button_len[later],
                "the other button's key");
        }
    }
    return count;
}

#ifndef PICOCALC_QUEST_HOST
/* Zero until settings_load(), which main() calls before anything reads
 * it, so the defaults stay in one place. */
settings_t g_settings;

/* The file is small; longer is cut at its last whole line. */
#define SETTINGS_MAX 4096u
#define ERRORS_MAX   8u

static char s_text[SETTINGS_MAX];
static unsigned s_bad;
static settings_error_t s_first;
static char s_title[64];

void settings_load(void) {
    settings_defaults(&g_settings);
    CabalFile *f = cabal_file_open(SETTINGS_PATH);
    if (f == NULL) {
        printf("  settings     : no %s; the defaults\n", SETTINGS_PATH);
        return;
    }
    uint32_t size = cabal_file_size(f);
    int32_t got = cabal_file_read(f, s_text, SETTINGS_MAX);
    cabal_file_close(f);
    if (got < 0) got = 0;
    size_t len = (size_t)got;
    bool cut = size > SETTINGS_MAX;
    if (cut) {
        while (len > 0 && s_text[len - 1] != '\n') len--;
    }

    settings_error_t errors[ERRORS_MAX];
    s_bad = settings_parse(&g_settings, s_text, len, errors, ERRORS_MAX);
    for (unsigned i = 0; i < s_bad && i < ERRORS_MAX; i++) {
        printf("  settings     : %s line %u not used, %s: \"%.*s\"\n", SETTINGS_PATH,
               errors[i].line, errors[i].why, (int)errors[i].len, s_text + errors[i].at);
    }
    if (s_bad > ERRORS_MAX) printf("  settings     : and %u more\n", s_bad - ERRORS_MAX);
    if (s_bad > 0) s_first = errors[0];
    if (cut) {
        printf("  settings     : %s is %lu bytes; only the first %u read\n", SETTINGS_PATH,
               (unsigned long)size, (unsigned)len);
        if (s_bad++ == 0) s_first = (settings_error_t){ 0, 0, 0, "the file is over 4 KB" };
    }

    const settings_t *s = &g_settings;
    printf("  settings     : buttons %c left, %c right; pointer %u%%; volume %u%%; "
           "pointer mode in scumm %d sci %d kyra %d gob %d agi %d\n",
           s->left_button, s->right_button, s->pointer_speed, s->volume,
           s->pointer_mode[ENGINE_SCUMM], s->pointer_mode[ENGINE_SCI],
           s->pointer_mode[ENGINE_KYRA], s->pointer_mode[ENGINE_GOB],
           s->pointer_mode[ENGINE_AGI]);
}

void settings_announce(void) {
    static bool done;
    if (done || s_bad == 0) return;
    done = true;
    if (s_first.line == 0) {
        snprintf(s_title, sizeof s_title, "picocalc.ini: not all read");
    } else if (s_bad == 1) {
        snprintf(s_title, sizeof s_title, "picocalc.ini: line %u not used", s_first.line);
    } else {
        snprintf(s_title, sizeof s_title, "picocalc.ini: line %u +%u not used", s_first.line,
                 s_bad - 1u);
    }
    status_show_for(s_title, s_first.why, STATUS_LONG_US);
}
#endif

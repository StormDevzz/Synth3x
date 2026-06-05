#include "../fm.h"

typedef struct {
    const char *bin;
    const char *de;
    const char *fmt;
} TermEntry;

static const TermEntry TERMS[] = {
    {"gnome-terminal",  "gnome",   "--working-directory='%s'"},
    {"konsole",         "kde",     "--workdir '%s'"},
    {"xfce4-terminal",  "xfce",    "--working-directory='%s'"},
    {"mate-terminal",   "mate",    "--working-directory='%s'"},
    {"lxterminal",      "lxde",    "--working-directory='%s'"},
    {"qterminal",       "lxqt",    "--workdir '%s'"},
    {"terminator",      NULL,      "--working-directory='%s'"},
    {"tilix",           NULL,      "--working-directory='%s'"},
    {"deepin-terminal", NULL,      "--work-directory '%s'"},
    {"alacritty",       NULL,      "--working-directory '%s'"},
    {"kitty",           NULL,      "--directory '%s'"},
    {"foot",            NULL,      "--working-directory '%s'"},
    {"urxvt",           NULL,      "-cd '%s'"},
    {"st",              NULL,      "-e sh -c 'cd \"%s\" && exec bash'"},
    {"xterm",           NULL,      "-e sh -c 'cd \"%s\" && exec bash'"},
    {NULL, NULL, NULL}
};

static int find_terminal(const char *preferred) {
    int n = sizeof(TERMS) / sizeof(TERMS[0]) - 1;
    if (preferred) {
        for (int i = 0; i < n; i++) {
            if (g_strcmp0(preferred, TERMS[i].bin) == 0) {
                char path[4096];
                snprintf(path, sizeof(path), "/usr/bin/%s", TERMS[i].bin);
                if (access(path, X_OK) == 0) return i;
            }
        }
    }
    return -1;
}

static int match_desktop(const char *desktop) {
    if (!desktop || !desktop[0]) return -1;
    int n = sizeof(TERMS) / sizeof(TERMS[0]) - 1;
    char lower[64];
    int si = 0;
    for (int i = 0; desktop[i] && si < 63; i++)
        lower[si++] = g_ascii_tolower(desktop[i]);
    lower[si] = '\0';

    for (int i = 0; i < n; i++) {
        if (TERMS[i].de && strstr(lower, TERMS[i].de)) {
            char path[4096];
            snprintf(path, sizeof(path), "/usr/bin/%s", TERMS[i].bin);
            if (access(path, X_OK) == 0) return i;
        }
    }
    return -1;
}

static int find_any_terminal(void) {
    int n = sizeof(TERMS) / sizeof(TERMS[0]) - 1;
    for (int i = 0; i < n; i++) {
        char path[4096];
        snprintf(path, sizeof(path), "/usr/bin/%s", TERMS[i].bin);
        if (access(path, X_OK) == 0) return i;
    }
    return -1;
}

static gboolean can_spawn_detached(const char *bin) {
    return strcmp(bin, "gnome-terminal") == 0 ||
           strcmp(bin, "konsole") == 0 ||
           strcmp(bin, "xfce4-terminal") == 0 ||
           strcmp(bin, "mate-terminal") == 0 ||
           strcmp(bin, "lxterminal") == 0 ||
           strcmp(bin, "qterminal") == 0 ||
           strcmp(bin, "terminator") == 0 ||
           strcmp(bin, "tilix") == 0 ||
           strcmp(bin, "deepin-terminal") == 0;
}

void open_terminal(void) {
    int idx = find_terminal(g_getenv("TERMINAL"));
    if (idx < 0) idx = match_desktop(g_getenv("XDG_CURRENT_DESKTOP"));
    if (idx < 0) idx = find_any_terminal();
    if (idx < 0) { update_status("no terminal found"); return; }

    char *quoted = g_shell_quote(state.cwd);
    char *full_cmd = g_strdup_printf("%s %s", TERMS[idx].bin, TERMS[idx].fmt);
    char *expanded = g_strdup_printf(full_cmd, quoted);
    g_free(full_cmd);
    g_free(quoted);
    g_strstrip(expanded);

    GError *err = NULL;
    if (g_spawn_command_line_async(expanded, &err)) {
        update_status("terminal opened: %s", TERMS[idx].bin);
        g_free(expanded);
        return;
    }

    gchar *fallback;
    if (can_spawn_detached(TERMS[idx].bin)) {
        fallback = g_strdup_printf("%s &", expanded);
    } else {
        fallback = g_strdup_printf("%s -e sh -c 'cd \"%s\" && exec bash' &",
                                   TERMS[idx].bin, state.cwd);
    }
    g_free(expanded);

    if (system(fallback) == 0) {
        update_status("terminal opened (fallback): %s", TERMS[idx].bin);
    } else {
        update_status("failed: %s", err ? err->message : "fallback also failed");
    }
    g_free(fallback);
    if (err) g_error_free(err);
}

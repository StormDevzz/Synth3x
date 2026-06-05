#define _GNU_SOURCE
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <fileman.h>

enum {
    COL_DIR       = 1,
    COL_EXE       = 2,
    COL_SEL       = 3,
    COL_TITLE     = 4,
    COL_STATUSBAR = 5,
    COL_HEADER    = 6,
};

void init_colors(void) {
    init_pair(COL_DIR,       COLOR_BLUE,   -1);
    init_pair(COL_EXE,       COLOR_GREEN,  -1);
    init_pair(COL_SEL,       COLOR_WHITE,  COLOR_BLUE);
    init_pair(COL_TITLE,     COLOR_YELLOW, COLOR_BLUE);
    init_pair(COL_STATUSBAR, COLOR_WHITE,  COLOR_BLUE);
    init_pair(COL_HEADER,    COLOR_CYAN,   -1);
}

char* fmt_size(off_t size, char *buf, size_t len) {
    if (size < 1024)
        snprintf(buf, len, "%4ld B", (long)size);
    else if (size < 1024 * 1024)
        snprintf(buf, len, "%4.0f K", (double)size / 1024);
    else if (size < 1024LL * 1024 * 1024)
        snprintf(buf, len, "%4.1f M", (double)size / (1024 * 1024));
    else
        snprintf(buf, len, "%4.1f G", (double)size / (1024LL * 1024 * 1024));
    return buf;
}

char* fmt_time(time_t t, char *buf, size_t len) {
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%d-%b-%Y %H:%M", tm);
    return buf;
}

static void draw_header(FileState *fs) {
    int w = COLS;
    char title[256];
    snprintf(title, sizeof(title), "  Synth3x FileMng  |  %s  ", fs->cwd);

    attron(A_BOLD | COLOR_PAIR(COL_TITLE));
    move(0, 0);
    int pad = w - (int)strlen(title);
    if (pad < 0) pad = 0;
    char *buf = malloc(w + 1);
    snprintf(buf, w + 1, "%.*s%*s", (int)strlen(title), title, pad, "");
    printw("%s", buf);
    free(buf);
    attroff(A_BOLD | COLOR_PAIR(COL_TITLE));
}

static void draw_columns(FileState *fs) {
    int w = COLS;

    attron(A_REVERSE | COLOR_PAIR(COL_HEADER));
    move(1, 0);

    char sort_ch = ' ';
    if (fs->sort_by == SORT_NAME) sort_ch = fs->sort_rev ? '^' : 'v';
    else if (fs->sort_by == SORT_SIZE) sort_ch = fs->sort_rev ? '^' : 'v';
    else if (fs->sort_by == SORT_TIME) sort_ch = fs->sort_rev ? '^' : 'v';

    int name_w = w - 56;
    if (name_w < 10) name_w = 10;
    printw(" %c %-*s %10s %12s  ", sort_ch, name_w, "Name", "Size", "Modified");
    for (int i = 0; i < w - (int)strlen("  Name          Size     Modified  "); i++)
        printw(" ");
    attroff(A_REVERSE | COLOR_PAIR(COL_HEADER));
}

static void draw_entry(FileEntry *e, int y, int w, int selected) {
    if (selected)
        attron(COLOR_PAIR(COL_SEL));
    else if (e->is_dir)
        attron(COLOR_PAIR(COL_DIR) | A_BOLD);
    else if (e->mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        attron(COLOR_PAIR(COL_EXE));

    move(y, 0);

    char type = e->is_dir ? 'd' : '-';
    char perm[11];
    snprintf(perm, sizeof(perm),
        "%c%c%c%c%c%c%c%c%c",
        e->mode & S_IRUSR ? 'r' : '-',
        e->mode & S_IWUSR ? 'w' : '-',
        e->mode & S_IXUSR ? 'x' : '-',
        e->mode & S_IRGRP ? 'r' : '-',
        e->mode & S_IWGRP ? 'w' : '-',
        e->mode & S_IXGRP ? 'x' : '-',
        e->mode & S_IROTH ? 'r' : '-',
        e->mode & S_IWOTH ? 'w' : '-',
        e->mode & S_IXOTH ? 'x' : '-');

    char sstr[16], tstr[32];
    fmt_size(e->size, sstr, sizeof(sstr));
    fmt_time(e->mtime, tstr, sizeof(tstr));

    int name_w = w - 56;
    if (name_w < 10) name_w = 10;
    printw(" %c%s %-*s %10s %12s  ", type, perm, name_w, e->name, sstr, tstr);

    if (selected)
        attroff(COLOR_PAIR(COL_SEL));
    else if (e->is_dir)
        attroff(COLOR_PAIR(COL_DIR) | A_BOLD);
    else if (e->mode & (S_IXUSR | S_IXGRP | S_IXOTH))
        attroff(COLOR_PAIR(COL_EXE));
}

void draw_ui(FileState *fs) {
    int h = LINES - 3;
    int w = COLS;

    draw_header(fs);
    draw_columns(fs);

    for (int i = 0; i < h; i++) {
        int idx = fs->top + i;
        if (idx < fs->count)
            draw_entry(&fs->entries[idx], i + 2, w, idx == fs->cursor);
        else {
            move(i + 2, 0);
            clrtoeol();
        }
    }

    attron(A_REVERSE | COLOR_PAIR(COL_STATUSBAR));
    move(LINES - 1, 0);
    char status[256];
    snprintf(status, sizeof(status),
        " [n]Name [s]Size [t]Time  [c]Copy [m]Move [d]Del [r]Ren [K]Mkdir [v]View [q]Quit");
    printw("%-*s", w, status);
    attroff(A_REVERSE | COLOR_PAIR(COL_STATUSBAR));

    move(LINES - 2, 0);
    clrtoeol();
    printw(" %d/%d", fs->cursor + 1, fs->count);
    // info area
    printw("  sort: %s", fs->sort_by == SORT_NAME ? "name" :
                         fs->sort_by == SORT_SIZE ? "size" : "time");

    refresh();
}

void draw_status(const char *msg) {
    attron(A_REVERSE | COLOR_PAIR(COL_STATUSBAR));
    move(LINES - 1, 0);
    clrtoeol();
    printw(" %-*s", COLS - 1, msg ? msg : "");
    attroff(A_REVERSE | COLOR_PAIR(COL_STATUSBAR));
    refresh();
}

void draw_error(const char *msg) {
    attron(A_REVERSE | COLOR_PAIR(COL_STATUSBAR));
    move(LINES - 1, 0);
    clrtoeol();
    printw(" ERROR: %-*s", COLS - 16, msg ? msg : "");
    attroff(A_REVERSE | COLOR_PAIR(COL_STATUSBAR));
    refresh();
    napms(1500);
}

#define _GNU_SOURCE
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "fileman.h"

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
    if (size < 1024)            snprintf(buf, len, "%4ld B", (long)size);
    else if (size < 1024*1024) snprintf(buf, len, "%4.0f K", (double)size/1024);
    else if (size < 1024LL*1024*1024)
        snprintf(buf, len, "%4.1f M", (double)size/(1024*1024));
    else
        snprintf(buf, len, "%4.1f G", (double)size/(1024LL*1024*1024));
    return buf;
}

char* fmt_time(time_t t, char *buf, size_t len) {
    struct tm *tm = localtime(&t);
    strftime(buf, len, "%d-%b-%Y %H:%M", tm);
    return buf;
}

static void draw_header(FileState *fs) {
    attron(A_BOLD | COLOR_PAIR(COL_TITLE));
    move(0, 0);
    int w = COLS;
    char title[256];
    snprintf(title, sizeof(title), "  Synth3x FileMng  |  %s  ", fs->cwd);
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
    switch (fs->sort_by) {
    case SORT_NAME: sort_ch = fs->sort_rev ? '^' : 'v'; break;
    case SORT_SIZE: sort_ch = fs->sort_rev ? '^' : 'v'; break;
    case SORT_TIME: sort_ch = fs->sort_rev ? '^' : 'v'; break;
    }
    printw(" %c %-*s %10s %12s  %s", sort_ch, w - 40, "Name", "Size", "Modified", "");
    for (int i = 0; i < w - (int)strlen("  Name          Size     Modified  "); i++) printw(" ");
    attroff(A_REVERSE | COLOR_PAIR(COL_HEADER));
}

static void draw_entry(FileEntry *e, int y, int w, int selected) {
    if (selected) {
        attron(COLOR_PAIR(COL_SEL));
    } else if (e->is_dir) {
        attron(COLOR_PAIR(COL_DIR) | A_BOLD);
    } else if (e->mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        attron(COLOR_PAIR(COL_EXE));
    }

    move(y, 0);

    char type = e->is_dir ? 'd' : '-';
    char perm[11];
    snprintf(perm, sizeof(perm), "%c%c%c%c%c%c%c%c%c",
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

    int max_name = w - 56;
    if (max_name < 10) max_name = 10;
    char name_disp[512];
    snprintf(name_disp, sizeof(name_disp), "%.*s", max_name, e->name);

    printw(" %c%s %-*s %10s %12s  %s", type, perm, max_name, name_disp, sstr, tstr, "");

    if (selected) {
        attroff(COLOR_PAIR(COL_SEL));
    } else if (e->is_dir) {
        attroff(COLOR_PAIR(COL_DIR) | A_BOLD);
    } else if (e->mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        attroff(COLOR_PAIR(COL_EXE));
    }
}

void draw_ui(FileState *fs) {
    int h = LINES - 3;
    int w = COLS;

    draw_header(fs);
    draw_columns(fs);

    for (int i = 0; i < h; i++) {
        int idx = fs->top + i;
        if (idx < fs->count) {
            draw_entry(&fs->entries[idx], i + 2, w, idx == fs->cursor);
        } else {
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
    printw(" %d/%d  %s", fs->cursor + 1, fs->count, "");

    for (int i = 0; i < w - 30; i++) printw(" ");
    printw("%s", "");
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

int confirm_dialog(const char *msg, const char *detail) {
    int h = 7, w = 60;
    int x = (COLS - w) / 2;
    int y = (LINES - h) / 2;

    WINDOW *win = newwin(h, w, y, x);
    keypad(win, TRUE);
    box(win, 0, 0);
    mvwprintw(win, 1, 2, " %s ", msg);
    if (detail) {
        int l = (int)strlen(detail);
        int max = w - 4;
        if (l > max) l = max;
        mvwprintw(win, 2, 2, " %.*s ", l, detail);
    }
    mvwprintw(win, h - 2, 2, " <y>es  <n>o  ");
    wrefresh(win);

    int result = 0;
    int ch;
    while (1) {
        ch = wgetch(win);
        if (ch == 'y' || ch == 'Y') { result = 1; break; }
        if (ch == 'n' || ch == 'N' || ch == 27) { result = 0; break; }
    }
    delwin(win);
    touchwin(stdscr);
    refresh();
    return result;
}

static char* input_dialog(const char *prompt, const char *initial) {
    int h = 5, w = 60;
    int x = (COLS - w) / 2;
    int y = (LINES - h) / 2;

    WINDOW *win = newwin(h, w, y, x);
    keypad(win, TRUE);
    box(win, 0, 0);
    mvwprintw(win, 1, 2, " %s ", prompt);
    wrefresh(win);

    char buf[512] = {0};
    if (initial) strncpy(buf, initial, sizeof(buf) - 1);

    echo();
    curs_set(1);
    mvwgetnstr(win, 2, 2, buf, sizeof(buf) - 1);
    noecho();
    curs_set(0);

    delwin(win);
    touchwin(stdscr);
    refresh();

    if (strlen(buf) == 0) return NULL;
    return strdup(buf);
}

int run_viewer(const char *path) {
    pid_t pid = fork();
    if (pid == 0) {
        def_prog_mode();
        endwin();
        execlp("less", "less", path, NULL);
        execlp("more", "more", path, NULL);
        fprintf(stderr, "no pager available\n");
        _exit(1);
    }
    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        reset_prog_mode();
        return 1;
    }
    return 0;
}

int do_copy(FileState *fs) {
    if (fs->count == 0) return 0;
    FileEntry *e = &fs->entries[fs->cursor];
    char *dest = input_dialog("Copy to:", e->name);
    if (!dest) return 0;

    char src[4096];
    snprintf(src, sizeof(src), "%s/%s", fs->cwd, e->name);
    char cmd[4608];
    snprintf(cmd, sizeof(cmd), "cp -r '%s' '%s'", src, dest);
    int r = system(cmd);
    free(dest);
    if (r == 0) draw_status("copied");
    else draw_error("copy failed");
    return r == 0;
}

int do_move(FileState *fs) {
    if (fs->count == 0) return 0;
    FileEntry *e = &fs->entries[fs->cursor];
    char *dest = input_dialog("Move to:", e->name);
    if (!dest) return 0;

    char src[4096];
    snprintf(src, sizeof(src), "%s/%s", fs->cwd, e->name);
    char cmd[4608];
    snprintf(cmd, sizeof(cmd), "mv '%s' '%s'", src, dest);
    int r = system(cmd);
    free(dest);
    if (r == 0) draw_status("moved");
    else draw_error("move failed");
    return r == 0;
}

int do_delete(FileState *fs) {
    if (fs->count == 0) return 0;
    FileEntry *e = &fs->entries[fs->cursor];
    char msg[64];
    snprintf(msg, sizeof(msg), "Delete %s?", e->name);
    if (!confirm_dialog(msg, e->is_dir ? "recursively" : NULL)) return 0;

    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", fs->cwd, e->name);
    char cmd[4608];
    if (e->is_dir)
        snprintf(cmd, sizeof(cmd), "rm -rf '%s'", path);
    else
        snprintf(cmd, sizeof(cmd), "rm '%s'", path);
    int r = system(cmd);
    if (r == 0) draw_status("deleted");
    else draw_error("delete failed");
    return r == 0;
}

int do_rename(FileState *fs) {
    if (fs->count == 0) return 0;
    FileEntry *e = &fs->entries[fs->cursor];
    char *newname = input_dialog("Rename to:", e->name);
    if (!newname) return 0;

    char old[4096], newp[4096];
    snprintf(old, sizeof(old), "%s/%s", fs->cwd, e->name);
    snprintf(newp, sizeof(newp), "%s/%s", fs->cwd, newname);
    int r = rename(old, newp);
    free(newname);
    if (r == 0) draw_status("renamed");
    else draw_error("rename failed");
    return r == 0;
}

int do_mkdir(FileState *fs) {
    char *name = input_dialog("Create directory:", NULL);
    if (!name) return 0;

    char path[4096];
    snprintf(path, sizeof(path), "%s/%s", fs->cwd, name);
    int r = mkdir(path, 0755);
    free(name);
    if (r == 0) draw_status("directory created");
    else draw_error("mkdir failed");
    return r == 0;
}

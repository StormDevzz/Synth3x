#define _GNU_SOURCE
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <fileman.h>

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

char* input_dialog(const char *prompt, const char *initial) {
    int h = 5, w = 60;
    int x = (COLS - w) / 2;
    int y = (LINES - h) / 2;

    WINDOW *win = newwin(h, w, y, x);
    keypad(win, TRUE);
    box(win, 0, 0);
    mvwprintw(win, 1, 2, " %s ", prompt);
    wrefresh(win);

    char buf[512] = {0};
    if (initial)
        strncpy(buf, initial, sizeof(buf) - 1);

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

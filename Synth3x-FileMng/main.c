#define _GNU_SOURCE
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include "fileman.h"

static FileState fs;

static void cleanup(void) {
    fs_free(&fs);
    endwin();
}

static void refresh_dir(FileState *fs) {
    fs_load(fs);
    if (fs->cursor >= fs->count) fs->cursor = fs->count > 0 ? fs->count - 1 : 0;
    if (fs->top > fs->cursor) fs->top = fs->cursor;
    int h = LINES - 4;
    if (fs->top + h > fs->count && fs->count > 0)
        fs->top = fs->count > h ? fs->count - h : 0;
}

int main(void) {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();
    use_default_colors();
    init_colors();
    atexit(cleanup);

    fs_init(&fs);
    if (!fs_load(&fs)) {
        endwin();
        fprintf(stderr, "cannot read current directory\n");
        return 1;
    }

    int ch;
    while (1) {
        draw_ui(&fs);
        ch = getch();

        switch (ch) {
        case 'q': case 27: return 0;

        case KEY_UP:    case 'k':
            if (fs.cursor > 0) fs.cursor--;
            break;
        case KEY_DOWN:  case 'j':
            if (fs.cursor < fs.count - 1) fs.cursor++;
            break;
        case KEY_PPAGE: case '\f':
            fs.cursor -= LINES - 4;
            if (fs.cursor < 0) fs.cursor = 0;
            break;
        case KEY_NPAGE:
            fs.cursor += LINES - 4;
            if (fs.cursor >= fs.count) fs.cursor = fs.count - 1;
            break;
        case KEY_HOME: case 'g':
            fs.cursor = 0; fs.top = 0;
            break;
        case KEY_END: case 'G':
            fs.cursor = fs.count - 1;
            break;

        case 'n':
            fs.sort_by = SORT_NAME;
            fs.sort_rev = 0;
            fs_sort(&fs); refresh_dir(&fs);
            break;
        case 's':
            fs.sort_by = SORT_SIZE;
            fs.sort_rev = 0;
            fs_sort(&fs); refresh_dir(&fs);
            break;
        case 't':
            fs.sort_by = SORT_TIME;
            fs.sort_rev = 0;
            fs_sort(&fs); refresh_dir(&fs);
            break;
        case 'N':
            fs.sort_by = SORT_NAME;
            fs.sort_rev = !fs.sort_rev;
            fs_sort(&fs); refresh_dir(&fs);
            break;
        case 'S':
            fs.sort_by = SORT_SIZE;
            fs.sort_rev = !fs.sort_rev;
            fs_sort(&fs); refresh_dir(&fs);
            break;
        case 'T':
            fs.sort_by = SORT_TIME;
            fs.sort_rev = !fs.sort_rev;
            fs_sort(&fs); refresh_dir(&fs);
            break;

        case '\n': case KEY_RIGHT: {
            if (fs.count == 0) break;
            FileEntry *e = &fs.entries[fs.cursor];
            if (e->is_dir) {
                char path[4096];
                snprintf(path, sizeof(path), "%s/%s", fs.cwd, e->name);
                if (!fs_cd(&fs, path)) {
                    draw_error("cannot enter directory");
                }
                refresh_dir(&fs);
            } else {
                char path[4096];
                snprintf(path, sizeof(path), "%s/%s", fs.cwd, e->name);
                run_viewer(path);
                touchwin(stdscr);
            }
            break;
        }
        case KEY_BACKSPACE: case 127: case 'h': {
            char *slash = strrchr(fs.cwd, '/');
            if (slash && slash != fs.cwd) {
                *slash = 0;
            } else if (slash == fs.cwd) {
                *(slash + 1) = 0;
            }
            refresh_dir(&fs);
            break;
        }
        case '~': {
            const char *home = getenv("HOME");
            if (home) { fs_cd(&fs, home); refresh_dir(&fs); }
            break;
        }

        case 'c': do_copy(&fs); refresh_dir(&fs); break;
        case 'm': do_move(&fs); refresh_dir(&fs); break;
        case 'd': do_delete(&fs); refresh_dir(&fs); break;
        case 'r': do_rename(&fs); refresh_dir(&fs); break;
        case 'K': do_mkdir(&fs); refresh_dir(&fs); break;

        default: break;
        }

        int h = LINES - 4;
        if (fs.cursor < fs.top) fs.top = fs.cursor;
        if (fs.cursor >= fs.top + h) fs.top = fs.cursor - h + 1;
    }
}

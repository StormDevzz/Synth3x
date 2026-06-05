#ifndef FM_H
#define FM_H

#include <gtk/gtk.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#define COL_NAME    0
#define COL_SIZE    1
#define COL_TYPE    2
#define COL_MTIME   3
#define COL_ICON    4
#define COL_IS_DIR  5
#define COL_PATH    6
#define N_COLUMNS   7

typedef struct {
    GtkWidget *window;
    GtkWidget *list_view;
    GtkListStore *store;
    GtkWidget *path_entry;
    GtkWidget *status_label;
    GtkWidget *sidebar;
    char       cwd[4096];
} AppState;

extern AppState state;

void load_directory(void);
void go_up(void);
void go_home(void);
void go_path(const char *path);
void open_selected(void);
void select_file_by_name(const char *name);

char* get_selected_path(void);
char* get_selected_name(void);

void file_copy(void);
void file_move(void);
void file_delete(void);
void file_rename(void);
void file_mkdir(void);
void show_properties(void);
void drop_files(const char *uri_list);

void update_status(const char *fmt, ...);

#endif

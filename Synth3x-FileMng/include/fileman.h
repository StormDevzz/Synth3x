#ifndef FILEMAN_H
#define FILEMAN_H

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define SORT_NAME 0
#define SORT_SIZE 1
#define SORT_TIME 2

typedef struct {
    char  name[512];
    mode_t mode;
    off_t  size;
    time_t mtime;
    int    is_dir;
} FileEntry;

typedef struct {
    FileEntry *entries;
    int        count;
    int        capacity;
    int        sort_by;
    int        sort_rev;
    char       cwd[4096];
    int        top;
    int        cursor;
} FileState;

void   fs_init(FileState *fs);
void   fs_free(FileState *fs);
int    fs_load(FileState *fs);
int    fs_cd(FileState *fs, const char *path);
void   fs_sort(FileState *fs);

char*  fmt_size(off_t size, char *buf, size_t len);
char*  fmt_time(time_t t, char *buf, size_t len);
int    confirm_dialog(const char *msg, const char *detail);
char*  input_dialog(const char *prompt, const char *initial);

void   init_colors(void);
void   draw_ui(FileState *fs);
void   draw_status(const char *msg);
void   draw_error(const char *msg);
int    run_viewer(const char *path);

int    do_copy(FileState *fs);
int    do_move(FileState *fs);
int    do_delete(FileState *fs);
int    do_rename(FileState *fs);
int    do_mkdir(FileState *fs);

#endif

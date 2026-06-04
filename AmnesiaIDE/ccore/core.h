#ifndef AMNESIA_CORE_H
#define AMNESIA_CORE_H

#include <stddef.h>

#define KEY_CTRL(k)   ((k) & 0x1f)
#define KEY_ESC       27
#define KEY_ENTER     10
#define KEY_BACKSPACE 127
#define KEY_TAB       9
#define KEY_F(n)      (0x101 + (n))

typedef struct { int rows, cols; } TermSize;

typedef struct {
    char **lines;
    int count, capacity;
} FileBuf;

void    term_raw_mode(int enable);
TermSize term_get_size(void);
void    term_clear(void);
void    term_goto(int row, int col);
void    term_hide_cursor(int hide);
void    term_write(const char *s, int len);

FileBuf *file_read(const char *path);
int      file_write(const char *path, FileBuf *buf);
void     file_free(FileBuf *buf);

int      syntax_match(const char *line, int col, const char **out_color);
int      syntax_lang(const char *filename);

int      spawn_shell(void);
int      spawn_command(const char *cmd);
int      read_stdin_raw(void);

#endif

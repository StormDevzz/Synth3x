#include "core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>

static struct termios orig_termios;

void term_raw_mode(int enable) {
    static int saved = 0;
    if (enable) {
        if (!saved) { tcgetattr(STDIN_FILENO, &orig_termios); saved = 1; }
        struct termios raw = orig_termios;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= CS8;
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN] = 0; raw.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    } else if (saved) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
}

TermSize term_get_size(void) {
    struct winsize ws; TermSize ts = {24, 80};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1)
        { ts.rows = ws.ws_row; ts.cols = ws.ws_col; }
    return ts;
}

void term_clear(void) { write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7); }
void term_goto(int r, int c) { char b[32]; int n = snprintf(b,32,"\x1b[%d;%dH",r,c); write(STDOUT_FILENO,b,n); }
void term_hide_cursor(int h) { write(STDOUT_FILENO, h ? "\x1b[?25l" : "\x1b[?25h", 6); }
void term_write(const char *s, int l) { write(STDOUT_FILENO, s, l); }

FileBuf *file_read(const char *path) {
    FILE *f = fopen(path, "r"); if (!f) return NULL;
    FileBuf *b = calloc(1, sizeof(FileBuf)); if (!b) { fclose(f); return NULL; }
    b->capacity = 128; b->lines = calloc(b->capacity, sizeof(char*));
    char line[4096];
    while (fgets(line, sizeof(line), f)) {
        int l = strlen(line); if (l && line[l-1]=='\n') line[--l]='\0';
        if (b->count >= b->capacity) {
            b->capacity *= 2; b->lines = realloc(b->lines, b->capacity*sizeof(char*));
        }
        b->lines[b->count++] = strdup(line);
    }
    fclose(f); return b;
}

int file_write(const char *path, FileBuf *b) {
    FILE *f = fopen(path, "w"); if (!f) return -1;
    for (int i=0;i<b->count;i++) fprintf(f,"%s\n",b->lines[i]);
    fclose(f); return 0;
}

void file_free(FileBuf *b) {
    if (!b) return; for (int i=0;i<b->count;i++) free(b->lines[i]);
    free(b->lines); free(b);
}

/* ── language keywords ── */
static const char *kw_c[] = {
    "auto","break","case","char","const","continue","default","do",
    "double","else","enum","extern","float","for","goto","if",
    "int","long","register","return","short","signed","sizeof","static",
    "struct","switch","typedef","union","unsigned","void","volatile","while",
    "#include","#define","#ifdef","#ifndef","#endif","#pragma","#error","#undef",
    NULL
};

static const char *kw_cpp[] = {
    "class","public","private","protected","virtual","override","explicit",
    "template","typename","namespace","using","this","new","delete",
    "friend","operator","inline","constexpr","nullptr","noexcept",
    "throw","catch","try","dynamic_cast","static_cast","reinterpret_cast",
    "const_cast","decltype","auto","enum","struct","union","extern",
    "mutable","volatile","export","alignas","alignof","static_assert",
    "#include","#define","#ifdef","#ifndef","#endif","#pragma","once",
    NULL
};

static const char *kw_cs[] = {
    "abstract","as","base","bool","break","byte","case","catch","char",
    "checked","class","const","continue","decimal","default","delegate",
    "do","double","else","enum","event","explicit","extern","false",
    "finally","fixed","float","for","foreach","goto","if","implicit",
    "in","int","interface","internal","is","lock","long","namespace",
    "new","null","object","operator","out","override","params","private",
    "protected","public","readonly","ref","return","sbyte","sealed",
    "short","sizeof","stackalloc","static","string","struct","switch",
    "this","throw","true","try","typeof","uint","ulong","unchecked",
    "unsafe","ushort","using","var","virtual","void","volatile","while",
    NULL
};

static const char *kw_rust[] = {
    "as","break","const","continue","crate","else","enum","extern",
    "false","fn","for","if","impl","in","let","loop","match","mod",
    "move","mut","pub","ref","return","self","Self","static","struct",
    "super","trait","true","type","unsafe","use","where","while",
    "async","await","dyn","abstract","become","box","do","final",
    "macro","override","priv","try","typeof","unsized","virtual","yield",
    NULL
};

static const char *kw_asm[] = {
    "section","global","extern","bits","org","align","db","dw","dd",
    "dq","resb","resw","resd","resq","incbin","equ","times","macro",
    "endmacro","struc","endstruc","istruc","at","iend","PROC","ENDP",
    "ASSUME","offset","ptr","byte","word","dword","qword","tbyte",
    "near","far","proc","endp","assume","public","extrn","include",
    "mov","add","sub","mul","div","inc","dec","and","or","xor","not",
    "shl","shr","push","pop","call","ret","jmp","je","jne","jg","jl",
    "jge","jle","cmp","test","int","syscall","lea","nop","hlt",
    ".data",".text",".bss",".code",".stack",".model",
    NULL
};

static int kw_match(const char *word, const char **kws) {
    for (int i=0; kws[i]; i++)
        if (strcmp(word, kws[i]) == 0) return 1;
    return 0;
}

int syntax_lang(const char *filename) {
    if (!filename) return 0;
    const char *ext = strrchr(filename, '.'); if (!ext) return 0;
    if (strcmp(ext,".c")==0||strcmp(ext,".h")==0) return 1;
    if (strcmp(ext,".cpp")==0||strcmp(ext,".cc")==0||strcmp(ext,".cxx")==0||strcmp(ext,".hpp")==0) return 2;
    if (strcmp(ext,".cs")==0) return 3;
    if (strcmp(ext,".rs")==0) return 4;
    if (strcmp(ext,".asm")==0||strcmp(ext,".s")==0||strcmp(ext,".S")==0||strcmp(ext,".inc")==0) return 5;
    return 0;
}

int syntax_match(const char *line, int col, const char **out_color) {
    if (!line) return 0;
    int len = strlen(line);
    if (col >= len) return 0;
    char c = line[col];

    if (c == '/' && col+1 < len && line[col+1] == '/') { *out_color = "\x1b[90m"; return 1; }
    if (c == '/' && col+1 < len && line[col+1] == '*') { *out_color = "\x1b[90m"; return 1; }
    if (c == '*' && col > 0 && line[col-1] == '/') { *out_color = "\x1b[90m"; return 1; }

    if ((c == '"') || (c == '\'')) { *out_color = "\x1b[33m"; return 1; }
    if (c == '#' && col == 0) { *out_color = "\x1b[92m"; return 1; }
    if (c >= '0' && c <= '9') { *out_color = "\x1b[35m"; return 1; }

    if (c == '{' || c == '}' || c == '(' || c == ')' ||
        c == '[' || c == ']') { *out_color = "\x1b[93m"; return 1; }

    int start = col;
    while (start > 0 && (line[start-1]=='_' || 
        (line[start-1]>='a'&&line[start-1]<='z') ||
        (line[start-1]>='A'&&line[start-1]<='Z'))) start--;
    int end = col;
    while (end < len && (line[end]=='_' ||
        (line[end]>='a'&&line[end]<='z') ||
        (line[end]>='A'&&line[end]<='Z'))) end++;

    if (start < end) {
        int wl = end-start; if (wl > 63) wl = 63;
        char word[64]; memcpy(word, line+start, wl); word[wl]='\0';

        #define CHECK(kw) if (kw_match(word, kw)) { *out_color = "\x1b[36m"; return 1; }
        CHECK(kw_c) CHECK(kw_cpp) CHECK(kw_cs) CHECK(kw_rust) CHECK(kw_asm)
        #undef CHECK

        if (word[0] >= 'A' && word[0] <= 'Z') { *out_color = "\x1b[93m"; return 1; }
    }
    return 0;
}

int spawn_shell(void) {
    pid_t pid = fork();
    if (pid == 0) {
        const char *shell = getenv("SHELL");
        if (!shell) shell = "/bin/sh";
        execlp(shell, shell, NULL);
        _exit(1);
    }
    if (pid > 0) {
        int st; waitpid(pid, &st, 0);
    }
    return pid;
}

int spawn_command(const char *cmd) {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("sh", "sh", "-c", cmd, NULL);
        _exit(1);
    }
    if (pid > 0) {
        int st; waitpid(pid, &st, 0);
        return WEXITSTATUS(st);
    }
    return -1;
}

int read_stdin_raw(void) {
    unsigned char b;
    if (read(STDIN_FILENO, &b, 1) == 1) return b;
    return -1;
}

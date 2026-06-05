#define _GNU_SOURCE
#include <ncurses.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fileman.h>

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

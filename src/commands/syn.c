#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#define EMERGE "/usr/bin/emerge"

int main(int argc, char *argv[]) {
    char *bin_name = basename(argv[0]);

    if (strcmp(bin_name, "emerge") == 0) {
        execvp(EMERGE, argv);
        fprintf(stderr, "[!] emerge not found. Install Gentoo base system first.\n");
        return 1;
    }

    if (argc < 2) {
        printf("Synth3x package manager — Gentoo emerge wrapper\n\n");
        printf("Usage:\n");
        printf("  syn install <pkg>     Install package\n");
        printf("  syn remove <pkg>      Remove package\n");
        printf("  syn search <q>        Search packages\n");
        printf("  syn sync              Sync portage tree\n");
        printf("  syn list              List installed (syn only)\n");
        printf("  syn <emerge args>     Pass through to emerge\n\n");
        printf("Alternatively, use emerge directly:\n");
        printf("  emerge --ask <pkg>\n");
        printf("  emerge --sync\n");
        return 0;
    }

    const char *cmd = argv[1];
    const char *target = argc > 2 ? argv[2] : NULL;

    if (strcmp(cmd, "install") == 0 || strcmp(cmd, "inst") == 0) {
        if (!target) { fprintf(stderr, "Usage: syn install <package>\n"); return 1; }
        char **new_argv = malloc((argc + 2) * sizeof(char *));
        new_argv[0] = (char *)EMERGE;
        new_argv[1] = (char *)"--ask";
        for (int i = 2; i < argc; i++) new_argv[i] = argv[i];
        new_argv[argc] = NULL;
        execvp(EMERGE, new_argv);
        free(new_argv);
    } else if (strcmp(cmd, "remove") == 0 || strcmp(cmd, "rm") == 0) {
        if (!target) { fprintf(stderr, "Usage: syn remove <package>\n"); return 1; }
        char *new_argv[] = {(char *)EMERGE, (char *)"--unmerge", (char *)target, NULL};
        execvp(EMERGE, new_argv);
    } else if (strcmp(cmd, "search") == 0) {
        if (!target) { fprintf(stderr, "Usage: syn search <query>\n"); return 1; }
        char *new_argv[] = {(char *)EMERGE, (char *)"--search", (char *)target, NULL};
        execvp(EMERGE, new_argv);
    } else if (strcmp(cmd, "sync") == 0 || strcmp(cmd, "update") == 0) {
        char *new_argv[] = {(char *)EMERGE, (char *)"--sync", NULL};
        execvp(EMERGE, new_argv);
    } else if (strcmp(cmd, "list") == 0 || strcmp(cmd, "ls") == 0) {
        char *new_argv[] = {(char *)EMERGE, (char *)"-p", (char *)"@world", NULL};
        execvp(EMERGE, new_argv);
    } else if (strcmp(cmd, "info") == 0) {
        if (!target) { fprintf(stderr, "Usage: syn info <package>\n"); return 1; }
        char *new_argv[] = {(char *)EMERGE, (char *)"-pv", (char *)target, NULL};
        execvp(EMERGE, new_argv);
    } else {
        execvp(EMERGE, argv + 1);
    }

    fprintf(stderr, "[!] emerge not found. Install Gentoo base system first.\n");
    return 1;
}

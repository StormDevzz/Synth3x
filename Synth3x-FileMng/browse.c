#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "fileman.h"

void fs_init(FileState *fs) {
    memset(fs, 0, sizeof(*fs));
    fs->sort_by = SORT_NAME;
    fs->capacity = 256;
    fs->entries = malloc(fs->capacity * sizeof(FileEntry));
    if (!getcwd(fs->cwd, sizeof(fs->cwd))) {
        strcpy(fs->cwd, "/");
    }
}

void fs_free(FileState *fs) {
    free(fs->entries);
    fs->entries = NULL;
    fs->count = 0;
    fs->capacity = 0;
}

static int entry_cmp_name(const void *a, const void *b) {
    const FileEntry *ea = (const FileEntry *)a;
    const FileEntry *eb = (const FileEntry *)b;
    if (ea->is_dir != eb->is_dir)
        return ea->is_dir ? -1 : 1;
    return strcoll(ea->name, eb->name);
}

static int entry_cmp_size(const void *a, const void *b) {
    const FileEntry *ea = (const FileEntry *)a;
    const FileEntry *eb = (const FileEntry *)b;
    if (ea->is_dir != eb->is_dir)
        return ea->is_dir ? -1 : 1;
    if (ea->size < eb->size) return -1;
    if (ea->size > eb->size) return 1;
    return 0;
}

static int entry_cmp_time(const void *a, const void *b) {
    const FileEntry *ea = (const FileEntry *)a;
    const FileEntry *eb = (const FileEntry *)b;
    if (ea->is_dir != eb->is_dir)
        return ea->is_dir ? -1 : 1;
    if (ea->mtime < eb->mtime) return -1;
    if (ea->mtime > eb->mtime) return 1;
    return 0;
}

void fs_sort(FileState *fs) {
    qsort(fs->entries, fs->count, sizeof(FileEntry),
          fs->sort_by == SORT_SIZE ? entry_cmp_size :
          fs->sort_by == SORT_TIME ? entry_cmp_time :
          entry_cmp_name);
    if (fs->sort_rev) {
        for (int i = 0, j = fs->count - 1; i < j; i++, j--) {
            FileEntry t = fs->entries[i];
            fs->entries[i] = fs->entries[j];
            fs->entries[j] = t;
        }
    }
}

int fs_load(FileState *fs) {
    DIR *d = opendir(fs->cwd);
    if (!d) return 0;

    fs->count = 0;
    struct dirent *de;
    struct stat st;

    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0) continue;

        if (fs->count >= fs->capacity) {
            fs->capacity *= 2;
            fs->entries = realloc(fs->entries, fs->capacity * sizeof(FileEntry));
        }

        FileEntry *e = &fs->entries[fs->count];
        strncpy(e->name, de->d_name, sizeof(e->name) - 1);

        char full[4096];
        snprintf(full, sizeof(full), "%s/%s", fs->cwd, de->d_name);

        if (lstat(full, &st) == 0) {
            e->mode   = st.st_mode;
            e->size   = st.st_size;
            e->mtime  = st.st_mtime;
            e->is_dir = S_ISDIR(st.st_mode);
        } else {
            e->mode   = 0;
            e->size   = 0;
            e->mtime  = 0;
            e->is_dir = 0;
        }
        fs->count++;
    }
    closedir(d);

    fs_sort(fs);
    return 1;
}

int fs_cd(FileState *fs, const char *path) {
    if (!path || *path == 0) return 0;

    char resolved[4096];
    if (realpath(path, resolved) == NULL) return 0;

    DIR *d = opendir(resolved);
    if (!d) return 0;
    closedir(d);

    strncpy(fs->cwd, resolved, sizeof(fs->cwd) - 1);
    fs->cursor = 0;
    fs->top = 0;
    return 1;
}

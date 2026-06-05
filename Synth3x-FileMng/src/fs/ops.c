#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fileman.h>

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

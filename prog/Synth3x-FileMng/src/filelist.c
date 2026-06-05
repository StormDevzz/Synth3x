#include "fm.h"
#include <dirent.h>

static char* fmt_size(off_t size) {
    static char buf[16];
    if (size < 1024)
        snprintf(buf, sizeof(buf), "%ld B", (long)size);
    else if (size < 1024 * 1024)
        snprintf(buf, sizeof(buf), "%.1f K", (double)size / 1024);
    else if (size < 1024LL * 1024 * 1024)
        snprintf(buf, sizeof(buf), "%.1f M", (double)size / (1024 * 1024));
    else
        snprintf(buf, sizeof(buf), "%.1f G", (double)size / (1024LL * 1024 * 1024));
    return buf;
}

static char* fmt_time(time_t t) {
    static char buf[32];
    struct tm *tm = localtime(&t);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm);
    return buf;
}

const char* mime_type(mode_t mode) {
    if (S_ISDIR(mode)) return "directory";
    if (S_ISLNK(mode)) return "symlink";
    if (S_ISFIFO(mode)) return "fifo";
    if (S_ISSOCK(mode)) return "socket";
    if (mode & S_IXUSR) return "executable";
    return "file";
}

void load_directory(void) {
    gtk_list_store_clear(state.store);
    gtk_entry_set_text(GTK_ENTRY(state.path_entry), state.cwd);

    DIR *d = opendir(state.cwd);
    if (!d) {
        update_status("cannot open directory");
        return;
    }

    struct dirent *de;
    struct stat st;
    int count = 0;

    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0) continue;

        char full[4096];
        snprintf(full, sizeof(full), "%s/%s", state.cwd, de->d_name);

        gboolean is_dir = FALSE;
        off_t size = 0;
        time_t mtime = 0;
        mode_t mode = 0;

        if (lstat(full, &st) == 0) {
            mode = st.st_mode;
            size = st.st_size;
            mtime = st.st_mtime;
            is_dir = S_ISDIR(mode);
        }

        GtkTreeIter iter;
        gtk_list_store_append(state.store, &iter);
        gtk_list_store_set(state.store, &iter,
            COL_NAME,   de->d_name,
            COL_SIZE,   is_dir ? "" : fmt_size(size),
            COL_TYPE,   mime_type(mode),
            COL_MTIME,  fmt_time(mtime),
            COL_IS_DIR, is_dir,
            COL_PATH,   full,
            -1);
        count++;
    }
    closedir(d);

    update_status("%d items", count);
}

void go_up(void) {
    char *slash = strrchr(state.cwd, '/');
    if (slash && slash != state.cwd)
        *slash = 0;
    else if (slash == state.cwd)
        *(slash + 1) = 0;
    load_directory();
}

void go_home(void) {
    const char *home = g_get_home_dir();
    if (home) {
        strncpy(state.cwd, home, sizeof(state.cwd) - 1);
        load_directory();
    }
}

void go_path(const char *path) {
    if (!path || !path[0]) return;
    char resolved[4096];
    if (realpath(path, resolved)) {
        strncpy(state.cwd, resolved, sizeof(state.cwd) - 1);
        load_directory();
    } else {
        update_status("invalid path: %s", path);
    }
}

void open_selected(void) {
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(state.list_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        char *name = NULL;
        char *full = NULL;
        gboolean is_dir = FALSE;
        gtk_tree_model_get(model, &iter, COL_NAME, &name, COL_IS_DIR, &is_dir, COL_PATH, &full, -1);
        if (is_dir && full) {
            strncpy(state.cwd, full, sizeof(state.cwd) - 1);
            load_directory();
        } else if (full) {
            gtk_show_uri_on_window(GTK_WINDOW(state.window), g_filename_to_uri(full, NULL, NULL),
                                   GDK_CURRENT_TIME, NULL);
        }
        g_free(name);
        g_free(full);
    }
}

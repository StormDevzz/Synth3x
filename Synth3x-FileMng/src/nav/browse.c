#include "../fm.h"

static char* fmt_size(off_t size) {
    static char buf[32];
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

static const char* mime_type(mode_t mode) {
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
    if (!d) { update_status("cannot open directory"); return; }

    struct dirent *de;
    struct stat st;
    int count = 0;

    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0) continue;

        char full[sizeof(state.cwd) + 256];
        int n = snprintf(full, sizeof(full), "%s/%s", state.cwd, de->d_name);
        if (n < 0 || (size_t)n >= sizeof(full)) continue;

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

    update_status("%d items  —  %s", count, state.cwd);
}

void go_up(void) {
    char *slash = strrchr(state.cwd, '/');
    if (slash && slash != state.cwd) *slash = 0;
    else if (slash == state.cwd) *(slash + 1) = 0;
    load_directory();
}

void go_home(void) {
    const char *home = g_get_home_dir();
    if (home) { snprintf(state.cwd, sizeof(state.cwd), "%s", home); load_directory(); }
}

void go_path(const char *path) {
    if (!path || !path[0]) return;
    char resolved[4096];
    if (realpath(path, resolved)) {
        snprintf(state.cwd, sizeof(state.cwd), "%s", resolved);
        load_directory();
    } else {
        update_status("invalid path: %s", path);
    }
}

void path_go_to(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Go to Path",
        GTK_WINDOW(state.window), GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL, "Go", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), state.cwd);
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "/path/to/directory");
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 4);
    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
        go_path(gtk_entry_get_text(GTK_ENTRY(entry)));
    gtk_widget_destroy(dialog);
}

void open_selected(void) {
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(state.list_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        char *name = NULL, *full = NULL;
        gboolean is_dir = FALSE;
        gtk_tree_model_get(model, &iter, COL_NAME, &name, COL_IS_DIR, &is_dir, COL_PATH, &full, -1);
        if (is_dir && full) {
            snprintf(state.cwd, sizeof(state.cwd), "%s", full);
            load_directory();
        } else if (full) {
            gtk_show_uri_on_window(GTK_WINDOW(state.window),
                g_filename_to_uri(full, NULL, NULL), GDK_CURRENT_TIME, NULL);
        }
        g_free(name); g_free(full);
    }
}

char* get_selected_path(void) {
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(state.list_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) return NULL;
    char *path = NULL;
    gtk_tree_model_get(model, &iter, COL_PATH, &path, -1);
    return path;
}

char* get_selected_name(void) {
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(state.list_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter)) return NULL;
    char *name = NULL;
    gtk_tree_model_get(model, &iter, COL_NAME, &name, -1);
    return name;
}

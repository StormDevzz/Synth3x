#include "../fm.h"

void show_properties(void) {
    char *path = get_selected_path();
    if (!path) { update_status("no file selected"); return; }

    struct stat st;
    if (lstat(path, &st) != 0) { update_status("cannot stat"); g_free(path); return; }

    char info[1024];
    snprintf(info, sizeof(info),
        "Size: %ld bytes\nMode: %o\nUID: %d  GID: %d\nLinks: %ld\nModified: %s",
        (long)st.st_size, (unsigned)st.st_mode & 0777,
        st.st_uid, st.st_gid, (long)st.st_nlink, ctime(&st.st_mtime));

    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state.window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", path);
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", info);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_free(path);
}

void drop_files(const char *uri_list) {
    if (!uri_list || !uri_list[0]) return;
    char **uris = g_uri_list_extract_uris(uri_list);
    if (!uris) return;

    int moved = 0;
    for (int i = 0; uris[i]; i++) {
        char *local = g_filename_from_uri(uris[i], NULL, NULL);
        if (!local) { g_free(uris[i]); continue; }
        const char *name = strrchr(local, '/');
        name = name ? name + 1 : local;
        char dest[4096], cmd[4608];
        snprintf(dest, sizeof(dest), "%s/%s", state.cwd, name);
        snprintf(cmd, sizeof(cmd), "mv '%s' '%s'", local, dest);
        if (system(cmd) == 0) moved++;
        g_free(local);
        g_free(uris[i]);
    }
    g_free(uris);
    if (moved > 0) { update_status("moved %d file(s)", moved); load_directory(); }
}

void update_status(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    gtk_label_set_text(GTK_LABEL(state.status_label), buf);
}

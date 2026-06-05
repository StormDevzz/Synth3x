#include "../fm.h"

static gboolean confirm(const char *msg, const char *detail) {
    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(state.window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, "%s", msg);
    if (detail) gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(d), "%s", detail);
    gint r = gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
    return r == GTK_RESPONSE_YES;
}

static void cp_or_mv(const char *title, gboolean is_copy) {
    char *src = get_selected_path();
    if (!src) { update_status("no file selected"); return; }
    char *name = get_selected_name();
    const char *act = is_copy ? "cp" : "mv";

    GtkWidget *dialog = gtk_file_chooser_dialog_new(title,
        GTK_WINDOW(state.window), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "Cancel", GTK_RESPONSE_CANCEL, title, GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state.cwd);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *dest_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        char dest[sizeof(state.cwd) + 256], cmd[4608];
        snprintf(dest, sizeof(dest), "%s/%s", dest_dir, name);
        snprintf(cmd, sizeof(cmd), "%s -r '%s' '%s'", act, src, dest);
        if (system(cmd) == 0)
            update_status("%s -> %s", is_copy ? "copied" : "moved", dest);
        else
            update_status("%s failed", is_copy ? "copy" : "move");
        g_free(dest_dir);
        load_directory();
    }
    gtk_widget_destroy(dialog);
    g_free(src); g_free(name);
}

void file_copy(void)   { cp_or_mv("Copy", TRUE); }
void file_move(void)   { cp_or_mv("Move", FALSE); }

void file_delete(void) {
    char *src = get_selected_path();
    if (!src) { update_status("no file selected"); return; }
    char *name = get_selected_name();
    char msg[256];
    snprintf(msg, sizeof(msg), "Delete %s?", name);
    if (confirm(msg, NULL)) {
        char cmd[4608];
        snprintf(cmd, sizeof(cmd), "rm -rf '%s'", src);
        if (system(cmd) == 0) update_status("deleted %s", name);
        else update_status("delete failed");
        load_directory();
    }
    g_free(src); g_free(name);
}

void file_rename(void) {
    char *src = get_selected_path();
    if (!src) { update_status("no file selected"); return; }
    char *name = get_selected_name();

    GtkWidget *dialog = gtk_dialog_new_with_buttons("Rename",
        GTK_WINDOW(state.window), GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL, "Rename", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(entry), name);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 4);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *newname = gtk_entry_get_text(GTK_ENTRY(entry));
        char newp[sizeof(state.cwd) + 256];
        snprintf(newp, sizeof(newp), "%s/%s", state.cwd, newname);
        if (rename(src, newp) == 0) update_status("renamed to %s", newname);
        else update_status("rename failed");
        load_directory();
    }
    gtk_widget_destroy(dialog);
    g_free(src); g_free(name);
}

void file_mkdir(void) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons("New Folder",
        GTK_WINDOW(state.window), GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL, "Create", GTK_RESPONSE_ACCEPT, NULL);
    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "folder name");
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 4);
    gtk_widget_show_all(dialog);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry));
        if (name && name[0]) {
            char path[sizeof(state.cwd) + 256];
            snprintf(path, sizeof(path), "%s/%s", state.cwd, name);
            if (mkdir(path, 0755) == 0) update_status("folder created");
            else update_status("mkdir failed");
            load_directory();
        }
    }
    gtk_widget_destroy(dialog);
}

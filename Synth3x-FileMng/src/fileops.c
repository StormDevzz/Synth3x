#include "fm.h"

static gboolean confirm_action(const char *msg, const char *detail) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state.window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "%s", msg);
    if (detail)
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", detail);
    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return res == GTK_RESPONSE_YES;
}

static char* get_selected_path(void) {
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(state.list_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return NULL;
    char *path = NULL;
    gtk_tree_model_get(model, &iter, COL_PATH, &path, -1);
    return path;
}

static char* get_selected_name(void) {
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(state.list_view));
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (!gtk_tree_selection_get_selected(sel, &model, &iter))
        return NULL;
    char *name = NULL;
    gtk_tree_model_get(model, &iter, COL_NAME, &name, -1);
    return name;
}

void file_copy(void) {
    char *src = get_selected_path();
    if (!src) { update_status("no file selected"); return; }

    char *name = get_selected_name();
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Copy to",
        GTK_WINDOW(state.window), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "Cancel", GTK_RESPONSE_CANCEL, "Copy Here", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state.cwd);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *dest_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        char dest[4096];
        snprintf(dest, sizeof(dest), "%s/%s", dest_dir, name);
        char cmd[4608];
        snprintf(cmd, sizeof(cmd), "cp -r '%s' '%s'", src, dest);
        if (system(cmd) == 0)
            update_status("copied to %s", dest);
        else
            update_status("copy failed");
        g_free(dest_dir);
        load_directory();
    }
    gtk_widget_destroy(dialog);
    g_free(src);
    g_free(name);
}

void file_move(void) {
    char *src = get_selected_path();
    if (!src) { update_status("no file selected"); return; }

    char *name = get_selected_name();
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Move to",
        GTK_WINDOW(state.window), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "Cancel", GTK_RESPONSE_CANCEL, "Move Here", GTK_RESPONSE_ACCEPT, NULL);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), state.cwd);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *dest_dir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        char dest[4096];
        snprintf(dest, sizeof(dest), "%s/%s", dest_dir, name);
        char cmd[4608];
        snprintf(cmd, sizeof(cmd), "mv '%s' '%s'", src, dest);
        if (system(cmd) == 0)
            update_status("moved to %s", dest);
        else
            update_status("move failed");
        g_free(dest_dir);
        load_directory();
    }
    gtk_widget_destroy(dialog);
    g_free(src);
    g_free(name);
}

void file_delete(void) {
    char *src = get_selected_path();
    if (!src) { update_status("no file selected"); return; }

    char *name = get_selected_name();
    char msg[256];
    snprintf(msg, sizeof(msg), "Delete %s?", name);
    if (confirm_action(msg, NULL)) {
        char cmd[4608];
        snprintf(cmd, sizeof(cmd), "rm -rf '%s'", src);
        if (system(cmd) == 0)
            update_status("deleted %s", name);
        else
            update_status("delete failed");
        load_directory();
    }
    g_free(src);
    g_free(name);
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
        char newp[4096];
        snprintf(newp, sizeof(newp), "%s/%s", state.cwd, newname);
        if (rename(src, newp) == 0)
            update_status("renamed to %s", newname);
        else
            update_status("rename failed");
        load_directory();
    }
    gtk_widget_destroy(dialog);
    g_free(src);
    g_free(name);
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
            char path[4096];
            snprintf(path, sizeof(path), "%s/%s", state.cwd, name);
            if (mkdir(path, 0755) == 0)
                update_status("folder created");
            else
                update_status("mkdir failed");
            load_directory();
        }
    }
    gtk_widget_destroy(dialog);
}

void update_status(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    gtk_label_set_text(GTK_LABEL(state.status_label), buf);
}

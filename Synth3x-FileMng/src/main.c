#include "fm.h"

AppState state;

static void on_activate(GtkApplication *app, gpointer data);
static void on_back_clicked(void);
static void on_home_clicked(void);
static void on_refresh_clicked(void);
static void on_path_activate(void);
static void on_row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col);
static void on_sidebar_row(GtkListBox *box, GtkListBoxRow *row);
static gboolean on_key_press(GtkWidget *w, GdkEventKey *ev);
static void on_copy_clicked(void);
static void on_move_clicked(void);
static void on_delete_clicked(void);
static void on_rename_clicked(void);
static void on_mkdir_clicked(void);

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("synth3x.fileman", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

static void on_activate(GtkApplication *app, gpointer data) {
    (void)data;
    if (!getcwd(state.cwd, sizeof(state.cwd)))
        strcpy(state.cwd, "/");

    state.window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state.window), "Synth3x File Manager");
    gtk_window_set_default_size(GTK_WINDOW(state.window), 900, 600);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(state.window), vbox);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(toolbar), 4);
    gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

    GtkWidget *btn;

    btn = gtk_button_new_with_label("Up");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(on_back_clicked), NULL);

    btn = gtk_button_new_with_label("Home");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(on_home_clicked), NULL);

    state.path_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(state.path_entry), state.cwd);
    gtk_box_pack_start(GTK_BOX(toolbar), state.path_entry, TRUE, TRUE, 4);
    g_signal_connect(state.path_entry, "activate", G_CALLBACK(on_path_activate), NULL);

    btn = gtk_button_new_with_label("Refresh");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(on_refresh_clicked), NULL);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(toolbar), sep, FALSE, FALSE, 4);

    btn = gtk_button_new_with_label("Copy");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(on_copy_clicked), NULL);

    btn = gtk_button_new_with_label("Move");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(on_move_clicked), NULL);

    btn = gtk_button_new_with_label("Delete");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(on_delete_clicked), NULL);

    btn = gtk_button_new_with_label("Rename");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(on_rename_clicked), NULL);

    btn = gtk_button_new_with_label("New Folder");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect_swapped(btn, "clicked", G_CALLBACK(on_mkdir_clicked), NULL);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);

    state.sidebar = gtk_list_box_new();
    gtk_widget_set_size_request(state.sidebar, 160, -1);
    gtk_paned_pack1(GTK_PANED(paned), state.sidebar, FALSE, FALSE);
    g_signal_connect(state.sidebar, "row-activated", G_CALLBACK(on_sidebar_row), NULL);

    const char *places[] = {
        g_get_home_dir(),
        "/home/nprevenant/Desktop",
        "/home/nprevenant/Documents",
        "/home/nprevenant/Downloads",
        "/",
        NULL
    };
    for (int i = 0; places[i]; i++) {
        if (g_file_test(places[i], G_FILE_TEST_IS_DIR)) {
            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *l = gtk_label_new(places[i]);
            gtk_label_set_xalign(GTK_LABEL(l), 0.0);
            gtk_container_add(GTK_CONTAINER(row), l);
            gtk_list_box_insert(GTK_LIST_BOX(state.sidebar), row, -1);
        }
    }

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_paned_pack2(GTK_PANED(paned), scroll, TRUE, TRUE);

    state.list_view = gtk_tree_view_new();
    gtk_container_add(GTK_CONTAINER(scroll), state.list_view);

    GtkCellRenderer *r;
    GtkTreeViewColumn *col;

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Name", r, "text", COL_NAME, NULL);
    gtk_tree_view_column_set_sort_column_id(col, COL_NAME);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_min_width(col, 200);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state.list_view), col);

    r = gtk_cell_renderer_text_new();
    g_object_set(r, "xalign", 1.0, NULL);
    col = gtk_tree_view_column_new_with_attributes("Size", r, "text", COL_SIZE, NULL);
    gtk_tree_view_column_set_sort_column_id(col, COL_SIZE);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state.list_view), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Type", r, "text", COL_TYPE, NULL);
    gtk_tree_view_column_set_sort_column_id(col, COL_TYPE);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state.list_view), col);

    r = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes("Modified", r, "text", COL_MTIME, NULL);
    gtk_tree_view_column_set_sort_column_id(col, COL_MTIME);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(state.list_view), col);

    state.store = gtk_list_store_new(N_COLUMNS,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(state.list_view), GTK_TREE_MODEL(state.store));
    g_object_unref(state.store);

    //gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(state.list_view), TRUE);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(state.list_view)),
                                GTK_SELECTION_SINGLE);

    g_signal_connect(state.list_view, "row-activated", G_CALLBACK(on_row_activated), NULL);
    g_signal_connect(state.window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    state.status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(state.status_label), 0.0);
    gtk_container_set_border_width(GTK_CONTAINER(state.status_label), 3);
    gtk_box_pack_start(GTK_BOX(hbox), state.status_label, TRUE, TRUE, 0);

    load_directory();
    gtk_widget_show_all(state.window);
}

static void on_back_clicked(void) { go_up(); }
static void on_home_clicked(void) { go_home(); }
static void on_refresh_clicked(void) { load_directory(); }
static void on_path_activate(void) { go_path(gtk_entry_get_text(GTK_ENTRY(state.path_entry))); }
static void on_row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col) {
    (void)view; (void)path; (void)col;
    open_selected();
}
static void on_sidebar_row(GtkListBox *box, GtkListBoxRow *row) {
    (void)box;
    GtkWidget *l = gtk_bin_get_child(GTK_BIN(row));
    go_path(gtk_label_get_text(GTK_LABEL(l)));
}
static gboolean on_key_press(GtkWidget *w, GdkEventKey *ev) {
    (void)w;
    if (ev->keyval == GDK_KEY_BackSpace) { go_up(); return TRUE; }
    if (ev->keyval == GDK_KEY_Delete) { file_delete(); return TRUE; }
    if ((ev->state & GDK_CONTROL_MASK) && ev->keyval == GDK_KEY_c) { file_copy(); return TRUE; }
    if ((ev->state & GDK_CONTROL_MASK) && ev->keyval == GDK_KEY_x) { file_move(); return TRUE; }
    if ((ev->state & GDK_CONTROL_MASK) && ev->keyval == GDK_KEY_n) { file_mkdir(); return TRUE; }
    return FALSE;
}
static void on_copy_clicked(void) { file_copy(); }
static void on_move_clicked(void) { file_move(); }
static void on_delete_clicked(void) { file_delete(); }
static void on_rename_clicked(void) { file_rename(); }
static void on_mkdir_clicked(void) { file_mkdir(); }

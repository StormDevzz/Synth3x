#include "fm.h"

AppState state;

static void on_activate(GtkApplication *app, gpointer data);
static void on_path_activate(void);
static void on_row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col);
static void on_sidebar_row(GtkListBox *box, GtkListBoxRow *row);
static gboolean on_key_press(GtkWidget *w, GdkEventKey *ev);
static gboolean on_button_press(GtkWidget *w, GdkEventButton *ev);
static void on_context_open(void);
static void on_context_copy(void);
static void on_context_move(void);
static void on_context_delete(void);
static void on_context_rename(void);
static void on_context_properties(void);
static void on_drag_data_get(GtkWidget *w, GdkDragContext *ctx, GtkSelectionData *data, guint info, guint time);
static void on_drag_data_received(GtkWidget *w, GdkDragContext *ctx, gint x, gint y, GtkSelectionData *data, guint info, guint time);

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
    g_signal_connect(btn, "clicked", G_CALLBACK(go_up), NULL);

    btn = gtk_button_new_with_label("Home");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect(btn, "clicked", G_CALLBACK(go_home), NULL);

    state.path_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(state.path_entry), state.cwd);
    gtk_box_pack_start(GTK_BOX(toolbar), state.path_entry, TRUE, TRUE, 4);
    g_signal_connect(state.path_entry, "activate", G_CALLBACK(on_path_activate), NULL);

    btn = gtk_button_new_with_label("Refresh");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect(btn, "clicked", G_CALLBACK(load_directory), NULL);

    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_pack_start(GTK_BOX(toolbar), sep, FALSE, FALSE, 4);

    btn = gtk_button_new_with_label("Copy");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect(btn, "clicked", G_CALLBACK(file_copy), NULL);

    btn = gtk_button_new_with_label("Move");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect(btn, "clicked", G_CALLBACK(file_move), NULL);

    btn = gtk_button_new_with_label("Delete");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect(btn, "clicked", G_CALLBACK(file_delete), NULL);

    btn = gtk_button_new_with_label("Rename");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect(btn, "clicked", G_CALLBACK(file_rename), NULL);

    btn = gtk_button_new_with_label("New Folder");
    gtk_box_pack_start(GTK_BOX(toolbar), btn, FALSE, FALSE, 2);
    g_signal_connect(btn, "clicked", G_CALLBACK(file_mkdir), NULL);

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(vbox), paned, TRUE, TRUE, 0);

    state.sidebar = gtk_list_box_new();
    gtk_widget_set_size_request(state.sidebar, 160, -1);
    gtk_paned_pack1(GTK_PANED(paned), state.sidebar, FALSE, FALSE);
    g_signal_connect(state.sidebar, "row-activated", G_CALLBACK(on_sidebar_row), NULL);

    const char *places[] = {
        g_get_home_dir(),
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

    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(state.list_view)),
                                GTK_SELECTION_SINGLE);

    g_signal_connect(state.list_view, "row-activated", G_CALLBACK(on_row_activated), NULL);
    g_signal_connect(state.window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    GtkTargetEntry drag_targets[] = {
        { (char*)"text/uri-list", 0, 0 }
    };
    gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(state.list_view),
        GDK_BUTTON1_MASK, drag_targets, 1, GDK_ACTION_MOVE | GDK_ACTION_COPY);
    gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(state.list_view),
        drag_targets, 1, GDK_ACTION_MOVE | GDK_ACTION_COPY);

    g_signal_connect(state.list_view, "drag-data-get",
        G_CALLBACK(on_drag_data_get), NULL);
    g_signal_connect(state.list_view, "drag-data-received",
        G_CALLBACK(on_drag_data_received), NULL);
    g_signal_connect(state.list_view, "button-press-event",
        G_CALLBACK(on_button_press), NULL);

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    state.status_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(state.status_label), 0.0);
    gtk_container_set_border_width(GTK_CONTAINER(state.status_label), 3);
    gtk_box_pack_start(GTK_BOX(hbox), state.status_label, TRUE, TRUE, 0);

    load_directory();
    gtk_widget_show_all(state.window);
}

// ----- navigation callbacks -----
static void on_path_activate(void) {
    go_path(gtk_entry_get_text(GTK_ENTRY(state.path_entry)));
}
static void on_row_activated(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *col) {
    (void)view; (void)path; (void)col;
    open_selected();
}
static void on_sidebar_row(GtkListBox *box, GtkListBoxRow *row) {
    (void)box;
    GtkWidget *l = gtk_bin_get_child(GTK_BIN(row));
    go_path(gtk_label_get_text(GTK_LABEL(l)));
}

// ----- key press -----
static gboolean on_key_press(GtkWidget *w, GdkEventKey *ev) {
    (void)w;
    if (ev->keyval == GDK_KEY_BackSpace) { go_up(); return TRUE; }
    if (ev->keyval == GDK_KEY_Delete) { file_delete(); return TRUE; }
    if ((ev->state & GDK_CONTROL_MASK) && ev->keyval == GDK_KEY_c) { file_copy(); return TRUE; }
    if ((ev->state & GDK_CONTROL_MASK) && ev->keyval == GDK_KEY_x) { file_move(); return TRUE; }
    if ((ev->state & GDK_CONTROL_MASK) && ev->keyval == GDK_KEY_n) { file_mkdir(); return TRUE; }
    return FALSE;
}

// ----- right-click context menu -----
static GtkWidget* context_menu = NULL;

static void show_context_menu(GdkEventButton *ev) {
    if (context_menu) gtk_widget_destroy(context_menu);
    context_menu = gtk_menu_new();

    GtkWidget *item;

    item = gtk_menu_item_new_with_label("Open");
    g_signal_connect(item, "activate", G_CALLBACK(on_context_open), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), item);

    item = gtk_menu_item_new_with_label("Copy");
    g_signal_connect(item, "activate", G_CALLBACK(on_context_copy), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), item);

    item = gtk_menu_item_new_with_label("Move");
    g_signal_connect(item, "activate", G_CALLBACK(on_context_move), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), item);

    item = gtk_menu_item_new_with_label("Rename");
    g_signal_connect(item, "activate", G_CALLBACK(on_context_rename), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), item);

    item = gtk_menu_item_new_with_label("Delete");
    g_signal_connect(item, "activate", G_CALLBACK(on_context_delete), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), item);

    item = gtk_menu_item_new_with_label("Properties");
    g_signal_connect(item, "activate", G_CALLBACK(on_context_properties), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(context_menu), item);

    gtk_widget_show_all(context_menu);
    gtk_menu_popup_at_pointer(GTK_MENU(context_menu), (GdkEvent*)ev);
}

static gboolean on_button_press(GtkWidget *w, GdkEventButton *ev) {
    (void)w;
    if (ev->type == GDK_BUTTON_PRESS && ev->button == 3) {
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(state.list_view),
                                          ev->x, ev->y, &path, NULL, NULL, NULL)) {
            gtk_tree_selection_unselect_all(
                gtk_tree_view_get_selection(GTK_TREE_VIEW(state.list_view)));
            gtk_tree_selection_select_path(
                gtk_tree_view_get_selection(GTK_TREE_VIEW(state.list_view)), path);
            gtk_tree_path_free(path);
        }
        show_context_menu(ev);
        return TRUE;
    }
    if (ev->type == GDK_BUTTON_PRESS && ev->button == 1) {
        GtkTreePath *path;
        if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(state.list_view),
                                          ev->x, ev->y, &path, NULL, NULL, NULL)) {
            gtk_tree_path_free(path);
        }
    }
    return FALSE;
}

static void on_context_open(void)    { open_selected(); }
static void on_context_copy(void)    { file_copy(); }
static void on_context_move(void)    { file_move(); }
static void on_context_delete(void)  { file_delete(); }
static void on_context_rename(void)  { file_rename(); }
static void on_context_properties(void) { show_properties(); }

// ----- drag source -----
static void on_drag_data_get(GtkWidget *w, GdkDragContext *ctx,
                              GtkSelectionData *data, guint info, guint time) {
    (void)w; (void)ctx; (void)info; (void)time;
    char *path = get_selected_path();
    if (path) {
        char *uri = g_filename_to_uri(path, NULL, NULL);
        if (uri) {
            gtk_selection_data_set(data, gtk_selection_data_get_target(data),
                                   8, (const guchar*)uri, strlen(uri));
            g_free(uri);
        }
        g_free(path);
    }
}

// ----- drop destination -----
static void on_drag_data_received(GtkWidget *w, GdkDragContext *ctx,
                                   gint x, gint y, GtkSelectionData *data,
                                   guint info, guint time) {
    (void)w; (void)ctx; (void)x; (void)y; (void)info;
    const guchar *raw = gtk_selection_data_get_data(data);
    if (raw && raw[0])
        drop_files((const char*)raw);
    gtk_drag_finish(ctx, TRUE, FALSE, time);
}

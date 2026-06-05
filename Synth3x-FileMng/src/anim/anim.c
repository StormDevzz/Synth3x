#include "../fm.h"

static GtkCssProvider *css_provider = NULL;
static const char *anim_css =
    "treeview.view {"
    "  transition: all 150ms ease;"
    "}"
    "treeview.view:hover {"
    "  background-color: rgba(80, 140, 220, 0.12);"
    "}"
    "treeview.view:selected:hover {"
    "  background-color: rgba(80, 140, 220, 0.35);"
    "}"
    "treeview.view button {"
    "  transition: all 120ms ease;"
    "}"
    "treeview.view button:hover {"
    "  background-color: rgba(80, 140, 220, 0.20);"
    "}"
    ".anim-border {"
    "  border-left: 3px solid rgba(80, 140, 220, 0.0);"
    "  transition: border-left 200ms ease;"
    "  padding-left: 2px;"
    "}"
    ".anim-border:hover {"
    "  border-left: 3px solid rgba(80, 140, 220, 1.0);"
    "}";

void init_animations(void) {
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider, anim_css, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkStyleContext *ctx = gtk_widget_get_style_context(state.list_view);
    gtk_style_context_add_class(ctx, "anim-border");

    state.anim_enabled = TRUE;
}

void toggle_animations(void) {
    state.anim_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(state.anim_btn));

    if (state.anim_enabled) {
        if (css_provider) {
            gtk_style_context_add_provider_for_screen(
                gdk_screen_get_default(),
                GTK_STYLE_PROVIDER(css_provider),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        }
    } else {
        if (css_provider) {
            gtk_style_context_remove_provider_for_screen(
                gdk_screen_get_default(),
                GTK_STYLE_PROVIDER(css_provider));
        }
    }

    update_status("animations %s", state.anim_enabled ? "on" : "off");
}

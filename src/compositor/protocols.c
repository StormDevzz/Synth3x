/* Synth3x Compositor — Protocol extensions
 * xdg-shell (stable), wlr-layer-shell (unstable), xdg-decoration (unstable)
 */

#include "compositor.h"
#include "protocols.h"
#include <stdio.h>
#include <stdlib.h>

/* ─── Forward declarations of helpers ─── */
static xdg_surface_t *xdg_surface_create(wl_surface_t *surf);
static void xdg_surface_destroy(xdg_surface_t *xs);
static void xdg_toplevel_send_configure(xdg_toplevel_t *t);
static void xdg_popup_send_configure(xdg_popup_t *p);
static void layer_surface_send_configure(zwlr_layer_surface_t *ls);

/* ─── Utility: find client object by id ─── */
static wl_object_t *find_obj(wl_client_t *c, uint32_t id) {
    for (int i = 0; i < c->obj_count; i++)
        if (c->objects[i].id == id)
            return &c->objects[i];
    return NULL;
}

/* ─── Interface references (declared in wl_server.c) ─── */
extern const wl_interface_t iface_xdg_wm_base;
extern const wl_interface_t iface_xdg_positioner;
extern const wl_interface_t iface_xdg_surface;
extern const wl_interface_t iface_xdg_toplevel;
extern const wl_interface_t iface_xdg_popup;
extern const wl_interface_t iface_zwlr_layer_shell;
extern const wl_interface_t iface_zwlr_layer_surface;
extern const wl_interface_t iface_zxdg_decoration_manager;
extern const wl_interface_t iface_zxdg_toplevel_decoration;

/* ─── Global state ─── */
static compositor_t *g_comp = NULL;
static xdg_surface_t *xdg_surfaces = NULL;

/* ═══════════════════════════════════════════════
 *    xdg_wm_base handlers
 * ═══════════════════════════════════════════════ */

static void handle_xdg_wm_base_destroy(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id); if (o) o->interface = NULL;
}

static void handle_xdg_wm_base_create_positioner(wl_client_t *c, arg_reader_t *r) {
    uint32_t wm_id, pos_id;
    arg_read_object(r, &wm_id);
    arg_read_new_id(r, &pos_id);
    xdg_positioner_t *pos = calloc(1, sizeof(xdg_positioner_t));
    if (pos) {
        pos->anchor = XDG_ANCHOR_NONE;
        pos->gravity = XDG_GRAVITY_NONE;
        wl_object_t *o = find_obj(c, pos_id);
        if (o) o->implementation = pos;
    }
    client_add_object(c, pos_id, &iface_xdg_positioner, pos);
}

static void handle_xdg_wm_base_get_xdg_surface(wl_client_t *c, arg_reader_t *r) {
    uint32_t wm_id, surf_id, xdg_surf_id;
    arg_read_object(r, &wm_id);
    arg_read_object(r, &surf_id);
    arg_read_new_id(r, &xdg_surf_id);
    
    wl_object_t *so = find_obj(c, surf_id);
    if (!so || !so->implementation) return;
    wl_surface_t *surf = (wl_surface_t *)so->implementation;
    
    xdg_surface_t *xs = xdg_surface_create(surf);
    if (!xs) return;
    
    wl_object_t *o = client_add_object(c, xdg_surf_id, &iface_xdg_surface, xs);
    if (o) o->implementation = xs;
}

static void handle_xdg_wm_base_pong(wl_client_t *c, arg_reader_t *r) {
    uint32_t id, serial;
    arg_read_object(r, &id);
    arg_read_uint(r, &serial);
    /* No-op: ping response acknowledgement */
}

/* ═══════════════════════════════════════════════
 *    xdg_positioner handlers
 * ═══════════════════════════════════════════════ */

static void handle_xdg_positioner_set_size(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_positioner_t *p = (xdg_positioner_t *)o->implementation;
        arg_read_int(r, &p->size_w);
        arg_read_int(r, &p->size_h);
    } else { int32_t a,b; arg_read_int(r,&a); arg_read_int(r,&b); }
}

static void handle_xdg_positioner_set_anchor_rect(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_positioner_t *p = (xdg_positioner_t *)o->implementation;
        arg_read_int(r, &p->anchor_rect_x);
        arg_read_int(r, &p->anchor_rect_y);
        arg_read_int(r, &p->anchor_rect_w);
        arg_read_int(r, &p->anchor_rect_h);
    } else { int32_t a[4]; for(int i=0;i<4;i++) arg_read_int(r,&a[i]); }
}

static void handle_xdg_positioner_set_anchor(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    uint32_t v; arg_read_uint(r, &v);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) ((xdg_positioner_t*)o->implementation)->anchor = v;
}

static void handle_xdg_positioner_set_gravity(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    uint32_t v; arg_read_uint(r, &v);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) ((xdg_positioner_t*)o->implementation)->gravity = v;
}

static void handle_xdg_positioner_set_constraint_adjustment(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    uint32_t v; arg_read_uint(r, &v);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) ((xdg_positioner_t*)o->implementation)->constraint_adjust = v;
}

static void handle_xdg_positioner_set_offset(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_positioner_t *p = (xdg_positioner_t *)o->implementation;
        arg_read_int(r, &p->offset_x);
        arg_read_int(r, &p->offset_y);
    } else { int32_t a,b; arg_read_int(r,&a); arg_read_int(r,&b); }
}

static void handle_xdg_positioner_set_reactive(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) ((xdg_positioner_t*)o->implementation)->reactive = 1;
}

static void handle_xdg_positioner_set_parent_size(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_positioner_t *p = (xdg_positioner_t *)o->implementation;
        arg_read_int(r, &p->parent_w);
        arg_read_int(r, &p->parent_h);
    } else { int32_t a,b; arg_read_int(r,&a); arg_read_int(r,&b); }
}

static void handle_xdg_positioner_set_parent_configure(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    uint32_t v; arg_read_uint(r, &v);
    /* No-op: serial tracking */
}

static void handle_xdg_positioner_done(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) ((xdg_positioner_t*)o->implementation)->done = 1;
}

static void handle_xdg_positioner_destroy(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) { free(o->implementation); o->implementation = NULL; }
}

/* ═══════════════════════════════════════════════
 *    xdg_surface handlers
 * ═══════════════════════════════════════════════ */

static void handle_xdg_surface_destroy(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_surface_t *xs = (xdg_surface_t *)o->implementation;
        xdg_surface_destroy(xs);
        o->implementation = NULL;
    }
}

static void handle_xdg_surface_get_toplevel(wl_client_t *c, arg_reader_t *r) {
    uint32_t xdg_id, toplevel_id;
    arg_read_object(r, &xdg_id);
    arg_read_new_id(r, &toplevel_id);
    
    wl_object_t *o = find_obj(c, xdg_id);
    if (!o || !o->implementation) return;
    xdg_surface_t *xs = (xdg_surface_t *)o->implementation;
    
    xdg_toplevel_t *t = calloc(1, sizeof(xdg_toplevel_t));
    if (!t) return;
    t->xdg_surface = xs;
    t->min_w = 0; t->min_h = 0;
    t->max_w = 0; t->max_h = 0;
    t->next_w = 800;
    t->next_h = 600;
    t->maximized = 0;
    t->fullscreen = 0;
    xs->is_toplevel = 1;
    xs->role.toplevel = t;
    
    wl_object_t *to = client_add_object(c, toplevel_id, &iface_xdg_toplevel, t);
    if (to) to->implementation = t;
    
    /* Send initial configure */
    xdg_toplevel_send_configure(t);
}

static void handle_xdg_surface_get_popup(wl_client_t *c, arg_reader_t *r) {
    uint32_t xdg_id, pos_id, popup_id;
    arg_read_object(r, &xdg_id);
    arg_read_object(r, &pos_id);
    arg_read_new_id(r, &popup_id);
    
    wl_object_t *o = find_obj(c, xdg_id);
    if (!o || !o->implementation) return;
    xdg_surface_t *xs = (xdg_surface_t *)o->implementation;
    
    xdg_popup_t *pop = calloc(1, sizeof(xdg_popup_t));
    if (!pop) return;
    pop->xdg_surface = xs;
    
    wl_object_t *po = find_obj(c, pos_id);
    if (po && po->implementation)
        memcpy(&pop->pos, po->implementation, sizeof(xdg_positioner_t));
    
    xs->is_toplevel = 0;
    xs->role.popup = pop;
    
    wl_object_t *pto = client_add_object(c, popup_id, &iface_xdg_popup, pop);
    if (pto) pto->implementation = pop;
    
    xdg_popup_send_configure(pop);
}

static void handle_xdg_surface_set_window_geometry(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    /* No-op: geometry hint */
    int32_t a[4]; for(int i=0;i<4;i++) arg_read_int(r,&a[i]);
}

static void handle_xdg_surface_ack_configure(wl_client_t *c, arg_reader_t *r) {
    uint32_t id, serial;
    arg_read_object(r, &id);
    arg_read_uint(r, &serial);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_surface_t *xs = (xdg_surface_t *)o->implementation;
        if (serial == xs->configure_serial)
            xs->configured = 1;
    }
}

/* ═══════════════════════════════════════════════
 *    xdg_toplevel handlers
 * ═══════════════════════════════════════════════ */

static void handle_xdg_toplevel_destroy(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_toplevel_t *t = (xdg_toplevel_t *)o->implementation;
        t->closed = 1;
        free(t);
        o->implementation = NULL;
    }
}

static void handle_xdg_toplevel_set_parent(wl_client_t *c, arg_reader_t *r) {
    uint32_t id, parent_id;
    arg_read_object(r, &id);
    arg_read_object(r, &parent_id);
    /* No-op: parenting */
}

static void handle_xdg_toplevel_set_title(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_toplevel_t *t = (xdg_toplevel_t *)o->implementation;
        arg_read_string(r, t->title, sizeof(t->title));
    } else { char buf[64]; arg_read_string(r, buf, sizeof(buf)); }
}

static void handle_xdg_toplevel_set_app_id(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_toplevel_t *t = (xdg_toplevel_t *)o->implementation;
        arg_read_string(r, t->app_id, sizeof(t->app_id));
    } else { char buf[64]; arg_read_string(r, buf, sizeof(buf)); }
}

static void handle_xdg_toplevel_show_window_menu(wl_client_t *c, arg_reader_t *r) {
    uint32_t id, seat_id;
    arg_read_object(r, &id);
    arg_read_object(r, &seat_id);
    int32_t x, y; arg_read_int(r, &x); arg_read_int(r, &y);
    uint32_t serial; arg_read_uint(r, &serial);
    /* No-op: window menu not implemented */
}

static void handle_xdg_toplevel_move(wl_client_t *c, arg_reader_t *r) {
    uint32_t id, seat_id, serial;
    arg_read_object(r, &id);
    arg_read_object(r, &seat_id);
    arg_read_uint(r, &serial);
    /* No-op: interactive move */
}

static void handle_xdg_toplevel_resize(wl_client_t *c, arg_reader_t *r) {
    uint32_t id, seat_id, serial, edges;
    arg_read_object(r, &id);
    arg_read_object(r, &seat_id);
    arg_read_uint(r, &serial);
    arg_read_uint(r, &edges);
    /* No-op: interactive resize */
}

static void handle_xdg_toplevel_set_max_size(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_toplevel_t *t = (xdg_toplevel_t *)o->implementation;
        arg_read_int(r, &t->max_w);
        arg_read_int(r, &t->max_h);
    } else { int32_t a,b; arg_read_int(r,&a); arg_read_int(r,&b); }
}

static void handle_xdg_toplevel_set_min_size(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_toplevel_t *t = (xdg_toplevel_t *)o->implementation;
        arg_read_int(r, &t->min_w);
        arg_read_int(r, &t->min_h);
    } else { int32_t a,b; arg_read_int(r,&a); arg_read_int(r,&b); }
}

static void handle_xdg_toplevel_set_maximized(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_toplevel_t *t = (xdg_toplevel_t *)o->implementation;
        t->maximized = 1;
        xdg_toplevel_send_configure(t);
    }
}

static void handle_xdg_toplevel_unset_maximized(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_toplevel_t *t = (xdg_toplevel_t *)o->implementation;
        t->maximized = 0;
        xdg_toplevel_send_configure(t);
    }
}

static void handle_xdg_toplevel_set_fullscreen(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    /* Skip output arg */
    uint32_t output_id; arg_read_object(r, &output_id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_toplevel_t *t = (xdg_toplevel_t *)o->implementation;
        t->fullscreen = 1;
        xdg_toplevel_send_configure(t);
    }
}

static void handle_xdg_toplevel_unset_fullscreen(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_toplevel_t *t = (xdg_toplevel_t *)o->implementation;
        t->fullscreen = 0;
        xdg_toplevel_send_configure(t);
    }
}

static void handle_xdg_toplevel_set_minimized(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        xdg_toplevel_t *t = (xdg_toplevel_t *)o->implementation;
        t->minimized = 1;
    }
}

/* ═══════════════════════════════════════════════
 *    xdg_popup handlers
 * ═══════════════════════════════════════════════ */

static void handle_xdg_popup_destroy(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) { free(o->implementation); o->implementation = NULL; }
}

static void handle_xdg_popup_grab(wl_client_t *c, arg_reader_t *r) {
    uint32_t id, seat_id, serial;
    arg_read_object(r, &id);
    arg_read_object(r, &seat_id);
    arg_read_uint(r, &serial);
    /* No-op: grab */
}

static void handle_xdg_popup_reposition(wl_client_t *c, arg_reader_t *r) {
    uint32_t id, pos_id;
    arg_read_object(r, &id);
    arg_read_object(r, &pos_id);
    uint32_t token; arg_read_uint(r, &token);
    /* No-op: reposition */
}

/* ═══════════════════════════════════════════════
 *    wlr-layer-shell handlers
 * ═══════════════════════════════════════════════ */

static void handle_zwlr_layer_shell_destroy(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
}

static void handle_zwlr_layer_shell_get_layer_surface(wl_client_t *c, arg_reader_t *r) {
    uint32_t shell_id, surf_id, layer_surf_id;
    uint32_t layer, ns;
    arg_read_object(r, &shell_id);
    arg_read_object(r, &surf_id);
    arg_read_new_id(r, &layer_surf_id);
    arg_read_uint(r, &layer);
    arg_read_string(r, (char*)&ns, 4); /* namespace */
    char ns_buf[64]; arg_read_string(r, ns_buf, sizeof(ns_buf));
    
    wl_object_t *so = find_obj(c, surf_id);
    if (!so || !so->implementation) return;
    wl_surface_t *surf = (wl_surface_t *)so->implementation;
    
    zwlr_layer_surface_t *ls = calloc(1, sizeof(zwlr_layer_surface_t));
    if (!ls) return;
    ls->surface = surf;
    ls->layer = layer;
    ls->anchor = 0;
    ls->exclusive_zone = 0;
    ls->margin_t = ls->margin_b = ls->margin_l = ls->margin_r = 0;
    ls->keyboard_interactive = 0;
    ls->w = surf->width;
    ls->h = surf->height;
    surf->layer = ls;
    
    wl_object_t *lo = client_add_object(c, layer_surf_id, &iface_zwlr_layer_surface, ls);
    if (lo) lo->implementation = ls;
    
    layer_surface_send_configure(ls);
}

/* wlr_layer_surface_v1 handlers */

static void handle_zwlr_layer_surface_destroy(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        zwlr_layer_surface_t *ls = (zwlr_layer_surface_t *)o->implementation;
        ls->closed = 1;
        if (ls->surface) ls->surface->layer = NULL;
        free(ls);
        o->implementation = NULL;
    }
}

static void handle_zwlr_layer_surface_set_size(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        zwlr_layer_surface_t *ls = (zwlr_layer_surface_t *)o->implementation;
        arg_read_uint(r, (uint32_t*)&ls->w);
        arg_read_uint(r, (uint32_t*)&ls->h);
    } else { uint32_t w,h; arg_read_uint(r,&w); arg_read_uint(r,&h); }
}

static void handle_zwlr_layer_surface_set_anchor(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    uint32_t v; arg_read_uint(r, &v);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) ((zwlr_layer_surface_t*)o->implementation)->anchor = v;
}

static void handle_zwlr_layer_surface_set_exclusive_zone(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    int32_t v; arg_read_int(r, &v);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) ((zwlr_layer_surface_t*)o->implementation)->exclusive_zone = v;
}

static void handle_zwlr_layer_surface_set_margin(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        zwlr_layer_surface_t *ls = (zwlr_layer_surface_t *)o->implementation;
        arg_read_int(r, &ls->margin_t);
        arg_read_int(r, &ls->margin_b);
        arg_read_int(r, &ls->margin_l);
        arg_read_int(r, &ls->margin_r);
    } else { int32_t a[4]; for(int i=0;i<4;i++) arg_read_int(r,&a[i]); }
}

static void handle_zwlr_layer_surface_set_keyboard_interactivity(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    uint32_t v; arg_read_uint(r, &v);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) ((zwlr_layer_surface_t*)o->implementation)->keyboard_interactive = v;
}

static void handle_zwlr_layer_surface_get_popup(wl_client_t *c, arg_reader_t *r) {
    uint32_t ls_id, pos_id, popup_id;
    arg_read_object(r, &ls_id);
    arg_read_object(r, &pos_id);
    arg_read_new_id(r, &popup_id);
    /* No-op: popup creation for layer surfaces */
}

static void handle_zwlr_layer_surface_ack_configure(wl_client_t *c, arg_reader_t *r) {
    uint32_t id, serial;
    arg_read_object(r, &id);
    arg_read_uint(r, &serial);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) {
        zwlr_layer_surface_t *ls = (zwlr_layer_surface_t *)o->implementation;
        if (serial == ls->configure_serial)
            ls->configured = 1;
    }
}

static void handle_zwlr_layer_surface_set_layer(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    uint32_t v; arg_read_uint(r, &v);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) ((zwlr_layer_surface_t*)o->implementation)->layer = v;
}

/* ═══════════════════════════════════════════════
 *    xdg-decoration handlers
 * ═══════════════════════════════════════════════ */

static void handle_zxdg_decoration_mgr_destroy(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
}

static void handle_zxdg_decoration_mgr_get_toplevel_decoration(wl_client_t *c, arg_reader_t *r) {
    uint32_t mgr_id, toplevel_id, decor_id;
    arg_read_object(r, &mgr_id);
    arg_read_object(r, &toplevel_id);
    arg_read_new_id(r, &decor_id);
    
    wl_object_t *to = find_obj(c, toplevel_id);
    if (!to || !to->implementation) return;
    xdg_toplevel_t *toplevel = (xdg_toplevel_t *)to->implementation;
    
    zxdg_toplevel_decoration_t *d = calloc(1, sizeof(zxdg_toplevel_decoration_t));
    if (!d) return;
    d->toplevel = toplevel;
    d->mode = ZXDG_TOPLEVEL_DECOR_MODE_SERVER;
    toplevel->decoration = d;
    
    wl_object_t *deo = client_add_object(c, decor_id, &iface_zxdg_toplevel_decoration, d);
    if (deo) deo->implementation = d;
    
    /* Send configure: server-side decorations */
    struct __attribute__((packed)) { uint32_t mode; } ev = { ZXDG_TOPLEVEL_DECOR_MODE_SERVER };
    wl_send_event(c, decor_id, ZXDG_TOPLEVEL_DECOR_CONFIGURE, &ev, sizeof(ev));
}

static void handle_zxdg_toplevel_decor_destroy(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation) { free(o->implementation); o->implementation = NULL; }
}

static void handle_zxdg_toplevel_decor_set_mode(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    uint32_t v; arg_read_uint(r, &v);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation)
        ((zxdg_toplevel_decoration_t*)o->implementation)->mode = v;
}

static void handle_zxdg_toplevel_decor_unset_mode(wl_client_t *c, arg_reader_t *r) {
    uint32_t id; arg_read_object(r, &id);
    wl_object_t *o = find_obj(c, id);
    if (o && o->implementation)
        ((zxdg_toplevel_decoration_t*)o->implementation)->mode = ZXDG_TOPLEVEL_DECOR_MODE_SERVER;
}

/* ═══════════════════════════════════════════════
 *    Configure sending (helpers)
 * ═══════════════════════════════════════════════ */

static uint32_t next_serial(void) {
    static uint32_t serial = 1;
    return serial++;
}

static void xdg_toplevel_send_configure(xdg_toplevel_t *t) {
    if (!t || !t->xdg_surface) return;
    xdg_surface_t *xs = t->xdg_surface;
    wl_client_t *c = xs->surface->client;
    uint32_t surf_obj_id = 0;
    
    /* Find the xdg_surface object id in client */
    for (int i = 0; i < c->obj_count; i++) {
        if (c->objects[i].implementation == xs) {
            surf_obj_id = c->objects[i].id;
            break;
        }
    }
    if (!surf_obj_id) return;
    
    uint32_t serial = next_serial();
    xs->configure_serial = serial;
    
    int w = t->maximized ? g_comp->drm.width : t->next_w;
    int h = t->maximized ? g_comp->drm.height : t->next_h;
    if (t->fullscreen) { w = g_comp->drm.width; h = g_comp->drm.height; }
    
    /* xdg_surface.configure */
    struct __attribute__((packed)) { uint32_t serial; } s_ev = { serial };
    wl_send_event(c, surf_obj_id, XDG_SURFACE_CONFIGURE, &s_ev, sizeof(s_ev));
    
    /* Find xdg_toplevel object id */
    uint32_t top_obj_id = 0;
    for (int i = 0; i < c->obj_count; i++) {
        if (c->objects[i].implementation == t) {
            top_obj_id = c->objects[i].id;
            break;
        }
    }
    if (!top_obj_id) return;
    
    /* xdg_toplevel.configure */
    struct __attribute__((packed)) {
        int32_t w, h;
        uint32_t states[16];
        uint32_t states_count;
    } t_ev;
    memset(&t_ev, 0, sizeof(t_ev));
    t_ev.w = w;
    t_ev.h = h;
    /* Add state array */
    int st_count = 0;
    if (t->maximized) t_ev.states[st_count++] = 1; /* XDG_TOPLEVEL_STATE_MAXIMIZED */
    if (t->fullscreen) t_ev.states[st_count++] = 2; /* XDG_TOPLEVEL_STATE_FULLSCREEN */
    if (t->minimized) t_ev.states[st_count++] = 4; /* XDG_TOPLEVEL_STATE_MINIMIZED */
    t_ev.states_count = st_count;
    
    wl_send_event(c, top_obj_id, XDG_TOPLEVEL_CONFIGURE, &t_ev,
                  8 + st_count * 4);
    
    fprintf(stderr, "xdg_toplevel configure: %dx%d serial=%u\n", w, h, serial);
}

static void xdg_popup_send_configure(xdg_popup_t *p) {
    if (!p || !p->xdg_surface) return;
    xdg_surface_t *xs = p->xdg_surface;
    wl_client_t *c = xs->surface->client;
    
    uint32_t serial = next_serial();
    xs->configure_serial = serial;
    
    /* xdg_surface.configure */
    struct __attribute__((packed)) { uint32_t s; } s_ev = { serial };
    
    uint32_t surf_obj_id = 0;
    for (int i = 0; i < c->obj_count; i++) {
        if (c->objects[i].implementation == xs) {
            surf_obj_id = c->objects[i].id;
            break; }
    }
    if (surf_obj_id)
        wl_send_event(c, surf_obj_id, XDG_SURFACE_CONFIGURE, &s_ev, sizeof(s_ev));
    
    /* xdg_popup.configure */
    uint32_t pop_obj_id = 0;
    for (int i = 0; i < c->obj_count; i++) {
        if (c->objects[i].implementation == p) {
            pop_obj_id = c->objects[i].id;
            break; }
    }
    if (!pop_obj_id) return;
    
    struct __attribute__((packed)) {
        int32_t x, y, w, h;
    } pop_ev = { p->pos.offset_x, p->pos.offset_y, p->pos.size_w, p->pos.size_h };
    wl_send_event(c, pop_obj_id, XDG_POPUP_CONFIGURE, &pop_ev, sizeof(pop_ev));
}

static void layer_surface_send_configure(zwlr_layer_surface_t *ls) {
    if (!ls) return;
    wl_client_t *c = ls->surface->client;
    
    uint32_t serial = next_serial();
    ls->configure_serial = serial;
    
    int w = ls->w ? ls->w : g_comp->drm.width;
    int h = ls->h ? ls->h : g_comp->drm.height;
    
    struct __attribute__((packed)) {
        uint32_t serial;
        int32_t w, h;
    } ev = { serial, w, h };
    
    uint32_t obj_id = 0;
    for (int i = 0; i < c->obj_count; i++) {
        if (c->objects[i].implementation == ls) {
            obj_id = c->objects[i].id;
            break;
        }
    }
    if (obj_id)
        wl_send_event(c, obj_id, ZWLR_LAYER_SURFACE_CONFIGURE, &ev, sizeof(ev));
}

/* ═══════════════════════════════════════════════
 *    xdg_surface lifecycle
 * ═══════════════════════════════════════════════ */

static xdg_surface_t *xdg_surface_create(wl_surface_t *surf) {
    xdg_surface_t *xs = calloc(1, sizeof(xdg_surface_t));
    if (!xs) return NULL;
    xs->surface = surf;
    xs->configured = 0;
    surf->xdg = xs;
    
    xs->next = xdg_surfaces;
    xdg_surfaces = xs;
    return xs;
}

static void xdg_surface_destroy(xdg_surface_t *xs) {
    if (!xs) return;
    if (xs->surface) xs->surface->xdg = NULL;
    
    if (xs->is_toplevel && xs->role.toplevel) {
        free(xs->role.toplevel);
    } else if (!xs->is_toplevel && xs->role.popup) {
        free(xs->role.popup);
    }
    
    /* Remove from global list */
    xdg_surface_t **pp = &xdg_surfaces;
    while (*pp) {
        if (*pp == xs) { *pp = xs->next; break; }
        pp = &(*pp)->next;
    }
    
    xs->destroyed = 1;
    free(xs);
}

/* ═══════════════════════════════════════════════
 *    Dispatch table construction
 * ═══════════════════════════════════════════════ */

int protocols_dispatch(wl_client_t *c, wl_object_t *obj,
                       uint32_t opcode, arg_reader_t *r) {
    typedef void (*h)(wl_client_t*, arg_reader_t*);
    
    if (obj->interface == &iface_xdg_wm_base) {
        static h htab[] = {
            handle_xdg_wm_base_destroy,
            handle_xdg_wm_base_create_positioner,
            handle_xdg_wm_base_get_xdg_surface,
            handle_xdg_wm_base_pong
        };
        if (opcode < 4 && htab[opcode]) { htab[opcode](c, r); return 1; }
    }
    if (obj->interface == &iface_xdg_positioner) {
        static h htab[] = {
            handle_xdg_positioner_destroy,
            handle_xdg_positioner_set_size,
            handle_xdg_positioner_set_anchor_rect,
            handle_xdg_positioner_set_anchor,
            handle_xdg_positioner_set_gravity,
            handle_xdg_positioner_set_constraint_adjustment,
            handle_xdg_positioner_set_offset,
            handle_xdg_positioner_set_reactive,
            handle_xdg_positioner_set_parent_size,
            handle_xdg_positioner_set_parent_configure,
            handle_xdg_positioner_done
        };
        if (opcode < 11 && htab[opcode]) { htab[opcode](c, r); return 1; }
    }
    if (obj->interface == &iface_xdg_surface) {
        static h htab[] = {
            handle_xdg_surface_destroy,
            handle_xdg_surface_get_toplevel,
            handle_xdg_surface_get_popup,
            handle_xdg_surface_set_window_geometry,
            handle_xdg_surface_ack_configure
        };
        if (opcode < 5 && htab[opcode]) { htab[opcode](c, r); return 1; }
    }
    if (obj->interface == &iface_xdg_toplevel) {
        static h htab[] = {
            handle_xdg_toplevel_destroy,
            handle_xdg_toplevel_set_parent,
            handle_xdg_toplevel_set_title,
            handle_xdg_toplevel_set_app_id,
            handle_xdg_toplevel_show_window_menu,
            handle_xdg_toplevel_move,
            handle_xdg_toplevel_resize,
            handle_xdg_toplevel_set_max_size,
            handle_xdg_toplevel_set_min_size,
            handle_xdg_toplevel_set_maximized,
            handle_xdg_toplevel_unset_maximized,
            handle_xdg_toplevel_set_fullscreen,
            handle_xdg_toplevel_unset_fullscreen,
            handle_xdg_toplevel_set_minimized
        };
        if (opcode < 14 && htab[opcode]) { htab[opcode](c, r); return 1; }
    }
    if (obj->interface == &iface_xdg_popup) {
        static h htab[] = {
            handle_xdg_popup_destroy,
            handle_xdg_popup_grab,
            handle_xdg_popup_reposition
        };
        if (opcode < 3 && htab[opcode]) { htab[opcode](c, r); return 1; }
    }
    if (obj->interface == &iface_zwlr_layer_shell) {
        static h htab[] = {
            handle_zwlr_layer_shell_destroy,
            handle_zwlr_layer_shell_get_layer_surface
        };
        if (opcode < 2 && htab[opcode]) { htab[opcode](c, r); return 1; }
    }
    if (obj->interface == &iface_zwlr_layer_surface) {
        static h htab[] = {
            handle_zwlr_layer_surface_destroy,
            handle_zwlr_layer_surface_set_size,
            handle_zwlr_layer_surface_set_anchor,
            handle_zwlr_layer_surface_set_exclusive_zone,
            handle_zwlr_layer_surface_set_margin,
            handle_zwlr_layer_surface_set_keyboard_interactivity,
            handle_zwlr_layer_surface_get_popup,
            handle_zwlr_layer_surface_ack_configure,
            handle_zwlr_layer_surface_set_layer
        };
        if (opcode < 9 && htab[opcode]) { htab[opcode](c, r); return 1; }
    }
    if (obj->interface == &iface_zxdg_decoration_manager) {
        static h htab[] = {
            handle_zxdg_decoration_mgr_destroy,
            handle_zxdg_decoration_mgr_get_toplevel_decoration
        };
        if (opcode < 2 && htab[opcode]) { htab[opcode](c, r); return 1; }
    }
    if (obj->interface == &iface_zxdg_toplevel_decoration) {
        static h htab[] = {
            handle_zxdg_toplevel_decor_destroy,
            handle_zxdg_toplevel_decor_set_mode,
            handle_zxdg_toplevel_decor_unset_mode
        };
        if (opcode < 3 && htab[opcode]) { htab[opcode](c, r); return 1; }
    }
    return 0;
}

/* ═══════════════════════════════════════════════
 *    Public API
 * ═══════════════════════════════════════════════ */

void protocols_init(compositor_t *c) {
    g_comp = c;
    xdg_surfaces = NULL;
}

void protocols_register_globals(wl_client_t *client, uint32_t registry_id) {
    /* Globals are registered in wl_server.c's handle_display_get_registry */
}

void protocols_bind(wl_client_t *client, uint32_t name, uint32_t id,
                    const char *iface, uint32_t version) {
    /* Bind handled in wl_server.c's handle_registry_bind */
}

void protocols_cleanup_client(compositor_t *c, wl_client_t *cl) {
    /* Walk all xdg surfaces and clean up ones belonging to this client */
    xdg_surface_t **pp = &xdg_surfaces;
    while (*pp) {
        xdg_surface_t *xs = *pp;
        if (xs->surface && xs->surface->client == cl) {
            *pp = xs->next;
            if (xs->surface) xs->surface->xdg = NULL;
            if (xs->is_toplevel && xs->role.toplevel) free(xs->role.toplevel);
            else if (!xs->is_toplevel && xs->role.popup) free(xs->role.popup);
            free(xs);
        } else {
            pp = &(*pp)->next;
        }
    }
}

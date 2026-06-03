/* Synth3x Compositor — Wayland Protocol Server
 * Implements the core Wayland display server protocol.
 * Supports: wl_display, wl_registry, wl_compositor, wl_surface,
 *           wl_shell, wl_shm, wl_seat, wl_pointer, wl_keyboard,
 *           wl_output, wl_callback, wl_region, wl_data_device.
 */

#include "compositor.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

/* ─── Interface names ─── */
static const wl_interface_t iface_wl_display = {
    "wl_display", 1, 2, 2
};
static const wl_interface_t iface_wl_registry = {
    "wl_registry", 1, 1, 2
};
static const wl_interface_t iface_wl_compositor = {
    "wl_compositor", 4, 2, 0
};
static const wl_interface_t iface_wl_surface = {
    "wl_surface", 4, 9, 0
};
static const wl_interface_t iface_wl_shell = {
    "wl_shell", 1, 1, 0
};
static const wl_interface_t iface_wl_shell_surface = {
    "wl_shell_surface", 1, 10, 3
};
static const wl_interface_t iface_wl_shm = {
    "wl_shm", 1, 1, 1
};
static const wl_interface_t iface_wl_shm_pool = {
    "wl_shm_pool", 1, 3, 0
};
static const wl_interface_t iface_wl_buffer = {
    "wl_buffer", 1, 1, 0
};
static const wl_interface_t iface_wl_callback = {
    "wl_callback", 1, 0, 1
};
static const wl_interface_t iface_wl_seat = {
    "wl_seat", 7, 3, 2
};
static const wl_interface_t iface_wl_pointer = {
    "wl_pointer", 7, 0, 9
};
static const wl_interface_t iface_wl_keyboard = {
    "wl_keyboard", 7, 0, 6
};
static const wl_interface_t iface_wl_touch = {
    "wl_touch", 7, 0, 5
};
static const wl_interface_t iface_wl_output = {
    "wl_output", 3, 0, 4
};
static const wl_interface_t iface_wl_region = {
    "wl_region", 1, 3, 0
};
static const wl_interface_t iface_wl_data_device = {
    "wl_data_device", 3, 0, 5
};
static const wl_interface_t iface_wl_subcompositor = {
    "wl_subcompositor", 1, 2, 0
};

/* ─── Helper: send Wayland message ─── */
void wl_send_event(wl_client_t *client, uint32_t id, uint32_t opcode,
                   const void *data, uint32_t len) {
    if (!client || client->fd < 0) return;
    
    uint8_t buf[4096];
    if (len + 8 > sizeof(buf)) len = sizeof(buf) - 8;
    
    wl_msg_header_t *hdr = (wl_msg_header_t *)buf;
    hdr->object_id = id;
    hdr->opcode = opcode;
    hdr->size = 8 + len;
    
    if (len > 0) memcpy(buf + 8, data, len);
    
    /* Pad to 4 bytes */
    uint32_t total = hdr->size;
    while (total & 3) {
        buf[total++] = 0;
        hdr->size = total;
    }
    
    write(client->fd, buf, total);
}

void wl_post_error(wl_client_t *client, uint32_t id, uint32_t code, const char *msg) {
    struct __attribute__((packed)) {
        uint32_t object_id;
        uint32_t code;
        char msg[128];
    } err;
    err.object_id = id;
    err.code = code;
    memset(err.msg, 0, sizeof(err.msg));
    strncpy(err.msg, msg, sizeof(err.msg) - 1);
    wl_send_event(client, WL_DISPLAY_ID, WL_DISPLAY_ERROR, &err, sizeof(err));
    
    /* Close connection after error */
    shutdown(client->fd, SHUT_RDWR);
}

/* ─── Client object lookup ─── */
static wl_object_t *client_find_object(wl_client_t *client, uint32_t id) {
    for (int i = 0; i < client->obj_count; i++) {
        if (client->objects[i].id == id)
            return &client->objects[i];
    }
    return NULL;
}

static wl_object_t *client_add_object(wl_client_t *client, uint32_t id,
                                       const wl_interface_t *iface,
                                       void *impl) {
    if (client->obj_count >= 64) return NULL;
    wl_object_t *obj = &client->objects[client->obj_count++];
    obj->id = id;
    obj->interface = iface;
    obj->implementation = impl;
    obj->client = client;
    return obj;
}

/* ─── Argument reader ─── */
typedef struct {
    const uint8_t *data;
    uint32_t size;
    uint32_t pos;
    int fds[8];
    int fd_count;
    int fd_pos;
} arg_reader_t;

static void arg_read_init(arg_reader_t *r, const void *data, uint32_t size) {
    r->data = (const uint8_t *)data;
    r->size = size;
    r->pos = 0;
    r->fd_count = 0;
    r->fd_pos = 0;
}

static void arg_read_int(arg_reader_t *r, int32_t *val) {
    if (r->pos + 4 > r->size) { *val = 0; return; }
    memcpy(val, r->data + r->pos, 4);
    r->pos += 4;
}

static void arg_read_uint(arg_reader_t *r, uint32_t *val) {
    int32_t v;
    arg_read_int(r, &v);
    *val = (uint32_t)v;
}

static void arg_read_string(arg_reader_t *r, char *buf, int max_len) {
    if (r->pos + 4 > r->size) { buf[0] = 0; return; }
    uint32_t slen;
    memcpy(&slen, r->data + r->pos, 4);
    /* Align string length to 4 bytes */
    uint32_t aligned = (slen + 3) & ~3;
    if (r->pos + 4 + aligned > r->size || slen >= (uint32_t)max_len) {
        buf[0] = 0;
        r->pos += 4 + aligned;
        return;
    }
    memcpy(buf, r->data + r->pos + 4, slen);
    buf[slen] = 0;
    r->pos += 4 + aligned;
}

static void arg_read_object(arg_reader_t *r, uint32_t *id) {
    arg_read_uint(r, id);
}

static void arg_read_new_id(arg_reader_t *r, uint32_t *id) {
    arg_read_uint(r, id);
}

static int arg_read_fd(arg_reader_t *r) {
    if (r->fd_pos >= r->fd_count) return -1;
    return r->fds[r->fd_pos++];
}

/* ─── Protocol request handlers ─── */

/* wl_display */
static void handle_display_get_registry(wl_client_t *client, arg_reader_t *r) {
    uint32_t id;
    arg_read_new_id(r, &id);
    client_add_object(client, id, &iface_wl_registry, NULL);
    
    /* Send globals (registry events) */
    struct __attribute__((packed)) {
        uint32_t id;
        uint32_t version;
        char name[32];
    } ev;
    
    /* wl_compositor */
    memset(&ev, 0, sizeof(ev));
    ev.id = WL_COMPOSITOR_ID;
    ev.version = 4;
    strcpy(ev.name, "wl_compositor");
    wl_send_event(client, id, WL_REGISTRY_GLOBAL, &ev, sizeof(ev));
    
    /* wl_subcompositor */
    ev.id = WL_SUBCOMPOSITOR_ID;
    ev.version = 1;
    strcpy(ev.name, "wl_subcompositor");
    wl_send_event(client, id, WL_REGISTRY_GLOBAL, &ev, sizeof(ev));
    
    /* wl_shm */
    ev.id = WL_SHM_ID;
    ev.version = 1;
    strcpy(ev.name, "wl_shm");
    wl_send_event(client, id, WL_REGISTRY_GLOBAL, &ev, sizeof(ev));
    
    /* wl_shell */
    ev.id = 12;
    ev.version = 1;
    strcpy(ev.name, "wl_shell");
    wl_send_event(client, id, WL_REGISTRY_GLOBAL, &ev, sizeof(ev));
    
    /* wl_seat */
    ev.id = WL_SEAT_ID;
    ev.version = 7;
    strcpy(ev.name, "wl_seat");
    wl_send_event(client, id, WL_REGISTRY_GLOBAL, &ev, sizeof(ev));
    
    /* wl_output */
    ev.id = WL_OUTPUT_ID_BASE;
    ev.version = 3;
    strcpy(ev.name, "wl_output");
    wl_send_event(client, id, WL_REGISTRY_GLOBAL, &ev, sizeof(ev));
    
    /* wl_data_device_manager */
    ev.id = 13;
    ev.version = 3;
    strcpy(ev.name, "wl_data_device_manager");
    wl_send_event(client, id, WL_REGISTRY_GLOBAL, &ev, sizeof(ev));
}

static void handle_display_sync(wl_client_t *client, arg_reader_t *r) {
    uint32_t callback_id;
    arg_read_new_id(r, &callback_id);
    client_add_object(client, callback_id, &iface_wl_callback, NULL);
    
    /* Send callback done immediately */
    struct __attribute__((packed)) {
        uint32_t data;
    } ev = {1};
    wl_send_event(client, callback_id, WL_CALLBACK_DONE, &ev, sizeof(ev));
}

/* wl_registry */
static void handle_registry_bind(wl_client_t *client, arg_reader_t *r) {
    uint32_t name, id;
    arg_read_uint(r, &name);
    r->pos += 4; /* skip interface name string */
    /* Actually we need to read the string properly */
    /* Rewind to read the string */
    r->pos -= 8; /* uint for name, then skip to string */
    char iface_name[64];
    arg_read_uint(r, &name);
    arg_read_string(r, iface_name, sizeof(iface_name));
    uint32_t version;
    arg_read_uint(r, &version);
    arg_read_new_id(r, &id);
    
    /* Create the bound object */
    if (name == WL_COMPOSITOR_ID && strcmp(iface_name, "wl_compositor") == 0) {
        client_add_object(client, id, &iface_wl_compositor, NULL);
    } else if (name == WL_SHM_ID && strcmp(iface_name, "wl_shm") == 0) {
        client_add_object(client, id, &iface_wl_shm, NULL);
    } else if (name == WL_SEAT_ID) {
        client_add_object(client, id, &iface_wl_seat, NULL);
    } else if (name == 12 && strcmp(iface_name, "wl_shell") == 0) {
        client_add_object(client, id, &iface_wl_shell, NULL);
    } else if (name == WL_OUTPUT_ID_BASE) {
        client_add_object(client, id, &iface_wl_output, NULL);
    } else if (name == WL_SUBCOMPOSITOR_ID) {
        client_add_object(client, id, &iface_wl_subcompositor, NULL);
    } else if (name == 13) {
        client_add_object(client, id, &iface_wl_data_device, NULL);
    }
}

/* wl_compositor */
static void handle_compositor_create_surface(wl_client_t *client, arg_reader_t *r) {
    uint32_t id;
    arg_read_new_id(r, &id);
    
    wl_surface_t *surf = calloc(1, sizeof(wl_surface_t));
    if (!surf) return;
    surf->id = id;
    surf->client = client;
    surf->comp = client->comp;
    surf->width = 0;
    surf->height = 0;
    surf->buffer = NULL;
    surf->x = 100;
    surf->y = 100;
    surf->mapped = 0;
    surf->destroyed = 0;
    
    /* Add to client */
    client_add_object(client, id, &iface_wl_surface, surf);
    
    /* Add to global surface list */
    surf->next = client->comp->surfaces;
    client->comp->surfaces = surf;
}

static void handle_compositor_create_region(wl_client_t *client, arg_reader_t *r) {
    uint32_t id;
    arg_read_new_id(r, &id);
    client_add_object(client, id, &iface_wl_region, NULL);
}

/* wl_surface */
static void handle_surface_destroy(wl_client_t *client, arg_reader_t *r) {
    uint32_t id;
    arg_read_object(r, &id);
    wl_object_t *obj = client_find_object(client, id);
    if (obj && obj->implementation) {
        wl_surface_t *surf = (wl_surface_t *)obj->implementation;
        surf->destroyed = 1;
        surf->mapped = 0;
        free(surf);
        obj->implementation = NULL;
    }
}

static void handle_surface_attach(wl_client_t *client, arg_reader_t *r) {
    wl_object_t *obj = client_find_object(client, r->data ? 0 : 0);
    /* Skip: reading surface object from the message would need proper parsing */
    uint32_t surf_id, buf_id;
    r->pos = 0;
    arg_read_object(r, &surf_id);
    arg_read_object(r, &buf_id);
    int32_t dx, dy;
    arg_read_int(r, &dx);
    arg_read_int(r, &dy);
    
    obj = client_find_object(client, surf_id);
    if (!obj || !obj->implementation) return;
    wl_surface_t *surf = (wl_surface_t *)obj->implementation;
    
    if (buf_id == 0) {
        surf->buffer = NULL;
        return;
    }
    
    wl_object_t *buf_obj = client_find_object(client, buf_id);
    if (buf_obj && buf_obj->implementation) {
        surf->buffer = (wl_buffer_t *)buf_obj->implementation;
        surf->buffer->busy = 1;
    }
}

static void handle_surface_damage(wl_client_t *client, arg_reader_t *r) {
    /* Skip damage - we redraw everything */
}

static void handle_surface_frame(wl_client_t *client, arg_reader_t *r) {
    uint32_t surf_id, callback_id;
    arg_read_object(r, &surf_id);
    arg_read_new_id(r, &callback_id);
    client_add_object(client, callback_id, &iface_wl_callback, NULL);
    
    /* Send callback done immediately */
    struct __attribute__((packed)) { uint32_t data; } ev = {0};
    wl_send_event(client, callback_id, WL_CALLBACK_DONE, &ev, sizeof(ev));
}

static void handle_surface_commit(wl_client_t *client, arg_reader_t *r) {
    uint32_t surf_id;
    arg_read_object(r, &surf_id);
    
    wl_object_t *obj = client_find_object(client, surf_id);
    if (!obj || !obj->implementation) return;
    wl_surface_t *surf = (wl_surface_t *)obj->implementation;
    
    if (surf->buffer) {
        surf->mapped = 1;
        surf->buffer->busy = 0;
    }
}

static void handle_surface_set_buffer_transform(wl_client_t *client, arg_reader_t *r) {}
static void handle_surface_set_buffer_scale(wl_client_t *client, arg_reader_t *r) {}

/* wl_shell */
static void handle_shell_get_shell_surface(wl_client_t *client, arg_reader_t *r) {
    uint32_t shell_id, surf_id, shell_surf_id;
    arg_read_object(r, &shell_id);
    arg_read_object(r, &surf_id);
    arg_read_new_id(r, &shell_surf_id);
    client_add_object(client, shell_surf_id, &iface_wl_shell_surface, NULL);
}

/* wl_shell_surface */
static void handle_shell_surface_pong(wl_client_t *client, arg_reader_t *r) {}
static void handle_shell_surface_move(wl_client_t *client, arg_reader_t *r) {}
static void handle_shell_surface_resize(wl_client_t *client, arg_reader_t *r) {}
static void handle_shell_surface_set_toplevel(wl_client_t *client, arg_reader_t *r) {}
static void handle_shell_surface_set_transient(wl_client_t *client, arg_reader_t *r) {}
static void handle_shell_surface_set_fullscreen(wl_client_t *client, arg_reader_t *r) {}
static void handle_shell_surface_set_popup(wl_client_t *client, arg_reader_t *r) {}
static void handle_shell_surface_set_maximized(wl_client_t *client, arg_reader_t *r) {}
static void handle_shell_surface_set_title(wl_client_t *client, arg_reader_t *r) {
    uint32_t obj_id, surf_id;
    arg_read_object(r, &obj_id);
    arg_read_object(r, &surf_id);
    char title[64] = {0};
    arg_read_string(r, title, sizeof(title));
    
    /* Find the surface associated with this shell surface and set title */
    wl_object_t *obj = client_find_object(client, obj_id);
    if (!obj) return;
    /* Store title in the surface's metadata */
    /* For now, just log it */
    fprintf(stderr, "Shell surface title: %s\n", title);
}
static void handle_shell_surface_set_class(wl_client_t *client, arg_reader_t *r) {}

/* wl_shm */
static void handle_shm_create_pool(wl_client_t *client, arg_reader_t *r) {
    uint32_t id;
    arg_read_new_id(r, &id);
    int fd = arg_read_fd(r);
    int32_t size;
    arg_read_int(r, &size);
    
    if (fd < 0 || size <= 0) return;
    
    wl_shm_pool_t *pool = calloc(1, sizeof(wl_shm_pool_t));
    if (!pool) { close(fd); return; }
    
    pool->fd = fd;
    pool->size = size;
    pool->data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    pool->refcount = 1;
    
    if (pool->data == MAP_FAILED) {
        free(pool);
        close(fd);
        return;
    }
    
    client_add_object(client, id, &iface_wl_shm_pool, pool);
}

/* wl_shm_pool */
static void handle_shm_pool_create_buffer(wl_client_t *client, arg_reader_t *r) {
    uint32_t pool_id, buf_id;
    arg_read_object(r, &pool_id);
    arg_read_new_id(r, &buf_id);
    int32_t offset, width, height, stride;
    uint32_t format;
    arg_read_int(r, &offset);
    arg_read_int(r, &width);
    arg_read_int(r, &height);
    arg_read_int(r, &stride);
    arg_read_uint(r, &format);
    
    wl_object_t *obj = client_find_object(client, pool_id);
    if (!obj || !obj->implementation) return;
    wl_shm_pool_t *pool = (wl_shm_pool_t *)obj->implementation;
    
    wl_buffer_t *buf = calloc(1, sizeof(wl_buffer_t));
    if (!buf) return;
    buf->pool = pool;
    buf->offset = offset;
    buf->width = width;
    buf->height = height;
    buf->stride = stride;
    buf->format = format;
    buf->refcount = 1;
    pool->refcount++;
    
    client_add_object(client, buf_id, &iface_wl_buffer, buf);
}

static void handle_shm_pool_destroy(wl_client_t *client, arg_reader_t *r) {
    uint32_t id;
    arg_read_object(r, &id);
    wl_object_t *obj = client_find_object(client, id);
    if (obj && obj->implementation) {
        wl_shm_pool_t *pool = (wl_shm_pool_t *)obj->implementation;
        pool->refcount--;
        if (pool->refcount <= 0) {
            munmap(pool->data, pool->size);
            close(pool->fd);
            free(pool);
        }
        obj->implementation = NULL;
    }
}

static void handle_shm_pool_resize(wl_client_t *client, arg_reader_t *r) {
    uint32_t id;
    int32_t size;
    arg_read_object(r, &id);
    arg_read_int(r, &size);
    wl_object_t *obj = client_find_object(client, id);
    if (obj && obj->implementation) {
        wl_shm_pool_t *pool = (wl_shm_pool_t *)obj->implementation;
        if (size > (int32_t)pool->size) {
            mremap(pool->data, pool->size, size, MREMAP_MAYMOVE);
            pool->size = size;
        }
    }
}

/* wl_buffer */
static void handle_buffer_destroy(wl_client_t *client, arg_reader_t *r) {
    uint32_t id;
    arg_read_object(r, &id);
    wl_object_t *obj = client_find_object(client, id);
    if (obj && obj->implementation) {
        wl_buffer_t *buf = (wl_buffer_t *)obj->implementation;
        if (buf->pool) {
            buf->pool->refcount--;
            if (buf->pool->refcount <= 0) {
                munmap(buf->pool->data, buf->pool->size);
                close(buf->pool->fd);
                free(buf->pool);
            }
        }
        free(buf);
        obj->implementation = NULL;
    }
}

/* wl_seat */
static void handle_seat_get_pointer(wl_client_t *client, arg_reader_t *r) {
    uint32_t seat_id, ptr_id;
    arg_read_object(r, &seat_id);
    arg_read_new_id(r, &ptr_id);
    client_add_object(client, ptr_id, &iface_wl_pointer, NULL);
}

static void handle_seat_get_keyboard(wl_client_t *client, arg_reader_t *r) {
    uint32_t seat_id, kbd_id;
    arg_read_object(r, &seat_id);
    arg_read_new_id(r, &kbd_id);
    client_add_object(client, kbd_id, &iface_wl_keyboard, NULL);
}

static void handle_seat_get_touch(wl_client_t *client, arg_reader_t *r) {
    uint32_t seat_id, touch_id;
    arg_read_object(r, &seat_id);
    arg_read_new_id(r, &touch_id);
    client_add_object(client, touch_id, &iface_wl_touch, NULL);
}

/* ─── Message dispatch table ─── */
typedef void (*handler_t)(wl_client_t *, arg_reader_t *);

static const handler_t display_handlers[] = {
    [WL_DISPLAY_SYNC] = handle_display_sync,
    [WL_DISPLAY_GET_REGISTRY] = handle_display_get_registry,
};

static const handler_t registry_handlers[] = {
    [WL_REGISTRY_BIND] = handle_registry_bind,
};

static const handler_t compositor_handlers[] = {
    [WL_COMPOSITOR_CREATE_SURFACE] = handle_compositor_create_surface,
    [WL_COMPOSITOR_CREATE_REGION] = handle_compositor_create_region,
};

static const handler_t surface_handlers[] = {
    [WL_SURFACE_DESTROY] = handle_surface_destroy,
    [WL_SURFACE_ATTACH] = handle_surface_attach,
    [WL_SURFACE_DAMAGE] = handle_surface_damage,
    [WL_SURFACE_FRAME] = handle_surface_frame,
    [WL_SURFACE_SET_OPAQUE_REGION] = NULL,
    [WL_SURFACE_SET_INPUT_REGION] = NULL,
    [WL_SURFACE_COMMIT] = handle_surface_commit,
    [WL_SURFACE_SET_BUFFER_TRANSFORM] = handle_surface_set_buffer_transform,
    [WL_SURFACE_SET_BUFFER_SCALE] = handle_surface_set_buffer_scale,
};

static const handler_t shell_handlers[] = {
    [WL_SHELL_GET_SHELL_SURFACE] = handle_shell_get_shell_surface,
};

static const handler_t shell_surface_handlers[] = {
    [WL_SHELL_SURFACE_PONG] = handle_shell_surface_pong,
    [WL_SHELL_SURFACE_MOVE] = handle_shell_surface_move,
    [WL_SHELL_SURFACE_RESIZE] = handle_shell_surface_resize,
    [WL_SHELL_SURFACE_SET_TOPLEVEL] = handle_shell_surface_set_toplevel,
    [WL_SHELL_SURFACE_SET_TRANSIENT] = handle_shell_surface_set_transient,
    [WL_SHELL_SURFACE_SET_FULLSCREEN] = handle_shell_surface_set_fullscreen,
    [WL_SHELL_SURFACE_SET_POPUP] = handle_shell_surface_set_popup,
    [WL_SHELL_SURFACE_SET_MAXIMIZED] = handle_shell_surface_set_maximized,
    [WL_SHELL_SURFACE_SET_TITLE] = handle_shell_surface_set_title,
    [WL_SHELL_SURFACE_SET_CLASS] = handle_shell_surface_set_class,
};

static const handler_t shm_handlers[] = {
    [WL_SHM_CREATE_POOL] = handle_shm_create_pool,
};

static const handler_t shm_pool_handlers[] = {
    [WL_SHM_POOL_CREATE_BUFFER] = handle_shm_pool_create_buffer,
    [WL_SHM_POOL_DESTROY] = handle_shm_pool_destroy,
    [WL_SHM_POOL_RESIZE] = handle_shm_pool_resize,
};

static const handler_t buffer_handlers[] = {
    [WL_BUFFER_DESTROY] = handle_buffer_destroy,
};

static const handler_t seat_handlers[] = {
    [WL_SEAT_GET_POINTER] = handle_seat_get_pointer,
    [WL_SEAT_GET_KEYBOARD] = handle_seat_get_keyboard,
    [WL_SEAT_GET_TOUCH] = handle_seat_get_touch,
};

static const handler_t region_handlers[] = {
    [WL_REGION_DESTROY] = NULL,
    [WL_REGION_ADD] = NULL,
    [WL_REGION_SUBTRACT] = NULL,
};

/* ─── Dispatch a Wayland message ─── */
static void dispatch_message(wl_client_t *client, wl_msg_header_t *hdr,
                              const uint8_t *payload, int fd_count, int *fds) {
    arg_reader_t r;
    arg_read_init(&r, payload, hdr->size - 8);
    r.fd_count = fd_count;
    memcpy(r.fds, fds, fd_count * sizeof(int));
    
    wl_object_t *obj = client_find_object(client, hdr->object_id);
    if (!obj) {
        wl_post_error(client, hdr->object_id, 0, "Unknown object");
        return;
    }
    
    const handler_t *handlers = NULL;
    int handler_count = 0;
    
    if (obj->interface == &iface_wl_display) {
        handlers = display_handlers;
        handler_count = sizeof(display_handlers) / sizeof(handler_t);
    } else if (obj->interface == &iface_wl_registry) {
        handlers = registry_handlers;
        handler_count = sizeof(registry_handlers) / sizeof(handler_t);
    } else if (obj->interface == &iface_wl_compositor) {
        handlers = compositor_handlers;
        handler_count = sizeof(compositor_handlers) / sizeof(handler_t);
    } else if (obj->interface == &iface_wl_surface) {
        handlers = surface_handlers;
        handler_count = sizeof(surface_handlers) / sizeof(handler_t);
    } else if (obj->interface == &iface_wl_shell) {
        handlers = shell_handlers;
        handler_count = sizeof(shell_handlers) / sizeof(handler_t);
    } else if (obj->interface == &iface_wl_shell_surface) {
        handlers = shell_surface_handlers;
        handler_count = sizeof(shell_surface_handlers) / sizeof(handler_t);
    } else if (obj->interface == &iface_wl_shm) {
        handlers = shm_handlers;
        handler_count = sizeof(shm_handlers) / sizeof(handler_t);
    } else if (obj->interface == &iface_wl_shm_pool) {
        handlers = shm_pool_handlers;
        handler_count = sizeof(shm_pool_handlers) / sizeof(handler_t);
    } else if (obj->interface == &iface_wl_buffer) {
        handlers = buffer_handlers;
        handler_count = sizeof(buffer_handlers) / sizeof(handler_t);
    } else if (obj->interface == &iface_wl_seat) {
        handlers = seat_handlers;
        handler_count = sizeof(seat_handlers) / sizeof(handler_t);
    } else if (obj->interface == &iface_wl_region) {
        handlers = region_handlers;
        handler_count = sizeof(region_handlers) / sizeof(handler_t);
    }
    
    if (handlers && hdr->opcode < handler_count && handlers[hdr->opcode]) {
        handlers[hdr->opcode](client, &r);
    }
}

/* ─── Accept new client ─── */
static int accept_client(compositor_t *c) {
    struct sockaddr_un addr;
    socklen_t len = sizeof(addr);
    int fd = accept(c->wl_listen_fd, (struct sockaddr *)&addr, &len);
    if (fd < 0) return -1;
    
    wl_client_t *client = calloc(1, sizeof(wl_client_t));
    if (!client) { close(fd); return -1; }
    
    client->fd = fd;
    client->next_id = CLIENT_OBJECT_BASE;
    client->comp = c;
    
    /* Create the wl_display object for this client (id=1) */
    client_add_object(client, WL_DISPLAY_ID, &iface_wl_display, NULL);
    
    /* Add to client list */
    client->next = c->clients;
    c->clients = client;
    
    fprintf(stderr, "Wayland: client connected (fd=%d)\n", fd);
    return 0;
}

/* ─── Read client messages ─── */
static void read_client(wl_client_t *client) {
    uint8_t buf[4096];
    struct iovec iov = {.iov_base = buf, .iov_len = sizeof(buf)};
    uint8_t cmsg_buf[CMSG_SPACE(sizeof(int) * 8)];
    struct msghdr msg = {
        .msg_iov = &iov, .msg_iovlen = 1,
        .msg_control = cmsg_buf, .msg_controllen = sizeof(cmsg_buf),
    };
    
    ssize_t ret = recvmsg(client->fd, &msg, 0);
    if (ret <= 0) {
        /* Client disconnected */
        fprintf(stderr, "Wayland: client disconnected\n");
        shutdown(client->fd, SHUT_RDWR);
        close(client->fd);
        client->fd = -1;
        return;
    }
    
    /* Extract FDs */
    int fds[8], fd_count = 0;
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    while (cmsg) {
        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
            int *fd_data = (int *)CMSG_DATA(cmsg);
            int count = (cmsg->cmsg_len - CMSG_LEN(0)) / sizeof(int);
            for (int i = 0; i < count && fd_count < 8; i++)
                fds[fd_count++] = fd_data[i];
        }
        cmsg = CMSG_NXTHDR(&msg, cmsg);
    }
    
    /* Process messages */
    uint32_t pos = 0;
    while (pos + 8 <= (uint32_t)ret) {
        wl_msg_header_t *hdr = (wl_msg_header_t *)(buf + pos);
        if (hdr->size < 8) break;
        if (pos + hdr->size > (uint32_t)ret) break;
        
        dispatch_message(client, hdr, buf + pos + 8, fd_count, fds);
        
        pos += hdr->size;
        /* Align to 4 bytes */
        while (pos & 3) pos++;
    }
    
    /* Close unused FDs */
    for (int i = 0; i < fd_count; i++) close(fds[i]);
}

/* ─── Initialize Wayland server ─── */
int wl_server_init(compositor_t *c) {
    const char *display = getenv("WAYLAND_DISPLAY");
    if (!display) display = "wayland-0";
    
    /* Build socket path */
    char path[108];
    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (runtime_dir) {
        snprintf(path, sizeof(path), "%s/%s", runtime_dir, display);
    } else {
        snprintf(path, sizeof(path), "/tmp/%s", display);
    }
    
    /* Remove old socket */
    unlink(path);
    
    /* Create socket */
    c->wl_listen_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (c->wl_listen_fd < 0) {
        fprintf(stderr, "Wayland: cannot create socket\n");
        return -1;
    }
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    
    if (bind(c->wl_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Wayland: cannot bind to %s\n", path);
        close(c->wl_listen_fd);
        c->wl_listen_fd = -1;
        return -1;
    }
    
    /* Make sure socket is writable by all */
    chmod(path, 0666);
    
    if (listen(c->wl_listen_fd, 4) < 0) {
        close(c->wl_listen_fd);
        c->wl_listen_fd = -1;
        return -1;
    }
    
    /* Set WAYLAND_DISPLAY for clients */
    setenv("WAYLAND_DISPLAY", display, 1);
    if (runtime_dir)
        setenv("XDG_RUNTIME_DIR", runtime_dir, 1);
    
    fprintf(stderr, "Wayland: listening on %s\n", path);
    return 0;
}

/* ─── Poll Wayland clients ─── */
void wl_server_poll(compositor_t *c) {
    /* Accept new clients */
    struct pollfd listen_pfd = {.fd = c->wl_listen_fd, .events = POLLIN};
    if (poll(&listen_pfd, 1, 0) > 0) {
        accept_client(c);
    }
    
    /* Read from existing clients */
    wl_client_t *prev = NULL;
    wl_client_t *client = c->clients;
    while (client) {
        if (client->fd < 0) {
            /* Remove disconnected client */
            wl_client_t *to_free = client;
            if (prev) prev->next = client->next;
            else c->clients = client->next;
            client = client->next;
            free(to_free);
            continue;
        }
        
        struct pollfd pfd = {.fd = client->fd, .events = POLLIN};
        if (poll(&pfd, 1, 0) > 0) {
            read_client(client);
        }
        
        prev = client;
        client = client->next;
    }
}

void wl_server_shutdown(compositor_t *c) {
    /* Close all clients */
    wl_client_t *client = c->clients;
    while (client) {
        wl_client_t *next = client->next;
        if (client->fd >= 0) {
            shutdown(client->fd, SHUT_RDWR);
            close(client->fd);
        }
        free(client);
        client = next;
    }
    c->clients = NULL;
    
    /* Close listen socket */
    if (c->wl_listen_fd >= 0) {
        close(c->wl_listen_fd);
        c->wl_listen_fd = -1;
    }
}

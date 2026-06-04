/* Synth3x Compositor — DRM/KMS Display Backend
 * Uses Linux Kernel Mode Setting for proper display.
 * Supports: modesetting, pageflip, multi-buffer, dumb buffers.
 */

#include "compositor.h"
#include <xf86drm.h>
#include <xf86drmMode.h>

/* Find a connected connector */
static int find_connector(int fd, drmModeRes *res, drmModeConnector **conn_out) {
    for (int i = 0; i < res->count_connectors; i++) {
        drmModeConnector *conn = drmModeGetConnector(fd, res->connectors[i]);
        if (!conn) continue;
        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
            *conn_out = conn;
            return 0;
        }
        drmModeFreeConnector(conn);
    }
    /* Fallback: use first connector even if not connected */
    if (res->count_connectors > 0) {
        drmModeConnector *conn = drmModeGetConnector(fd, res->connectors[0]);
        if (conn && conn->count_modes > 0) {
            *conn_out = conn;
            return 0;
        }
    }
    return -1;
}

/* Find a suitable CRTC for the connector */
static int find_crtc(int fd, drmModeRes *res, drmModeConnector *conn) {
    for (int i = 0; i < conn->count_encoders; i++) {
        drmModeEncoder *enc = drmModeGetEncoder(fd, conn->encoders[i]);
        if (!enc) continue;
        for (int j = 0; j < res->count_crtcs; j++) {
            if (enc->possible_crtcs & (1 << j)) {
                drmModeFreeEncoder(enc);
                return res->crtcs[j];
            }
        }
        drmModeFreeEncoder(enc);
    }
    /* Fallback: use first CRTC */
    if (res->count_crtcs > 0)
        return res->crtcs[0];
    return -1;
}

/* Create a dumb buffer and map it */
static int create_dumb_buffer(int fd, int width, int height, int bpp,
                              uint32_t *fb_id, void **map, size_t *size, int *stride) {
    struct drm_mode_create_dumb create = {0};
    create.width = width;
    create.height = height;
    create.bpp = bpp;
    
    if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) < 0)
        return -1;
    
    *stride = create.pitch;
    *size = create.size;
    
    struct drm_mode_map_dumb map_req = {0};
    map_req.handle = create.handle;
    if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req) < 0) {
        struct drm_mode_destroy_dumb destroy = {.handle = create.handle};
        drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        return -1;
    }
    
    *map = mmap(0, create.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                fd, map_req.offset);
    if (*map == MAP_FAILED) {
        struct drm_mode_destroy_dumb destroy = {.handle = create.handle};
        drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        return -1;
    }
    
    if (drmModeAddFB(fd, width, height, bpp, bpp == 32 ? 32 : bpp,
                     *stride, create.handle, fb_id) < 0) {
        munmap(*map, create.size);
        struct drm_mode_destroy_dumb destroy = {.handle = create.handle};
        drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
        return -1;
    }
    
    return 0;
}

int drm_init(compositor_t *c) {
    drm_state_t *d = &c->drm;
    
    /* Try to open DRM device */
    const char *devices[] = {
        "/dev/dri/card0", "/dev/dri/card1",
        "/dev/dri/renderD128", NULL
    };
    
    d->fd = -1;
    for (int i = 0; devices[i]; i++) {
        d->fd = open(devices[i], O_RDWR);
        if (d->fd >= 0) break;
    }
    
    if (d->fd < 0) {
        fprintf(stderr, "DRM: No DRM device found\n");
        return -1;
    }
    
    /* Get DRM resources */
    drmModeRes *res = drmModeGetResources(d->fd);
    if (!res) {
        fprintf(stderr, "DRM: Cannot get resources\n");
        close(d->fd); d->fd = -1;
        return -1;
    }
    
    /* Find connector */
    drmModeConnector *conn = NULL;
    if (find_connector(d->fd, res, &conn) < 0) {
        fprintf(stderr, "DRM: No connected connector\n");
        drmModeFreeResources(res);
        close(d->fd); d->fd = -1;
        return -1;
    }
    
    /* Find CRTC */
    d->crtc_id = find_crtc(d->fd, res, conn);
    if (d->crtc_id < 0) {
        fprintf(stderr, "DRM: No suitable CRTC\n");
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(d->fd); d->fd = -1;
        return -1;
    }
    d->connector_id = conn->connector_id;
    
    /* Use preferred mode or first mode */
    drmModeModeInfo *mode = &conn->modes[0];
    for (int i = 0; i < conn->count_modes; i++) {
        if (conn->modes[i].type & DRM_MODE_TYPE_PREFERRED) {
            mode = &conn->modes[i];
            break;
        }
    }
    
    d->width = mode->hdisplay;
    d->height = mode->vdisplay;
    
    /* Store output info */
    c->outputs[0].width = d->width;
    c->outputs[0].height = d->height;
    c->outputs[0].refresh = mode->vrefresh;
    c->outputs[0].enabled = 1;
    c->outputs[0].wl_id = WL_OUTPUT_ID_BASE;
    snprintf(c->outputs[0].make, sizeof(c->outputs[0].make), "DRM");
    snprintf(c->outputs[0].model, sizeof(c->outputs[0].model), "%dx%d",
             d->width, d->height);
    c->output_count = 1;
    
    c->fb_w = d->width;
    c->fb_h = d->height;
    
    /* Create two dumb buffers for double buffering */
    for (int i = 0; i < 2; i++) {
        if (create_dumb_buffer(d->fd, d->width, d->height, 32,
                               &d->bufs[i].fb_id, &d->bufs[i].map,
                               &d->bufs[i].size, &d->stride) < 0) {
            fprintf(stderr, "DRM: Failed to create dumb buffer %d\n", i);
            drmModeFreeConnector(conn);
            drmModeFreeResources(res);
            close(d->fd); d->fd = -1;
            return -1;
        }
    }
    
    /* Set up backbuffer */
    c->backbuf_size = d->bufs[0].size;
    c->backbuf = malloc(c->backbuf_size);
    if (!c->backbuf) {
        drm_shutdown(c);
        return -1;
    }
    
    /* Initial mode set with buffer 0 */
    d->fb_id = d->bufs[0].fb_id;
    d->map = d->bufs[0].map;
    d->front_buf = 0;
    
    uint32_t conn_id = (uint32_t)d->connector_id;
    int ret = drmModeSetCrtc(d->fd, d->crtc_id, d->fb_id, 0, 0,
                              &conn_id, 1, mode);
    if (ret < 0) {
        fprintf(stderr, "DRM: Mode set failed: %d\n", ret);
        drm_shutdown(c);
        return -1;
    }
    
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    
    fprintf(stderr, "DRM: %dx%d @ %dHz\n", d->width, d->height, mode->vrefresh);
    return 0;
}

void drm_swap(compositor_t *c) {
    drm_state_t *d = &c->drm;
    if (d->fd < 0) return;
    
    if (d->bufs[0].map == NULL) {
        /* fbdev fallback mode: copy backbuffer directly to framebuffer map */
        memcpy(d->map, c->backbuf, c->backbuf_size);
        return;
    }
    
    /* Copy backbuffer to current front buffer */
    memcpy(d->map, c->backbuf, c->backbuf_size > d->bufs[d->front_buf].size ?
           d->bufs[d->front_buf].size : c->backbuf_size);
    
    /* Page flip to next buffer */
    int next = d->front_buf ^ 1;
    
    /* Copy backbuffer to next buffer too for consistency */
    memcpy(d->bufs[next].map, c->backbuf, c->backbuf_size > d->bufs[next].size ?
           d->bufs[next].size : c->backbuf_size);
    
    int ret = drmModePageFlip(d->fd, d->crtc_id, d->bufs[next].fb_id,
                               DRM_MODE_PAGE_FLIP_EVENT, NULL);
    if (ret == 0) {
        d->fb_id = d->bufs[next].fb_id;
        d->map = d->bufs[next].map;
        d->front_buf = next;
        
        /* Wait for page flip event (non-blocking would need event loop) */
        drmEventContext evctx = {.version = DRM_EVENT_CONTEXT_VERSION};
        struct pollfd pfd = {.fd = d->fd, .events = POLLIN};
        if (poll(&pfd, 1, 16) > 0) {
            drmHandleEvent(d->fd, &evctx);
        }
    }
}

void drm_shutdown(compositor_t *c) {
    drm_state_t *d = &c->drm;
    if (d->fd < 0) return;
    
    for (int i = 0; i < 2; i++) {
        if (d->bufs[i].map) {
            munmap(d->bufs[i].map, d->bufs[i].size);
            if (d->bufs[i].fb_id) {
                drmModeRmFB(d->fd, d->bufs[i].fb_id);
            }
        }
    }
    
    if (c->backbuf) free(c->backbuf);
    close(d->fd);
    d->fd = -1;
}

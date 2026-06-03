#ifndef SYNTH3X_COMPOSITOR_H
#define SYNTH3X_COMPOSITOR_H

#define _GNU_SOURCE
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#ifndef SYNTH3X_VERSION
#define SYNTH3X_VERSION "0.9"
#endif

/* ─── CONFIG ─── */
#define MAX_CLIENTS     16
#define MAX_SURFACES    64
#define MAX_OUTPUTS     4
#define MAX_SEATS       4
#define MAX_WIN         32
#define MAX_NOTIF       8
#define MAX_TERM_LOGS   16
#define WORKSPACES      4
#define PANEL_H         30
#define NOTIF_W         280
#define NOTIF_H         72
#define NOTIF_DUR       5

/* ─── Wayland object IDs ─── */
#define WL_DISPLAY_ID       1
#define WL_REGISTRY_ID      2
#define WL_COMPOSITOR_ID    3
#define WL_SUBCOMPOSITOR_ID 4
#define WL_SHM_ID           5
#define WL_SEAT_ID          6
#define WL_OUTPUT_ID_BASE   7
#define WL_DATA_DEVICE_ID   11
#define CLIENT_OBJECT_BASE  0x1000

/* ─── Wayland opcodes ─── */
enum wl_display_request {
    WL_DISPLAY_SYNC          = 0,
    WL_DISPLAY_GET_REGISTRY  = 1,
};
enum wl_display_event {
    WL_DISPLAY_ERROR         = 0,
    WL_DISPLAY_DELETE_ID     = 1,
};

enum wl_registry_request {
    WL_REGISTRY_BIND         = 0,
};
enum wl_registry_event {
    WL_REGISTRY_GLOBAL       = 0,
    WL_REGISTRY_GLOBAL_REMOVE = 1,
};

enum wl_compositor_request {
    WL_COMPOSITOR_CREATE_SURFACE       = 0,
    WL_COMPOSITOR_CREATE_REGION        = 1,
};

enum wl_surface_request {
    WL_SURFACE_DESTROY       = 0,
    WL_SURFACE_ATTACH        = 1,
    WL_SURFACE_DAMAGE        = 2,
    WL_SURFACE_FRAME         = 3,
    WL_SURFACE_SET_OPAQUE_REGION = 4,
    WL_SURFACE_SET_INPUT_REGION   = 5,
    WL_SURFACE_COMMIT        = 6,
    WL_SURFACE_SET_BUFFER_TRANSFORM = 7,
    WL_SURFACE_SET_BUFFER_SCALE     = 8,
};

enum wl_callback_event {
    WL_CALLBACK_DONE         = 0,
};

enum wl_shm_request {
    WL_SHM_CREATE_POOL       = 0,
};
enum wl_shm_event {
    WL_SHM_FORMATS           = 0,
};

enum wl_shm_pool_request {
    WL_SHM_POOL_CREATE_BUFFER = 0,
    WL_SHM_POOL_DESTROY      = 1,
    WL_SHM_POOL_RESIZE       = 2,
};

enum wl_buffer_request {
    WL_BUFFER_DESTROY        = 0,
};

enum wl_shell_request {
    WL_SHELL_GET_SHELL_SURFACE = 0,
};

enum wl_shell_surface_request {
    WL_SHELL_SURFACE_PONG          = 0,
    WL_SHELL_SURFACE_MOVE          = 1,
    WL_SHELL_SURFACE_RESIZE        = 2,
    WL_SHELL_SURFACE_SET_TOPLEVEL  = 3,
    WL_SHELL_SURFACE_SET_TRANSIENT = 4,
    WL_SHELL_SURFACE_SET_FULLSCREEN = 5,
    WL_SHELL_SURFACE_SET_POPUP     = 6,
    WL_SHELL_SURFACE_SET_MAXIMIZED = 7,
    WL_SHELL_SURFACE_SET_TITLE     = 8,
    WL_SHELL_SURFACE_SET_CLASS     = 9,
};
enum wl_shell_surface_event {
    WL_SHELL_SURFACE_PING           = 0,
    WL_SHELL_SURFACE_CONFIGURE      = 1,
    WL_SHELL_SURFACE_POPUP_DONE     = 2,
};

enum wl_seat_request {
    WL_SEAT_GET_POINTER      = 0,
    WL_SEAT_GET_KEYBOARD     = 1,
    WL_SEAT_GET_TOUCH        = 2,
};
enum wl_seat_event {
    WL_SEAT_CAPABILITIES     = 0,
    WL_SEAT_NAME             = 1,
};

enum wl_pointer_event {
    WL_POINTER_ENTER         = 0,
    WL_POINTER_LEAVE         = 1,
    WL_POINTER_MOTION        = 2,
    WL_POINTER_BUTTON        = 3,
    WL_POINTER_AXIS          = 4,
    WL_POINTER_FRAME         = 5,
    WL_POINTER_AXIS_SOURCE   = 6,
    WL_POINTER_AXIS_STOP     = 7,
    WL_POINTER_AXIS_DISCRETE = 8,
};

enum wl_keyboard_event {
    WL_KEYBOARD_KEYMAP       = 0,
    WL_KEYBOARD_ENTER        = 1,
    WL_KEYBOARD_LEAVE        = 2,
    WL_KEYBOARD_KEY          = 3,
    WL_KEYBOARD_MODIFIERS    = 4,
    WL_KEYBOARD_REPEAT_INFO  = 5,
};

enum wl_touch_event {
    WL_TOUCH_DOWN            = 0,
    WL_TOUCH_UP              = 1,
    WL_TOUCH_MOTION          = 2,
    WL_TOUCH_FRAME           = 3,
    WL_TOUCH_CANCEL          = 4,
};

enum wl_output_event {
    WL_OUTPUT_GEOMETRY       = 0,
    WL_OUTPUT_MODE           = 1,
    WL_OUTPUT_DONE           = 2,
    WL_OUTPUT_SCALE          = 3,
};

enum wl_region_request {
    WL_REGION_DESTROY        = 0,
    WL_REGION_ADD            = 1,
    WL_REGION_SUBTRACT       = 2,
};

enum wl_data_device_event {
    WL_DATA_DEVICE_DATA_OFFER   = 0,
    WL_DATA_DEVICE_ENTER        = 1,
    WL_DATA_DEVICE_LEAVE        = 2,
    WL_DATA_DEVICE_MOTION       = 3,
    WL_DATA_DEVICE_DROP         = 4,
    WL_DATA_DEVICE_SELECTION    = 5,
};

/* ─── Colors ─── */
#define COLOR_BG        0xFF0A0514
#define COLOR_PANEL_BG  0xFF140A20
#define COLOR_PANEL_FG  0xFFC8B4E6
#define COLOR_WIN_BG    0xFF0F0A18
#define COLOR_WIN_TITLE 0xFF180E26
#define COLOR_WIN_BORDER 0xFF461E6E
#define COLOR_ACCENT    0xFFFF0080
#define COLOR_TEXT      0xFF00FFE6
#define COLOR_DIM       0xFF5A3C6E
#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_GREEN     0xFF50DC64
#define COLOR_YELLOW    0xFFFADC32
#define COLOR_RED       0xFFFA5064
#define COLOR_ORANGE    0xFFFF8800

#define RGB565(r,g,b) ((((r)>>3)<<11) | (((g)>>2)<<5) | ((b)>>3))

/* ─── Forward declarations ─── */
struct wl_client;
struct wl_surface;
struct compositor;

/* ─── Wayland message header ─── */
typedef struct __attribute__((packed)) {
    uint32_t object_id;
    uint16_t size;
    uint16_t opcode;
} wl_msg_header_t;

/* ─── Wayland object ─── */
typedef struct wl_object {
    uint32_t id;
    const struct wl_interface *interface;
    void *implementation;
    struct wl_client *client;
} wl_object_t;

/* ─── Wayland interface descriptor ─── */
typedef struct wl_interface {
    const char *name;
    int version;
    int method_count;
    int event_count;
} wl_interface_t;

/* ─── Client connection ─── */
typedef struct wl_client {
    int fd;
    uint32_t next_id;
    wl_object_t objects[64];
    int obj_count;
    struct wl_client *next;
    struct compositor *comp;
} wl_client_t;

/* ─── Shared memory pool ─── */
typedef struct {
    int refcount;
    int fd;
    size_t size;
    void *data;
} wl_shm_pool_t;

/* ─── Buffer ─── */
typedef struct {
    int refcount;
    wl_shm_pool_t *pool;
    int offset;
    int width, height, stride;
    uint32_t format;
    int busy;
} wl_buffer_t;

/* ─── Surface ─── */
typedef struct wl_surface {
    uint32_t id;
    wl_client_t *client;
    int width, height;
    wl_buffer_t *buffer;
    int x, y;
    int mapped;
    struct wl_surface *next;
    struct compositor *comp;
    int destroyed;
} wl_surface_t;

/* ─── Output (display) ─── */
typedef struct {
    int x, y;
    int width, height;
    int refresh;
    int enabled;
    uint32_t wl_id;
    char make[32];
    char model[32];
} wl_output_t;

/* ─── Seat (input device) ─── */
typedef struct {
    uint32_t wl_id;
    int capabilities;
    wl_client_t *focus_client;
    wl_surface_t *focus_surface;
    int mx, my;
    int buttons;
    uint32_t serial;
} wl_seat_t;

/* ─── Window (AmnesiaDE shell window) ─── */
typedef struct {
    int x, y, w, h;
    char title[48];
    int hidden, ws, drag, dx, dy;
    int maximized;
    int orig_x, orig_y, orig_w, orig_h;
    wl_surface_t *surface;
} ShellWin;

/* ─── Notification ─── */
typedef struct {
    char title[48];
    char body[128];
    time_t t;
} Notif;

/* ─── DRM backend state ─── */
typedef struct {
    int fd;
    int crtc_id;
    int connector_id;
    int width, height;
    int stride;
    size_t size;
    uint32_t fb_id;
    void *map;
    int front_buf;
    struct {
        uint32_t fb_id;
        void *map;
        size_t size;
    } bufs[2];
} drm_state_t;

/* ─── Input device state ─── */
typedef struct {
    int fd;
    int type;
    char name[64];
} input_dev_t;

/* ─── Compositor main structure ─── */
typedef struct compositor {
    /* DRM display */
    drm_state_t drm;
    
    /* Input devices */
    input_dev_t input_devs[16];
    int input_count;
    
    /* Wayland state */
    int wl_listen_fd;
    wl_client_t *clients;
    wl_surface_t *surfaces;
    wl_output_t outputs[MAX_OUTPUTS];
    int output_count;
    wl_seat_t seat;
    
    /* Shell/DE state */
    ShellWin wins[MAX_WIN];
    int wc;
    int aw;
    int current_ws;
    
    Notif notifs[MAX_NOTIF];
    int nc;
    
    /* Terminal */
    char term_logs[MAX_TERM_LOGS][80];
    int term_log_count;
    char term_input[128];
    
    /* Desktop */
    int mx, my, mclick;
    int shift_pressed, super_pressed;
    int running;
    int vscodium_installed;
    int guide_page;
    int stats_tick;
    int fb_w, fb_h;
    int mouse_pressed, selecting;
    int sel_start_x, sel_start_y, sel_end_x, sel_end_y;
    int show_copy_dialog;
    char selected_text[512];
    char clipboard[1024];
    int notif_fd;
    int tty_fd;
    
    /* Font */
    uint8_t *backbuf;
    int backbuf_size;
    
    /* Timing */
    struct timespec last_frame;
} compositor_t;

/* ─── Exposed functions ─── */

/* compositor.c */
int compositor_init(compositor_t *c);
void compositor_run(compositor_t *c);
void compositor_shutdown(compositor_t *c);
void compositor_frame(compositor_t *c);

/* drm.c */
int drm_init(compositor_t *c);
void drm_swap(compositor_t *c);
void drm_shutdown(compositor_t *c);

/* input.c */
int input_init(compositor_t *c);
void input_poll(compositor_t *c);
void input_shutdown(compositor_t *c);

/* wl_server.c */
int wl_server_init(compositor_t *c);
void wl_server_poll(compositor_t *c);
void wl_server_shutdown(compositor_t *c);
void wl_send_event(wl_client_t *client, uint32_t id, uint32_t opcode, const void *data, uint32_t len);
void wl_post_error(wl_client_t *client, uint32_t id, uint32_t code, const char *msg);

/* shell.c */
void shell_init(compositor_t *c);
void shell_draw(compositor_t *c);
void shell_handle_click(compositor_t *c);
void shell_handle_key(compositor_t *c, int code);
void shell_handle_mouse(compositor_t *c, int dx, int dy, int abs_x, int abs_y);
void extract_selected_text(compositor_t *c);
void shell_term_log(compositor_t *c, const char *msg);
void shell_notif(compositor_t *c, const char *title, const char *body);
void shell_beep(compositor_t *c, int freq, int ms);

/* render.c (C helpers) */
void render_clear(compositor_t *c, uint32_t color);
void render_pixel(compositor_t *c, int x, int y, uint32_t color);
void render_rect(compositor_t *c, int x, int y, int w, int h, uint32_t color);
void render_char(compositor_t *c, int x, int y, char ch, uint32_t fg, uint32_t bg);
void render_text(compositor_t *c, int x, int y, const char *s, uint32_t fg, uint32_t bg);
void render_rect_blend(compositor_t *c, int x, int y, int w, int h, uint32_t color);
uint32_t render_get_neon(compositor_t *c);

/* render.S (assembly routines) */
void asm_fill_rect32(uint32_t *buf, int stride, int w, int h, uint32_t color);
void asm_fill_hline32(uint32_t *buf, int stride, int x, int y, int w, uint32_t color);
void asm_copy_rect32(uint32_t *dst, int dst_stride, uint32_t *src, int src_stride, int w, int h);
void asm_blend_rect32(uint32_t *dst, int stride, int w, int h, uint32_t color);

/* font.S */
extern const uint8_t font8x8[];

#endif

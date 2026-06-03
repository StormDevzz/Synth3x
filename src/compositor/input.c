/* Synth3x Compositor — Input Backend
 * Handles evdev input devices (mouse, keyboard, touchpad, touchscreen).
 * Uses poll-based event reading with O_NONBLOCK.
 */

#include "compositor.h"
#include <linux/input.h>

int input_init(compositor_t *c) {
    c->input_count = 0;
    
    /* Scan /dev/input/event* devices */
    for (int i = 0; i < 16 && c->input_count < 16; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        
        /* Get device name */
        struct input_id id;
        if (ioctl(fd, EVIOCGID, &id) < 0) {
            close(fd);
            continue;
        }
        
        char name[64] = "unknown";
        ioctl(fd, EVIOCGNAME(sizeof(name)), name);
        
        input_dev_t *dev = &c->input_devs[c->input_count];
        dev->fd = fd;
        dev->type = id.bustype;
        strncpy(dev->name, name, sizeof(dev->name) - 1);
        c->input_count++;
        
        fprintf(stderr, "Input: %s (bus=%d)\n", name, id.bustype);
    }
    
    /* Also try /dev/input/mice for PS/2 compatibility */
    int mfd = open("/dev/input/mice", O_RDONLY | O_NONBLOCK);
    if (mfd >= 0 && c->input_count < 16) {
        c->input_devs[c->input_count].fd = mfd;
        c->input_devs[c->input_count].type = 0xFFFF;
        strncpy(c->input_devs[c->input_count].name, "ps2-mouse", 63);
        c->input_count++;
    }
    
    /* Setup seat */
    c->seat.wl_id = WL_SEAT_ID;
    c->seat.capabilities = 1 | 2 | 4; /* pointer | keyboard | touch */
    c->seat.focus_client = NULL;
    c->seat.focus_surface = NULL;
    c->seat.serial = 1;
    c->seat.mx = c->fb_w / 2;
    c->seat.my = c->fb_h / 2;
    
    return c->input_count > 0 ? 0 : -1;
}

void input_poll(compositor_t *c) {
    for (int i = 0; i < c->input_count; i++) {
        int fd = c->input_devs[i].fd;
        if (fd < 0) continue;
        
        struct input_event ev;
        while (read(fd, &ev, sizeof(ev)) == (int)sizeof(ev)) {
            /* Handle relative mouse motion */
            if (ev.type == EV_REL) {
                int dx = 0, dy = 0;
                if (ev.code == REL_X) dx = ev.value * 2;
                if (ev.code == REL_Y) dy = ev.value * 2;
                shell_handle_mouse(c, dx, dy, -1, -1);
            }
            
            /* Handle absolute motion (touchpad, touchscreen) */
            if (ev.type == EV_ABS) {
                int abs_x = -1, abs_y = -1;
                if (ev.code == ABS_X || ev.code == ABS_MT_POSITION_X) {
                    abs_x = ev.value * c->fb_w / 65535;
                }
                if (ev.code == ABS_Y || ev.code == ABS_MT_POSITION_Y) {
                    abs_y = ev.value * c->fb_h / 65535;
                }
                if (abs_x >= 0 || abs_y >= 0) {
                    shell_handle_mouse(c, 0, 0, abs_x, abs_y);
                }
            }
            
            /* Handle keyboard scancodes (EV_KEY with EV_MSC) */
            if (ev.type == EV_KEY && ev.code > 0) {
                if (ev.code == BTN_LEFT || ev.code == BTN_TOUCH ||
                    ev.code == BTN_TOOL_FINGER) {
                    if (ev.value == 1) {
                        c->mclick = 1;
                        c->mouse_pressed = 1;
                        c->seat.buttons |= 0x100;
                    } else if (ev.value == 0) {
                        c->mouse_pressed = 0;
                        c->mclick = 0;
                        c->seat.buttons &= ~0x100;
                    }
                } else if (ev.code == BTN_RIGHT) {
                    if (ev.value == 1) c->seat.buttons |= 0x200;
                    else c->seat.buttons &= ~0x200;
                } else if (ev.code == BTN_MIDDLE) {
                    if (ev.value == 1) c->seat.buttons |= 0x400;
                    else c->seat.buttons &= ~0x400;
                } else if (ev.value == 1) {
                    /* Handle keyboard scancodes */
                    c->seat.serial++;
                    shell_handle_key(c, ev.code);
                } else if (ev.value == 0) {
                    if (ev.code == 42 || ev.code == 54)
                        c->shift_pressed = 0;
                    if (ev.code == 125 || ev.code == 126)
                        c->super_pressed = 0;
                }
            }
            
            /* Handle keyboard scancodes via MSC_SCAN */
            if (ev.type == EV_MSC && ev.code == MSC_SCAN) {
                /* scancode in ev.value */
            }
        }
        
        /* Handle modifier key state updates from EV_KEY events */
    }
}

void input_shutdown(compositor_t *c) {
    for (int i = 0; i < c->input_count; i++) {
        if (c->input_devs[i].fd >= 0) {
            close(c->input_devs[i].fd);
            c->input_devs[i].fd = -1;
        }
    }
    c->input_count = 0;
}

#include "ui.h"
#include "canvas.h"
#include "tools.h"
#include <cstdio>
#include <cstdarg>
#include <cstring>

UI::UI() : ren(nullptr), status_timer(0), picker_on(0), picker_fg(1),
           picker_r(128), picker_g(128), picker_b(128), picker_drag(0),
           about_on(0) {
    status_msg[0] = 0;
}

UI::~UI() {}

void UI::init(SDL_Renderer *r) { ren = r; }

/* ─── Text rendering ─── */

SDL_Texture *UI::render_text_tex(const char *s, int r, int g, int b) {
    int len = strlen(s);
    if (len == 0) return nullptr;
    int tw = len * 9 + 4;
    int th = 18;

    cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, tw, th);
    cairo_t *ct = cairo_create(surf);

    cairo_set_source_rgba(ct, 0, 0, 0, 0);
    cairo_paint(ct);
    cairo_select_font_face(ct, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(ct, 14.0);
    cairo_set_source_rgb(ct, r/255.0, g/255.0, b/255.0);
    cairo_move_to(ct, 2, 14);
    cairo_show_text(ct, s);

    SDL_Surface *sdl_surf = SDL_CreateRGBSurfaceFrom(
        cairo_image_surface_get_data(surf),
        tw, th, 32, cairo_image_surface_get_stride(surf),
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (!sdl_surf) { cairo_destroy(ct); cairo_surface_destroy(surf); return nullptr; }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, sdl_surf);
    SDL_FreeSurface(sdl_surf);
    cairo_destroy(ct);
    cairo_surface_destroy(surf);
    return tex;
}

void UI::render_text(const char *s, int x, int y, int r, int g, int b) {
    SDL_Texture *tex = render_text_tex(s, r, g, b);
    if (!tex) return;
    int w, h; SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(ren, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

void UI::render_text_centered(const char *s, SDL_Rect *rect, int r, int g, int b) {
    SDL_Texture *tex = render_text_tex(s, r, g, b);
    if (!tex) return;
    int tw, th; SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
    SDL_Rect dst = {rect->x + (rect->w - tw)/2,
                    rect->y + (rect->h - th)/2, tw, th};
    SDL_RenderCopy(ren, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

/* ─── Status ─── */

void UI::set_status(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(status_msg, sizeof(status_msg), fmt, ap);
    va_end(ap);
    status_timer = SDL_GetTicks();
}

void UI::render_statusbar() {
    int y = 680;
    SDL_SetRenderDrawColor(ren, 30, 30, 32, 255);
    SDL_Rect bar = {0, y, 1000, 20};
    SDL_RenderFillRect(ren, &bar);
    if (status_msg[0] && SDL_GetTicks() - status_timer < 3000) {
        int slen = strlen(status_msg);
        render_text(status_msg, 1000 - slen * 9 - 10, y + 2, 200, 200, 120);
    }
}

/* ─── Toolbar ─── */

void UI::render_toolbar(const AppState &st) {
    int x = 5, y = 5;

    for (int i = 0; i < TOOL_COUNT; i++) {
        SDL_Rect r = {x, y + i * 28, TOOL_W - 10, 24};
        if (i == st.tool) {
            SDL_SetRenderDrawColor(ren, 60, 100, 180, 255);
            SDL_RenderFillRect(ren, &r);
        } else {
            SDL_SetRenderDrawColor(ren, 55, 55, 60, 255);
            SDL_RenderFillRect(ren, &r);
        }
        SDL_SetRenderDrawColor(ren, 80, 80, 85, 255);
        SDL_RenderDrawRect(ren, &r);
        render_text(tool_names[i], r.x + 6, r.y + 4, 220, 220, 220);
    }

    int by = y + TOOL_COUNT * 28 + 6;

    render_text("Size:", x + 4, by, 180, 180, 180);
    SDL_Rect sld_bg = {x + 4, by + 16, TOOL_W - 18, 10};
    SDL_SetRenderDrawColor(ren, 60, 60, 65, 255);
    SDL_RenderFillRect(ren, &sld_bg);
    float pct = (st.brush_size - 1) / 19.0f;
    SDL_Rect sld_fill = {sld_bg.x + 1, sld_bg.y + 1,
                         (int)((TOOL_W - 20) * pct), sld_bg.h - 2};
    SDL_SetRenderDrawColor(ren, 100, 150, 220, 255);
    SDL_RenderFillRect(ren, &sld_fill);

    char sz[8]; snprintf(sz, sizeof(sz), "%d", st.brush_size);
    render_text(sz, x + TOOL_W - 30, by, 200, 200, 200);

    int cy = by + 34;

    SDL_Rect fg_box = {x + 4, cy, 24, 24};
    SDL_SetRenderDrawColor(ren, st.fg.r, st.fg.g, st.fg.b, 255);
    SDL_RenderFillRect(ren, &fg_box);
    SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
    SDL_RenderDrawRect(ren, &fg_box);
    render_text("FG", x + 32, cy + 4, 200, 200, 200);

    SDL_SetRenderDrawColor(ren, 150, 150, 150, 255);
    SDL_RenderDrawLine(ren, x + TOOL_W - 20, cy + 4, x + TOOL_W - 10, cy + 14);
    SDL_RenderDrawLine(ren, x + TOOL_W - 20, cy + 14, x + TOOL_W - 10, cy + 4);

    SDL_Rect bg_box = {x + 4, cy + 28, 24, 24};
    SDL_SetRenderDrawColor(ren, st.bg.r, st.bg.g, st.bg.b, 255);
    SDL_RenderFillRect(ren, &bg_box);
    SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
    SDL_RenderDrawRect(ren, &bg_box);
    render_text("BG", x + 32, cy + 32, 200, 200, 200);

    char hex[10];
    snprintf(hex, sizeof(hex), "#%02X%02X%02X", st.fg.r, st.fg.g, st.fg.b);
    render_text(hex, x + 32, cy + 54, 180, 180, 150);
    snprintf(hex, sizeof(hex), "#%02X%02X%02X", st.bg.r, st.bg.g, st.bg.b);
    render_text(hex, x + 32, cy + 68, 180, 180, 150);

    int ay = cy + 88;
    const char *btns[] = {"New Canvas", "Save PNG", "Undo", "About"};
    for (int i = 0; i < 4; i++) {
        SDL_Rect btn = {x + 4, ay + i * 26, TOOL_W - 18, 22};
        SDL_SetRenderDrawColor(ren, 60, 60, 68, 255);
        SDL_RenderFillRect(ren, &btn);
        SDL_SetRenderDrawColor(ren, 85, 85, 90, 255);
        SDL_RenderDrawRect(ren, &btn);
        render_text(btns[i], btn.x + 6, btn.y + 3, 200, 200, 200);
    }
}

void UI::render_palette(const Color &fg, const Color &bg) {
    for (int i = 0; i < 32; i++) {
        int px = 160 + (i % PAL_COLS) * (PAL_SW + 2);
        int py = 1 + (i / PAL_COLS) * (PAL_SW + 2);
        SDL_Rect r = {px, py, PAL_SW, PAL_SW};
        SDL_SetRenderDrawColor(ren, palette[i].r, palette[i].g,
                                     palette[i].b, 255);
        SDL_RenderFillRect(ren, &r);
        if (palette[i] == fg || palette[i] == bg) {
            SDL_SetRenderDrawColor(ren, 255, 255, 0, 255);
        } else {
            SDL_SetRenderDrawColor(ren, 80, 80, 85, 255);
        }
        SDL_RenderDrawRect(ren, &r);
    }
}

void UI::render_all(Canvas &canvas, const AppState &state) {
    SDL_SetRenderDrawColor(ren, 45, 45, 48, 255);
    SDL_RenderClear(ren);

    SDL_Rect cr = {160, 40, 800, 620};
    SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
    SDL_RenderFillRect(ren, &cr);

    extern SDL_Texture *canvas_get_texture(Canvas &c);
    SDL_RenderCopy(ren, canvas_get_texture(canvas), nullptr, &cr);

    render_toolbar(state);
    render_palette(state.fg, state.bg);
    render_statusbar();

    if (picker_on) picker_render();
    if (about_on) about_render();
}

/* ─── Hit testing ─── */

static int hit(int mx, int my, int x, int y, int w, int h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}

int UI::hit_tool(int mx, int my) {
    for (int i = 0; i < TOOL_COUNT; i++) {
        SDL_Rect r = {5, 5 + i * 28, TOOL_W - 10, 24};
        if (hit(mx, my, r.x, r.y, r.w, r.h)) return i;
    }
    return -1;
}

int UI::hit_brush_slider(int mx, int my, int *out) {
    SDL_Rect sld = {9, 5 + TOOL_COUNT * 28 + 6 + 16, TOOL_W - 18, 10};
    if (hit(mx, my, sld.x, sld.y, sld.w, sld.h)) {
        float pct = (mx - sld.x) / (float)sld.w;
        *out = (int)(pct * 19) + 1;
        if (*out < 1) *out = 1;
        if (*out > 20) *out = 20;
        return 1;
    }
    return 0;
}

int UI::hit_fg_box(int mx, int my) {
    return hit(mx, my, 9, 5 + TOOL_COUNT * 28 + 6 + 34, 24, 24);
}

int UI::hit_bg_box(int mx, int my) {
    return hit(mx, my, 9, 5 + TOOL_COUNT * 28 + 6 + 34 + 28, 24, 24);
}

int UI::hit_swap_colors(int mx, int my) {
    return hit(mx, my, 5 + TOOL_W - 24, 5 + TOOL_COUNT * 28 + 6 + 34, 18, 18);
}

int UI::hit_palette(int mx, int my, int button, Color *fg, Color *bg) {
    if (my >= 1 && my < PAL_H + 2 && mx >= 160) {
        int col = (mx - 160) / (PAL_SW + 2);
        int row = (my - 1) / (PAL_SW + 2);
        int idx = row * PAL_COLS + col;
        if (idx >= 0 && idx < 32) {
            if (button == SDL_BUTTON_LEFT) *fg = palette[idx];
            else *bg = palette[idx];
            return 1;
        }
    }
    return 0;
}

int UI::hit_button(int mx, int my, const char *label) {
    int ay = 5 + TOOL_COUNT * 28 + 6 + 34 + 88;
    const char *btns[] = {"New Canvas", "Save PNG", "Undo", "About"};
    for (int i = 0; i < 4; i++) {
        if (strcmp(label, btns[i]) == 0) {
            SDL_Rect btn = {9, ay + i * 26, TOOL_W - 18, 22};
            return hit(mx, my, btn.x, btn.y, btn.w, btn.h);
        }
    }
    return 0;
}

/* ─── Color picker ─── */

int UI::picker_active() { return picker_on; }
void UI::picker_open(int for_fg, const Color &c) {
    picker_on = 1; picker_fg = for_fg;
    picker_r = c.r; picker_g = c.g; picker_b = c.b;
    picker_drag = 0;
}

void UI::picker_render() {
    int bx = 500 - 150, by = 350 - 100;

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_Rect over = {0, 0, 1000, 700};
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
    SDL_RenderFillRect(ren, &over);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

    SDL_Rect box = {bx, by, 300, 200};
    SDL_SetRenderDrawColor(ren, 45, 45, 50, 255);
    SDL_RenderFillRect(ren, &box);
    SDL_SetRenderDrawColor(ren, 80, 80, 85, 255);
    SDL_RenderDrawRect(ren, &box);
    render_text("Color Picker", bx + 80, by + 8, 220, 220, 220);

    int *vals[] = {&picker_r, &picker_g, &picker_b};
    const char *labels[] = {"R", "G", "B"};
    uint8_t cols[] = {255,80,80, 80,255,80, 80,80,255};

    for (int i = 0; i < 3; i++) {
        int ly = by + 32 + i * 30;
        render_text(labels[i], bx + 10, ly, cols[i*3], cols[i*3+1], cols[i*3+2]);
        SDL_Rect tr = {bx + 35, ly, 180, 18};
        SDL_SetRenderDrawColor(ren, 55, 55, 60, 255);
        SDL_RenderFillRect(ren, &tr);
        SDL_SetRenderDrawColor(ren, 70, 70, 75, 255);
        SDL_RenderDrawRect(ren, &tr);
        int v = *vals[i];
        SDL_Rect fr = {tr.x + 2, tr.y + 2, (int)(176 * v / 255.0), 14};
        uint8_t cr = i==0?255:60, cg = i==1?255:60, cb = i==2?255:60;
        SDL_SetRenderDrawColor(ren, cr, cg, cb, 255);
        SDL_RenderFillRect(ren, &fr);
        char vbuf[8]; snprintf(vbuf, sizeof(vbuf), "%d", v);
        render_text(vbuf, bx + 225, ly, 200, 200, 200);
    }

    SDL_Rect prev = {bx + 230, by + 100, 50, 50};
    SDL_SetRenderDrawColor(ren, picker_r, picker_g, picker_b, 255);
    SDL_RenderFillRect(ren, &prev);
    SDL_SetRenderDrawColor(ren, 150, 150, 150, 255);
    SDL_RenderDrawRect(ren, &prev);
    render_text("preview", bx + 228, by + 155, 150, 150, 150);

    SDL_Rect ok_btn = {bx + 40, by + 165, 80, 26};
    SDL_SetRenderDrawColor(ren, 50, 100, 50, 255);
    SDL_RenderFillRect(ren, &ok_btn);
    SDL_SetRenderDrawColor(ren, 80, 130, 80, 255);
    SDL_RenderDrawRect(ren, &ok_btn);
    render_text_centered("OK", &ok_btn, 220, 255, 220);

    SDL_Rect ca_btn = {bx + 170, by + 165, 80, 26};
    SDL_SetRenderDrawColor(ren, 100, 50, 50, 255);
    SDL_RenderFillRect(ren, &ca_btn);
    SDL_SetRenderDrawColor(ren, 130, 80, 80, 255);
    SDL_RenderDrawRect(ren, &ca_btn);
    render_text_centered("Cancel", &ca_btn, 255, 220, 220);
}

void UI::picker_handle(SDL_Event &ev, Color *fg, Color *bg) {
    int bx = 500 - 150, by = 350 - 100;

    if (ev.type == SDL_MOUSEBUTTONDOWN) {
        int mx = ev.button.x, my = ev.button.y;
        int *vals[] = {&picker_r, &picker_g, &picker_b};

        for (int i = 0; i < 3; i++) {
            SDL_Rect tr = {bx + 35, by + 32 + i * 30, 180, 18};
            if (hit(mx, my, tr.x, tr.y, tr.w, tr.h)) {
                *vals[i] = (int)((mx - tr.x) * 255.0 / tr.w);
                if (*vals[i] < 0) *vals[i] = 0;
                if (*vals[i] > 255) *vals[i] = 255;
                picker_drag = i + 1;
                return;
            }
        }

        SDL_Rect ok_btn = {bx + 40, by + 165, 80, 26};
        if (hit(mx, my, ok_btn.x, ok_btn.y, ok_btn.w, ok_btn.h)) {
            Color c = {(uint8_t)picker_r, (uint8_t)picker_g, (uint8_t)picker_b};
            if (picker_fg) *fg = c; else *bg = c;
            picker_on = 0;
            set_status("Color set");
            return;
        }
        SDL_Rect ca_btn = {bx + 170, by + 165, 80, 26};
        if (hit(mx, my, ca_btn.x, ca_btn.y, ca_btn.w, ca_btn.h)) {
            picker_on = 0;
            return;
        }
        SDL_Rect box = {bx, by, 300, 200};
        if (!hit(mx, my, box.x, box.y, box.w, box.h))
            picker_on = 0;
    }
    if (ev.type == SDL_MOUSEBUTTONUP) picker_drag = 0;
    if (ev.type == SDL_MOUSEMOTION && picker_drag > 0) {
        int mx = ev.motion.x;
        int i = picker_drag - 1;
        int *vals[] = {&picker_r, &picker_g, &picker_b};
        *vals[i] = (int)((mx - (bx + 35)) * 255.0 / 180.0);
        if (*vals[i] < 0) *vals[i] = 0;
        if (*vals[i] > 255) *vals[i] = 255;
    }
}

/* ─── About ─── */

void UI::about_open() { about_on = 1; }
void UI::about_close() { about_on = 0; }
int UI::about_active() { return about_on; }

void UI::about_render() {
    int bx = 500 - 200, by = 350 - 80;

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_Rect over = {0, 0, 1000, 700};
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
    SDL_RenderFillRect(ren, &over);
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

    SDL_Rect box = {bx, by, 400, 160};
    SDL_SetRenderDrawColor(ren, 40, 40, 45, 255);
    SDL_RenderFillRect(ren, &box);
    SDL_SetRenderDrawColor(ren, 80, 80, 85, 255);
    SDL_RenderDrawRect(ren, &box);

    render_text("SynPaint v1.0", bx + 130, by + 15, 255, 200, 100);
    render_text("A simple paint program for Synth3x", bx + 70, by + 45, 200, 200, 200);
    render_text("Built with SDL2 + Cairo", bx + 105, by + 70, 180, 180, 180);
    render_text("Click anywhere to close", bx + 105, by + 100, 150, 150, 150);
}

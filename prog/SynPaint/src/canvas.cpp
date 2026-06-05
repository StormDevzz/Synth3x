#include "canvas.h"
#include <cstring>
#include <cmath>
#include <cstdio>
#include <ctime>

Canvas::Canvas() : surf(nullptr), cr(nullptr), tex(nullptr), ren(nullptr) {}

Canvas::~Canvas() { shutdown(); }

int Canvas::init(SDL_Renderer *r) {
    ren = r;
    surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
    if (!surf) return -1;
    cr = cairo_create(surf);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB32,
                            SDL_TEXTUREACCESS_STREAMING, W, H);
    if (!tex) return -1;
    sync();
    return 0;
}

void Canvas::shutdown() {
    if (cr) cairo_destroy(cr);
    if (surf) cairo_surface_destroy(surf);
    if (tex) SDL_DestroyTexture(tex);
}

void Canvas::sync() {
    void *pixels; int pitch;
    SDL_LockTexture(tex, nullptr, &pixels, &pitch);
    memcpy(pixels, cairo_image_surface_get_data(surf), H * pitch);
    SDL_UnlockTexture(tex);
}

void Canvas::save_undo() {
    UndoState u;
    u.w = W; u.h = H;
    u.data.resize(W * H * 4);
    memcpy(u.data.data(), cairo_image_surface_get_data(surf), W * H * 4);
    undo_stack.push_back(u);
    if ((int)undo_stack.size() > MAX_UNDO)
        undo_stack.erase(undo_stack.begin());
}

void Canvas::undo() {
    if (undo_stack.empty()) return;
    UndoState &u = undo_stack.back();
    if (cr) cairo_destroy(cr);
    if (surf) cairo_surface_destroy(surf);
    surf = cairo_image_surface_create_for_data(
        u.data.data(), CAIRO_FORMAT_ARGB32, u.w, u.h, u.w * 4);
    cr = cairo_create(surf);
    undo_stack.pop_back();
    sync();
}

void Canvas::clear() {
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);
    sync();
}

void Canvas::restore_from(const UndoState &u) {
    memcpy(cairo_image_surface_get_data(surf), u.data.data(), u.w * u.h * 4);
    cairo_surface_mark_dirty(surf);
}

void Canvas::stroke_brush(int x0, int y0, int x1, int y1,
                           float r, float g, float b, float size) {
    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, size);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, x0, y0);
    cairo_line_to(cr, x1, y1);
    cairo_stroke(cr);
}

void Canvas::stroke_eraser(int x0, int y0, int x1, int y1,
                            float r, float g, float b, float size) {
    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, size);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, x0, y0);
    cairo_line_to(cr, x1, y1);
    cairo_stroke(cr);
}

void Canvas::stroke_line(int x0, int y0, int x1, int y1,
                          float r, float g, float b, float size) {
    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, size);
    cairo_move_to(cr, x0, y0);
    cairo_line_to(cr, x1, y1);
    cairo_stroke(cr);
}

void Canvas::stroke_rect(int x0, int y0, int x1, int y1,
                          float r, float g, float b, float size) {
    int x = x0 < x1 ? x0 : x1;
    int y = y0 < y1 ? y0 : y1;
    int w = abs(x1 - x0);
    int h = abs(y1 - y0);
    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, size);
    cairo_rectangle(cr, x, y, w, h);
    cairo_stroke(cr);
}

void Canvas::stroke_circle(int cx, int cy, int ex, int ey,
                            float r, float g, float b, float size) {
    int dx = ex - cx;
    int dy = ey - cy;
    int rad = (int)sqrt(dx*dx + dy*dy);
    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, size);
    cairo_arc(cr, cx, cy, rad, 0, 2 * M_PI);
    cairo_stroke(cr);
}

void Canvas::fill_flood(int x, int y, float r, float g, float b) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    unsigned char *data = cairo_image_surface_get_data(surf);
    int stride = cairo_image_surface_get_stride(surf);

    uint8_t target[4];
    int idx = y * stride + x * 4;
    target[0] = data[idx]; target[1] = data[idx+1];
    target[2] = data[idx+2]; target[3] = data[idx+3];

    uint8_t fill[4] = {(uint8_t)(b*255), (uint8_t)(g*255),
                       (uint8_t)(r*255), 255};
    if (memcmp(target, fill, 4) == 0) return;

    std::vector<SDL_Point> stack;
    stack.push_back({x, y});
    while (!stack.empty()) {
        SDL_Point p = stack.back(); stack.pop_back();
        if (p.x < 0 || p.x >= W || p.y < 0 || p.y >= H) continue;
        idx = p.y * stride + p.x * 4;
        if (memcmp(&data[idx], target, 4) != 0) continue;
        data[idx] = fill[0]; data[idx+1] = fill[1];
        data[idx+2] = fill[2]; data[idx+3] = fill[3];
        stack.push_back({p.x+1, p.y});
        stack.push_back({p.x-1, p.y});
        stack.push_back({p.x, p.y+1});
        stack.push_back({p.x, p.y-1});
    }
    cairo_surface_mark_dirty(surf);
}

void Canvas::save_png(const char *path) {
    cairo_surface_write_to_png(surf, path);
}

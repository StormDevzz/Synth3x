#ifndef CANVAS_H
#define CANVAS_H

#include <SDL2/SDL.h>
#include <cairo/cairo.h>
#include <vector>
#include <cstdint>

struct UndoState {
    std::vector<uint8_t> data;
    int w, h;
};

class Canvas {
public:
    Canvas();
    ~Canvas();

    int init(SDL_Renderer *ren);
    void shutdown();
    void sync();

    void save_undo();
    void undo();
    void clear();

    void stroke_brush(int x0, int y0, int x1, int y1,
                      float r, float g, float b, float size);
    void stroke_eraser(int x0, int y0, int x1, int y1,
                       float r, float g, float b, float size);
    void stroke_line(int x0, int y0, int x1, int y1,
                     float r, float g, float b, float size);
    void stroke_rect(int x0, int y0, int x1, int y1,
                     float r, float g, float b, float size);
    void stroke_circle(int cx, int cy, int ex, int ey,
                       float r, float g, float b, float size);
    void fill_flood(int x, int y, float r, float g, float b);

    void save_png(const char *path);

    int width() const { return W; }
    int height() const { return H; }
    void *data() { return cairo_image_surface_get_data(surf); }

    /* For preview restore */
    void restore_from(const UndoState &u);
    int  has_undo() const { return !undo_stack.empty(); }
    const UndoState &peek_undo() const { return undo_stack.back(); }

    static const int W = 800;
    static const int H = 620;
    static const int MAX_UNDO = 20;

    SDL_Texture *tex;

private:
    cairo_surface_t *surf;
    cairo_t *cr;
    SDL_Renderer *ren;
    std::vector<UndoState> undo_stack;
};

#endif

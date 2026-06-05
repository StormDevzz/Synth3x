#ifndef UI_H
#define UI_H

#include <SDL2/SDL.h>
#include "palette.h"

class Canvas;

class UI {
public:
    UI();
    ~UI();

    void init(SDL_Renderer *ren);

    /* Main render */
    void render_all(Canvas &canvas, const struct AppState &state);

    /* Toolbar hit testing */
    int  hit_tool(int mx, int my);
    int  hit_brush_slider(int mx, int my, int *out_size);
    int  hit_fg_box(int mx, int my);
    int  hit_bg_box(int mx, int my);
    int  hit_swap_colors(int mx, int my);
    int  hit_palette(int mx, int my, int button, Color *fg, Color *bg);
    int  hit_button(int mx, int my, const char *label);

    /* Color picker */
    int  picker_active();
    void picker_open(int for_fg, const Color &c);
    void picker_handle(SDL_Event &ev, Color *fg, Color *bg);
    void picker_render();

    /* About */
void about_open();
void about_close();
int  about_active();
    void about_render();

    /* Status */
    void set_status(const char *fmt, ...);
    void render_statusbar();

private:
    SDL_Renderer *ren;
    char status_msg[256];
    Uint32 status_timer;

    int picker_on, picker_fg;
    int picker_r, picker_g, picker_b;
    int picker_drag;

    int about_on;

    SDL_Texture *render_text_tex(const char *s, int r, int g, int b);
    void render_text(const char *s, int x, int y, int r, int g, int b);
    void render_text_centered(const char *s, SDL_Rect *rect, int r, int g, int b);

    void render_toolbar(const struct AppState &state);
    void render_palette(const Color &fg, const Color &bg);

    static const int TOOL_W = 140;
};

struct AppState {
    int tool;
    int brush_size;
    Color fg, bg;
    int mouse_x, mouse_y;
};

#endif

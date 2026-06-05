#ifndef APP_H
#define APP_H

#include <SDL2/SDL.h>
#include "palette.h"
#include "tools.h"
#include "canvas.h"
#include "ui.h"

class SynPaint {
public:
    SynPaint();
    ~SynPaint();

    int  init();
    void run();

private:
    SDL_Window   *win;
    SDL_Renderer *ren;
    Canvas        canvas;
    UI            ui;

    ToolState ts;
    Color fg, bg;

    int show_picker;

    void handle_event(SDL_Event &ev);
    void handle_mousedown(SDL_MouseButtonEvent &btn);
    void handle_mouseup(SDL_MouseButtonEvent &btn);
    void handle_mousemove(SDL_MouseMotionEvent &mot);
    void handle_key(SDL_KeyboardEvent &key);
    void do_draw(int x, int y);
    void finish_shape();
    void update_cursor();
    void render();

    static const int WIN_W = 1000;
    static const int WIN_H = 700;
    static const int CANVAS_X = 160;
    static const int CANVAS_Y = 40;
};

#endif

#include "app.h"
#include <cstdio>
#include <ctime>

SDL_Texture *canvas_get_texture(Canvas &c) {
    return c.tex;
}

SynPaint::SynPaint() : win(nullptr), ren(nullptr), show_picker(0) {
    ts.tool = TOOL_BRUSH;
    ts.brush_size = 3;
    ts.drawing = 0;
    ts.start_x = ts.start_y = 0;
    ts.last_x = ts.last_y = 0;
    ts.mouse_x = ts.mouse_y = 0;
    fg = {0, 0, 0};
    bg = {255, 255, 255};
}

SynPaint::~SynPaint() {
    canvas.shutdown();
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    SDL_Quit();
}

int SynPaint::init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return -1;
    }
    win = SDL_CreateWindow("SynPaint - S3n Paint",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           WIN_W, WIN_H,
                           SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!win) { fprintf(stderr, "Window failed\n"); return -1; }
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) { fprintf(stderr, "Renderer failed\n"); return -1; }

    if (canvas.init(ren) < 0) { fprintf(stderr, "Canvas failed\n"); return -1; }
    ui.init(ren);
    ui.set_status("Welcome to SynPaint!");
    SDL_ShowCursor(SDL_TRUE);
    return 0;
}

void SynPaint::update_cursor() {
    int mx = ts.mouse_x, my = ts.mouse_y;
    if (mx >= CANVAS_X && mx < CANVAS_X + 800 &&
        my >= CANVAS_Y && my < CANVAS_Y + 620)
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
    else
        SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
}

void SynPaint::render() {
    AppState st;
    st.tool = ts.tool;
    st.brush_size = ts.brush_size;
    st.fg = fg;
    st.bg = bg;
    st.mouse_x = ts.mouse_x;
    st.mouse_y = ts.mouse_y;

    ui.render_all(canvas, st);
    SDL_RenderPresent(ren);
}

void SynPaint::handle_event(SDL_Event &ev) {
    if (ui.picker_active()) {
        ui.picker_handle(ev, &fg, &bg);
        return;
    }
    if (ui.about_active()) {
        if (ev.type == SDL_MOUSEBUTTONDOWN) ui.about_close();
        return;
    }

    switch (ev.type) {
    case SDL_MOUSEBUTTONDOWN: handle_mousedown(ev.button); break;
    case SDL_MOUSEBUTTONUP:   handle_mouseup(ev.button);   break;
    case SDL_MOUSEMOTION:     handle_mousemove(ev.motion); break;
    case SDL_KEYDOWN:         handle_key(ev.key);         break;
    }
}

void SynPaint::handle_mousedown(SDL_MouseButtonEvent &btn) {
    int mx = btn.x, my = btn.y;

    if (mx < 160) {
        int ti = ui.hit_tool(mx, my);
        if (ti >= 0) { ts.tool = ti; return; }

        int sz;
        if (ui.hit_brush_slider(mx, my, &sz)) {
            ts.brush_size = sz;
            ui.set_status("Size: %d", sz);
            return;
        }
        if (ui.hit_fg_box(mx, my)) {
            ui.picker_open(1, fg);
            return;
        }
        if (ui.hit_bg_box(mx, my)) {
            ui.picker_open(0, bg);
            return;
        }
        if (ui.hit_swap_colors(mx, my)) {
            Color t = fg; fg = bg; bg = t;
            ui.set_status("Swapped");
            return;
        }
        if (ui.hit_button(mx, my, "New Canvas")) {
            canvas.save_undo();
            canvas.clear();
            ui.set_status("Cleared");
            return;
        }
        if (ui.hit_button(mx, my, "Save PNG")) {
            char path[256]; time_t t = time(nullptr);
            struct tm *lt = localtime(&t);
            snprintf(path, sizeof(path), "synpaint_%04d%02d%02d_%02d%02d%02d.png",
                     lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday,
                     lt->tm_hour, lt->tm_min, lt->tm_sec);
            canvas.save_png(path);
            ui.set_status("Saved: %s", path);
            return;
        }
        if (ui.hit_button(mx, my, "Undo")) {
            canvas.undo();
            ui.set_status("Undo");
            return;
        }
        if (ui.hit_button(mx, my, "About")) {
            ui.about_open();
            return;
        }
        return;
    }

    if (ui.hit_palette(mx, my, btn.button, &fg, &bg)) {
        ui.set_status("#%02X%02X%02X", fg.r, fg.g, fg.b);
        return;
    }

    if (mx >= CANVAS_X && mx < CANVAS_X + 800 &&
        my >= CANVAS_Y && my < CANVAS_Y + 620 &&
        btn.button == SDL_BUTTON_LEFT) {
        if (ts.tool == TOOL_FILL) {
            canvas.save_undo();
            canvas.fill_flood(mx - CANVAS_X, my - CANVAS_Y,
                              fg.r/255.0, fg.g/255.0, fg.b/255.0);
            canvas.sync();
            return;
        }
        canvas.save_undo();
        ts.drawing = 1;
        ts.start_x = ts.last_x = mx;
        ts.start_y = ts.last_y = my;
    }
}

void SynPaint::handle_mouseup(SDL_MouseButtonEvent &btn) {
    if (!ts.drawing) return;
    ts.drawing = 0;
    finish_shape();
}

void SynPaint::finish_shape() {
    float r = fg.r/255.0, g = fg.g/255.0, b = fg.b/255.0;
    float sz = ts.brush_size;
    int cx = ts.start_x - CANVAS_X;
    int cy = ts.start_y - CANVAS_Y;
    int mx = ts.mouse_x - CANVAS_X;
    int my = ts.mouse_y - CANVAS_Y;

    if (canvas.has_undo())
        canvas.restore_from(canvas.peek_undo());

    if (ts.tool == TOOL_LINE)
        canvas.stroke_line(cx, cy, mx, my, r, g, b, sz);
    else if (ts.tool == TOOL_RECT)
        canvas.stroke_rect(cx, cy, mx, my, r, g, b, sz);
    else if (ts.tool == TOOL_CIRCLE)
        canvas.stroke_circle(cx, cy, mx, my, r, g, b, sz);

    canvas.sync();
}

void SynPaint::handle_mousemove(SDL_MouseMotionEvent &mot) {
    ts.mouse_x = mot.x;
    ts.mouse_y = mot.y;
    update_cursor();
    if (!ts.drawing) return;

    float r = fg.r/255.0, g = fg.g/255.0, b = fg.b/255.0;
    float sz = ts.brush_size;
    int x0 = ts.last_x - CANVAS_X;
    int y0 = ts.last_y - CANVAS_Y;
    int x1 = mot.x - CANVAS_X;
    int y1 = mot.y - CANVAS_Y;

    if (ts.tool == TOOL_PEN)
        canvas.stroke_brush(x0, y0, x1, y1, r, g, b, 1);
    else if (ts.tool == TOOL_BRUSH)
        canvas.stroke_brush(x0, y0, x1, y1, r, g, b, sz);
    else if (ts.tool == TOOL_ERASER)
        canvas.stroke_eraser(x0, y0, x1, y1,
                             bg.r/255.0, bg.g/255.0, bg.b/255.0, sz);
    else {
        /* Shape preview */
        if (canvas.has_undo())
            canvas.restore_from(canvas.peek_undo());

        int cx = ts.start_x - CANVAS_X;
        int cy = ts.start_y - CANVAS_Y;
        if (ts.tool == TOOL_LINE) canvas.stroke_line(cx, cy, x1, y1, r, g, b, sz);
        else if (ts.tool == TOOL_RECT) canvas.stroke_rect(cx, cy, x1, y1, r, g, b, sz);
        else if (ts.tool == TOOL_CIRCLE) canvas.stroke_circle(cx, cy, x1, y1, r, g, b, sz);
    }

    ts.last_x = mot.x;
    ts.last_y = mot.y;
    canvas.sync();
}

void SynPaint::handle_key(SDL_KeyboardEvent &key) {
    if (key.keysym.sym == SDLK_z && (SDL_GetModState() & KMOD_CTRL)) {
        canvas.undo();
        ui.set_status("Undo");
    }
    if (key.keysym.sym == SDLK_s && (SDL_GetModState() & KMOD_CTRL)) {
        char path[256]; time_t t = time(nullptr);
        struct tm *lt = localtime(&t);
        snprintf(path, sizeof(path), "synpaint_%04d%02d%02d_%02d%02d%02d.png",
                 lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday,
                 lt->tm_hour, lt->tm_min, lt->tm_sec);
        canvas.save_png(path);
        ui.set_status("Saved: %s", path);
    }
    if (key.keysym.sym == SDLK_n && (SDL_GetModState() & KMOD_CTRL)) {
        canvas.save_undo();
        canvas.clear();
        ui.set_status("Cleared");
    }
}

void SynPaint::run() {
    SDL_Event ev;
    while (SDL_WaitEvent(&ev)) {
        if (ev.type == SDL_QUIT) break;
        handle_event(ev);
        render();
    }
}

#include <SDL2/SDL.h>
#include <cairo/cairo.h>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstdarg>

static const int WIN_W = 1000;
static const int WIN_H = 700;
static const int CANVAS_X = 160;
static const int CANVAS_Y = 40;
static const int CANVAS_W = 800;
static const int CANVAS_H = 620;
static const int PAL_H = 36;
static const int TOOL_W = 140;

enum Tool { TOOL_PEN, TOOL_BRUSH, TOOL_ERASER, TOOL_LINE, TOOL_RECT,
            TOOL_CIRCLE, TOOL_FILL, TOOL_COUNT };

static const char *tool_names[] = {
    "Pen", "Brush", "Eraser", "Line", "Rect", "Circle", "Fill"
};

struct Color { uint8_t r, g, b; };

static Color palette[] = {
    {0,0,0},{255,255,255},{255,0,0},{0,255,0},{0,0,255},{255,255,0},
    {255,0,255},{0,255,255},{128,0,0},{0,128,0},{0,0,128},{128,128,0},
    {128,0,128},{0,128,128},{192,192,192},{128,128,128},{255,128,0},
    {0,255,128},{128,0,255},{255,128,128},{128,255,128},{128,128,255},
    {255,165,0},{75,0,130},{238,130,238},{127,255,212},{244,164,96},
    {210,180,140},{255,20,147},{0,255,127},{72,209,204},{255,215,0}
};
static const int PAL_COLS = 16;
static const int PAL_ROWS = 2;
static const int PAL_SW = 18;

struct UndoState {
    std::vector<uint8_t> data;
    int w, h;
};

class SynPaint {
public:
    SDL_Window *win;
    SDL_Renderer *ren;
    SDL_Texture *canvas_tex;
    cairo_surface_t *cairo_surf;
    cairo_t *cr;

    int tool, brush_size;
    Color fg, bg;
    int mouse_x, mouse_y;
    int drawing, start_x, start_y;
    int last_x, last_y;

    std::vector<UndoState> undo_stack;
    static const int MAX_UNDO = 20;

    int show_color_picker;
    int cp_r, cp_g, cp_b;
    int cp_slider_drag;
    int cp_is_fg;

    int show_about;
    char status_msg[256];
    Uint32 status_timer;

    SynPaint() : win(nullptr), ren(nullptr), canvas_tex(nullptr),
                 cairo_surf(nullptr), cr(nullptr),
                 tool(TOOL_BRUSH), brush_size(3),
                 fg{0,0,0}, bg{255,255,255},
                 mouse_x(0), mouse_y(0),
                 drawing(0), start_x(0), start_y(0),
                 last_x(0), last_y(0),
                 show_color_picker(0), cp_r(128), cp_g(128), cp_b(128),
                 cp_slider_drag(0), cp_is_fg(1), show_about(0),
                 status_timer(0) {
        status_msg[0] = 0;
    }

    ~SynPaint() { cleanup(); }

    int init() {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            fprintf(stderr, "SDL error: %s\n", SDL_GetError());
            return -1;
        }
        win = SDL_CreateWindow("SynPaint - S3n Paint",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               WIN_W, WIN_H,
                               SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
        if (!win) { fprintf(stderr, "Window error\n"); return -1; }
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
        if (!ren) { fprintf(stderr, "Renderer error\n"); return -1; }
        SDL_ShowCursor(SDL_TRUE);

        cairo_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                CANVAS_W, CANVAS_H);
        cr = cairo_create(cairo_surf);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);

        canvas_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB32,
                                       SDL_TEXTUREACCESS_STREAMING,
                                       CANVAS_W, CANVAS_H);
        sync_canvas();
        set_status("Welcome to SynPaint!");
        return 0;
    }

    void cleanup() {
        if (cr) cairo_destroy(cr);
        if (cairo_surf) cairo_surface_destroy(cairo_surf);
        if (canvas_tex) SDL_DestroyTexture(canvas_tex);
        if (ren) SDL_DestroyRenderer(ren);
        if (win) SDL_DestroyWindow(win);
        SDL_Quit();
    }

    void sync_canvas() {
        void *pixels; int pitch;
        SDL_LockTexture(canvas_tex, nullptr, &pixels, &pitch);
        memcpy(pixels, cairo_image_surface_get_data(cairo_surf),
               CANVAS_H * pitch);
        SDL_UnlockTexture(canvas_tex);
    }

    void set_status(const char *fmt, ...) {
        va_list ap; va_start(ap, fmt);
        vsnprintf(status_msg, sizeof(status_msg), fmt, ap);
        va_end(ap);
        status_timer = SDL_GetTicks();
    }

    void save_undo() {
        UndoState u;
        u.w = CANVAS_W; u.h = CANVAS_H;
        u.data.resize(CANVAS_W * CANVAS_H * 4);
        memcpy(u.data.data(), cairo_image_surface_get_data(cairo_surf),
               CANVAS_W * CANVAS_H * 4);
        undo_stack.push_back(u);
        if ((int)undo_stack.size() > MAX_UNDO)
            undo_stack.erase(undo_stack.begin());
    }

    void undo() {
        if (undo_stack.empty()) return;
        UndoState &u = undo_stack.back();
        if (cr) cairo_destroy(cr);
        if (cairo_surf) cairo_surface_destroy(cairo_surf);
        cairo_surf = cairo_image_surface_create_for_data(
            u.data.data(), CAIRO_FORMAT_ARGB32, u.w, u.h, u.w * 4);
        cr = cairo_create(cairo_surf);
        sync_canvas();
        undo_stack.pop_back();
        set_status("Undo");
    }

    void clear_canvas() {
        save_undo();
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_paint(cr);
        sync_canvas();
        set_status("Canvas cleared");
    }

    void draw_brush_stroke(int x, int y) {
        cairo_set_source_rgb(cr, fg.r/255.0, fg.g/255.0, fg.b/255.0);
        cairo_set_line_width(cr, tool == TOOL_PEN ? 1 : brush_size);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_move_to(cr, last_x - CANVAS_X, last_y - CANVAS_Y);
        cairo_line_to(cr, x - CANVAS_X, y - CANVAS_Y);
        cairo_stroke(cr);
    }

    void draw_eraser_stroke(int x, int y) {
        cairo_set_source_rgb(cr, bg.r/255.0, bg.g/255.0, bg.b/255.0);
        cairo_set_line_width(cr, brush_size);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_move_to(cr, last_x - CANVAS_X, last_y - CANVAS_Y);
        cairo_line_to(cr, x - CANVAS_X, y - CANVAS_Y);
        cairo_stroke(cr);
    }

    void draw_line_shape(int ex, int ey) {
        cairo_set_source_rgb(cr, fg.r/255.0, fg.g/255.0, fg.b/255.0);
        cairo_set_line_width(cr, brush_size);
        cairo_move_to(cr, start_x - CANVAS_X, start_y - CANVAS_Y);
        cairo_line_to(cr, ex - CANVAS_X, ey - CANVAS_Y);
        cairo_stroke(cr);
    }

    void draw_rect_shape(int ex, int ey) {
        int x = start_x < ex ? start_x : ex;
        int y = start_y < ey ? start_y : ey;
        int w = abs(ex - start_x);
        int h = abs(ey - start_y);
        cairo_set_source_rgb(cr, fg.r/255.0, fg.g/255.0, fg.b/255.0);
        cairo_set_line_width(cr, brush_size);
        cairo_rectangle(cr, x - CANVAS_X, y - CANVAS_Y, w, h);
        cairo_stroke(cr);
    }

    void draw_circle_shape(int ex, int ey) {
        int dx = ex - start_x;
        int dy = ey - start_y;
        int r = (int)sqrt(dx*dx + dy*dy);
        cairo_set_source_rgb(cr, fg.r/255.0, fg.g/255.0, fg.b/255.0);
        cairo_set_line_width(cr, brush_size);
        cairo_arc(cr, start_x - CANVAS_X, start_y - CANVAS_Y, r, 0, 2*M_PI);
        cairo_stroke(cr);
    }

    void draw_fill(int x, int y) {
        int cx = x - CANVAS_X;
        int cy = y - CANVAS_Y;
        if (cx < 0 || cx >= CANVAS_W || cy < 0 || cy >= CANVAS_H) return;
        unsigned char *data = cairo_image_surface_get_data(cairo_surf);
        int stride = cairo_image_surface_get_stride(cairo_surf);
        uint8_t target[4];
        int idx = cy * stride + cx * 4;
        target[0] = data[idx]; target[1] = data[idx+1];
        target[2] = data[idx+2]; target[3] = data[idx+3];
        uint8_t fill[4] = {(uint8_t)(fg.b*255), (uint8_t)(fg.g*255),
                           (uint8_t)(fg.r*255), 255};
        if (memcmp(target, fill, 4) == 0) return;

        std::vector<SDL_Point> stack;
        stack.push_back({cx, cy});
        while (!stack.empty()) {
            SDL_Point p = stack.back(); stack.pop_back();
            if (p.x < 0 || p.x >= CANVAS_W || p.y < 0 || p.y >= CANVAS_H) continue;
            idx = p.y * stride + p.x * 4;
            if (memcmp(&data[idx], target, 4) != 0) continue;
            data[idx] = fill[0]; data[idx+1] = fill[1];
            data[idx+2] = fill[2]; data[idx+3] = fill[3];
            stack.push_back({p.x+1, p.y});
            stack.push_back({p.x-1, p.y});
            stack.push_back({p.x, p.y+1});
            stack.push_back({p.x, p.y-1});
        }
        cairo_surface_mark_dirty(cairo_surf);
    }

    void save_file_png() {
        char path[256]; time_t t = time(nullptr);
        struct tm *lt = localtime(&t);
        snprintf(path, sizeof(path), "synpaint_%04d%02d%02d_%02d%02d%02d.png",
                 lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday,
                 lt->tm_hour, lt->tm_min, lt->tm_sec);
        cairo_surface_write_to_png(cairo_surf, path);
        set_status("Saved: %s", path);
    }

    /* ─── Text rendering using Cairo ─── */
    SDL_Texture *render_text_tex(const char *s, int r, int g, int b) {
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

    void render_text(const char *s, int x, int y, int r, int g, int b) {
        SDL_Texture *tex = render_text_tex(s, r, g, b);
        if (!tex) return;
        int w, h; SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
        SDL_Rect dst = {x, y, w, h};
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }

    void render_text_rect(const char *s, SDL_Rect *rect, int r, int g, int b) {
        SDL_Texture *tex = render_text_tex(s, r, g, b);
        if (!tex) return;
        int tw, th; SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
        SDL_Rect dst = {rect->x + (rect->w - tw)/2, rect->y + (rect->h - th)/2, tw, th};
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }

    /* ─── Cursor ─── */
    void update_cursor() {
        if (mouse_x >= CANVAS_X && mouse_x < CANVAS_X + CANVAS_W &&
            mouse_y >= CANVAS_Y && mouse_y < CANVAS_Y + CANVAS_H) {
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR));
        } else {
            SDL_SetCursor(SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW));
        }
    }

    /* ─── Render UI ─── */
    void render() {
        SDL_SetRenderDrawColor(ren, 45, 45, 48, 255);
        SDL_RenderClear(ren);

        /* Canvas area */
        SDL_Rect canvas_rect = {CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H};
        SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
        SDL_RenderFillRect(ren, &canvas_rect);
        SDL_RenderCopy(ren, canvas_tex, nullptr, &canvas_rect);

        render_toolbar();
        render_palette();
        render_statusbar();

        if (show_color_picker) render_color_picker();
        if (show_about) render_about();

        SDL_RenderPresent(ren);
    }

    void render_toolbar() {
        int x = 5, y = 5;

        /* ─── Tool buttons ─── */
        for (int i = 0; i < TOOL_COUNT; i++) {
            SDL_Rect r = {x, y + i * 28, TOOL_W - 10, 24};
            if (i == tool) {
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

        /* ─── Brush size slider ─── */
        render_text("Size:", x + 4, by, 180, 180, 180);
        SDL_Rect slider_bg = {x + 4, by + 16, TOOL_W - 18, 10};
        SDL_SetRenderDrawColor(ren, 60, 60, 65, 255);
        SDL_RenderFillRect(ren, &slider_bg);
        float pct = (brush_size - 1) / 19.0f;
        SDL_Rect slider_fill = {slider_bg.x + 1, slider_bg.y + 1,
                                (int)((TOOL_W - 20) * pct), slider_bg.h - 2};
        SDL_SetRenderDrawColor(ren, 100, 150, 220, 255);
        SDL_RenderFillRect(ren, &slider_fill);

        /* Size value */
        char sz[8]; snprintf(sz, sizeof(sz), "%d", brush_size);
        render_text(sz, x + TOOL_W - 30, by, 200, 200, 200);

        /* ─── FG/BG color boxes ─── */
        int cy = by + 34;
        SDL_Rect fg_box = {x + 4, cy, 24, 24};
        SDL_SetRenderDrawColor(ren, fg.r, fg.g, fg.b, 255);
        SDL_RenderFillRect(ren, &fg_box);
        SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
        SDL_RenderDrawRect(ren, &fg_box);
        render_text("FG", x + 32, cy + 4, 200, 200, 200);
        /* Swap indicator */
        SDL_SetRenderDrawColor(ren, 150, 150, 150, 255);
        SDL_RenderDrawLine(ren, x + TOOL_W - 20, cy + 4, x + TOOL_W - 10, cy + 14);
        SDL_RenderDrawLine(ren, x + TOOL_W - 20, cy + 14, x + TOOL_W - 10, cy + 4);

        SDL_Rect bg_box = {x + 4, cy + 28, 24, 24};
        SDL_SetRenderDrawColor(ren, bg.r, bg.g, bg.b, 255);
        SDL_RenderFillRect(ren, &bg_box);
        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);
        SDL_RenderDrawRect(ren, &bg_box);
        render_text("BG", x + 32, cy + 32, 200, 200, 200);

        /* Clickable color indicator (opens picker) */
        char hex[10];
        snprintf(hex, sizeof(hex), "#%02X%02X%02X", fg.r, fg.g, fg.b);
        render_text(hex, x + 32, cy + 52, 180, 180, 150);

        /* ─── Action buttons ─── */
        int ay = cy + 74;
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

    void render_palette() {
        int start_x_pal = CANVAS_X;
        int start_y_pal = 1;
        for (int i = 0; i < (int)(sizeof(palette)/sizeof(palette[0])); i++) {
            int px = start_x_pal + (i % PAL_COLS) * (PAL_SW + 2);
            int py = start_y_pal + (i / PAL_COLS) * (PAL_SW + 2);
            SDL_Rect r = {px, py, PAL_SW, PAL_SW};
            SDL_SetRenderDrawColor(ren, palette[i].r, palette[i].g,
                                         palette[i].b, 255);
            SDL_RenderFillRect(ren, &r);
            /* Highlight if matches fg or bg */
            if (palette[i].r == fg.r && palette[i].g == fg.g && palette[i].b == fg.b) {
                SDL_SetRenderDrawColor(ren, 255, 255, 0, 255);
                SDL_RenderDrawRect(ren, &r);
            } else {
                SDL_SetRenderDrawColor(ren, 80, 80, 85, 255);
                SDL_RenderDrawRect(ren, &r);
            }
        }
    }

    void render_statusbar() {
        int y = WIN_H - 20;
        SDL_SetRenderDrawColor(ren, 30, 30, 32, 255);
        SDL_Rect bar = {0, y, WIN_W, 20};
        SDL_RenderFillRect(ren, &bar);

        char buf[128];
        snprintf(buf, sizeof(buf), " (%d,%d) | %s | Size: %d",
                 mouse_x - CANVAS_X, mouse_y - CANVAS_Y,
                 tool_names[tool], brush_size);
        render_text(buf, 4, y + 1, 150, 150, 150);

        if (status_msg[0] && SDL_GetTicks() - status_timer < 3000) {
            int slen = strlen(status_msg);
            render_text(status_msg, WIN_W - slen * 9 - 10, y + 1, 200, 200, 120);
        }
    }

    void render_color_picker() {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_Rect over = {0, 0, WIN_W, WIN_H};
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
        SDL_RenderFillRect(ren, &over);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

        int bx = WIN_W/2 - 150, by = WIN_H/2 - 100;
        SDL_Rect box = {bx, by, 300, 200};
        SDL_SetRenderDrawColor(ren, 45, 45, 50, 255);
        SDL_RenderFillRect(ren, &box);
        SDL_SetRenderDrawColor(ren, 80, 80, 85, 255);
        SDL_RenderDrawRect(ren, &box);

        render_text("Color Picker", bx + 80, by + 8, 220, 220, 220);

        const char *labels[] = {"R", "G", "B"};
        int *vals[] = {&cp_r, &cp_g, &cp_b};
        uint8_t colors[] = {255, 80, 80, 80, 255, 80, 80, 80, 255};
        for (int i = 0; i < 3; i++) {
            int ly = by + 32 + i * 30;
            render_text(labels[i], bx + 10, ly, colors[i*3], colors[i*3+1], colors[i*3+2]);
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

        /* Preview */
        SDL_Rect prev = {bx + 230, by + 100, 50, 50};
        SDL_SetRenderDrawColor(ren, cp_r, cp_g, cp_b, 255);
        SDL_RenderFillRect(ren, &prev);
        SDL_SetRenderDrawColor(ren, 150, 150, 150, 255);
        SDL_RenderDrawRect(ren, &prev);
        render_text("preview", bx + 228, by + 155, 150, 150, 150);

        /* OK button */
        SDL_Rect ok_btn = {bx + 40, by + 165, 80, 26};
        SDL_SetRenderDrawColor(ren, 50, 100, 50, 255);
        SDL_RenderFillRect(ren, &ok_btn);
        SDL_SetRenderDrawColor(ren, 80, 130, 80, 255);
        SDL_RenderDrawRect(ren, &ok_btn);
        render_text_rect("OK", &ok_btn, 220, 255, 220);

        /* Cancel button */
        SDL_Rect ca_btn = {bx + 170, by + 165, 80, 26};
        SDL_SetRenderDrawColor(ren, 100, 50, 50, 255);
        SDL_RenderFillRect(ren, &ca_btn);
        SDL_SetRenderDrawColor(ren, 130, 80, 80, 255);
        SDL_RenderDrawRect(ren, &ca_btn);
        render_text_rect("Cancel", &ca_btn, 255, 220, 220);
    }

    void render_about() {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_Rect over = {0, 0, WIN_W, WIN_H};
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 180);
        SDL_RenderFillRect(ren, &over);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

        int bx = WIN_W/2 - 200, by = WIN_H/2 - 80;
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

    /* ─── Input handling ─── */
    void handle_event(SDL_Event &ev) {
        if (show_color_picker) {
            handle_color_picker(ev);
            return;
        }
        if (show_about) {
            if (ev.type == SDL_MOUSEBUTTONDOWN)
                show_about = 0;
            return;
        }

        switch (ev.type) {
        case SDL_MOUSEBUTTONDOWN:
            handle_mouse_down(ev.button);
            break;
        case SDL_MOUSEBUTTONUP:
            handle_mouse_up(ev.button);
            break;
        case SDL_MOUSEMOTION:
            mouse_x = ev.motion.x;
            mouse_y = ev.motion.y;
            update_cursor();
            if (drawing) {
                handle_draw(ev.motion.x, ev.motion.y);
            }
            break;
        case SDL_KEYDOWN:
            handle_key(ev.key);
            break;
        }
    }

    int hit_rect(int mx, int my, const SDL_Rect *r) {
        return mx >= r->x && mx <= r->x + r->w &&
               my >= r->y && my <= r->y + r->h;
    }

    void handle_mouse_down(SDL_MouseButtonEvent &btn) {
        int x = btn.x, y = btn.y;

        /* ─── Toolbar ─── */
        if (x < TOOL_W) {
            int tool_h = TOOL_COUNT * 28;
            /* Tool buttons */
            int ti = (y - 5) / 28;
            if (ti >= 0 && ti < TOOL_COUNT && y >= 5 && y < 5 + tool_h) {
                tool = ti;
                set_status("Tool: %s", tool_names[tool]);
                return;
            }

            int by = 5 + tool_h + 6;

            /* Brush size slider */
            SDL_Rect sld = {5 + 4, by + 16, TOOL_W - 18, 10};
            if (hit_rect(x, y, &sld)) {
                float pct = (x - sld.x) / (float)sld.w;
                brush_size = (int)(pct * 19) + 1;
                if (brush_size < 1) brush_size = 1;
                if (brush_size > 20) brush_size = 20;
                set_status("Brush size: %d", brush_size);
                return;
            }

            int cy = by + 34;

            /* FG click opens picker */
            SDL_Rect fg_b = {5 + 4, cy, 24, 24};
            if (hit_rect(x, y, &fg_b)) {
                show_color_picker = 1; cp_is_fg = 1;
                cp_r = fg.r; cp_g = fg.g; cp_b = fg.b;
                return;
            }
            /* BG click opens picker */
            SDL_Rect bg_b = {5 + 4, cy + 28, 24, 24};
            if (hit_rect(x, y, &bg_b)) {
                show_color_picker = 1; cp_is_fg = 0;
                cp_r = bg.r; cp_g = bg.g; cp_b = bg.b;
                return;
            }
            /* Swap colors on the arrow */
            SDL_Rect swap_a = {5 + TOOL_W - 24, cy, 18, 18};
            if (hit_rect(x, y, &swap_a)) {
                Color tmp = fg; fg = bg; bg = tmp;
                set_status("Colors swapped");
                return;
            }

            int ay = cy + 74;
            SDL_Rect btns[] = {
                {5 + 4, ay + 0*26, TOOL_W - 18, 22},
                {5 + 4, ay + 1*26, TOOL_W - 18, 22},
                {5 + 4, ay + 2*26, TOOL_W - 18, 22},
                {5 + 4, ay + 3*26, TOOL_W - 18, 22},
            };
            if (hit_rect(x, y, &btns[0])) { clear_canvas(); return; }
            if (hit_rect(x, y, &btns[1])) { save_file_png(); return; }
            if (hit_rect(x, y, &btns[2])) { undo(); return; }
            if (hit_rect(x, y, &btns[3])) { show_about = 1; return; }
            return;
        }

        /* ─── Palette ─── */
        if (y < PAL_H + 2 && x >= CANVAS_X) {
            int col = (x - CANVAS_X) / (PAL_SW + 2);
            int row = y / (PAL_SW + 2);
            int idx = row * PAL_COLS + col;
            if (idx >= 0 && idx < (int)(sizeof(palette)/sizeof(palette[0]))) {
                if (btn.button == SDL_BUTTON_LEFT)
                    fg = palette[idx];
                else
                    bg = palette[idx];
                set_status("Color: #%02X%02X%02X", fg.r, fg.g, fg.b);
            }
            return;
        }

        /* ─── Canvas ─── */
        if (x >= CANVAS_X && x < CANVAS_X + CANVAS_W &&
            y >= CANVAS_Y && y < CANVAS_Y + CANVAS_H &&
            btn.button == SDL_BUTTON_LEFT) {
            if (tool == TOOL_FILL) {
                save_undo();
                draw_fill(x, y);
                sync_canvas();
                return;
            }
            save_undo();
            drawing = 1;
            start_x = last_x = x;
            start_y = last_y = y;
        }
    }

    void handle_mouse_up(SDL_MouseButtonEvent &btn) {
        if (!drawing) return;
        drawing = 0;
        if (tool == TOOL_LINE || tool == TOOL_RECT || tool == TOOL_CIRCLE) {
            /* Restore clean state then draw final shape */
            if (!undo_stack.empty()) {
                UndoState &u = undo_stack.back();
                memcpy(cairo_image_surface_get_data(cairo_surf),
                       u.data.data(), u.w * u.h * 4);
                cairo_surface_mark_dirty(cairo_surf);
            }
            if (tool == TOOL_LINE) draw_line_shape(mouse_x, mouse_y);
            else if (tool == TOOL_RECT) draw_rect_shape(mouse_x, mouse_y);
            else if (tool == TOOL_CIRCLE) draw_circle_shape(mouse_x, mouse_y);
            sync_canvas();
        }
    }

    void handle_draw(int x, int y) {
        if (tool == TOOL_PEN || tool == TOOL_BRUSH) draw_brush_stroke(x, y);
        else if (tool == TOOL_ERASER) draw_eraser_stroke(x, y);
        else if (tool == TOOL_LINE || tool == TOOL_RECT || tool == TOOL_CIRCLE) {
            if (!undo_stack.empty()) {
                UndoState &u = undo_stack.back();
                memcpy(cairo_image_surface_get_data(cairo_surf),
                       u.data.data(), u.w * u.h * 4);
                cairo_surface_mark_dirty(cairo_surf);
            }
            if (tool == TOOL_LINE) draw_line_shape(x, y);
            else if (tool == TOOL_RECT) draw_rect_shape(x, y);
            else if (tool == TOOL_CIRCLE) draw_circle_shape(x, y);
        }
        last_x = x; last_y = y;
        sync_canvas();
    }

    void handle_key(SDL_KeyboardEvent &key) {
        if (key.keysym.sym == SDLK_z && (SDL_GetModState() & KMOD_CTRL))
            undo();
        if (key.keysym.sym == SDLK_s && (SDL_GetModState() & KMOD_CTRL))
            save_file_png();
        if (key.keysym.sym == SDLK_n && (SDL_GetModState() & KMOD_CTRL))
            clear_canvas();
    }

    void handle_color_picker(SDL_Event &ev) {
        int bx = WIN_W/2 - 150, by = WIN_H/2 - 100;

        if (ev.type == SDL_MOUSEBUTTONDOWN) {
            int mx = ev.button.x, my = ev.button.y;

            /* Sliders */
            for (int i = 0; i < 3; i++) {
                int ly = by + 32 + i * 30;
                SDL_Rect tr = {bx + 35, ly, 180, 18};
                if (hit_rect(mx, my, &tr)) {
                    int *vals[] = {&cp_r, &cp_g, &cp_b};
                    *vals[i] = (int)((mx - tr.x) * 255.0 / tr.w);
                    if (*vals[i] < 0) *vals[i] = 0;
                    if (*vals[i] > 255) *vals[i] = 255;
                    cp_slider_drag = i + 1;
                    return;
                }
            }

            /* OK */
            SDL_Rect ok_btn = {bx + 40, by + 165, 80, 26};
            if (hit_rect(mx, my, &ok_btn)) {
                if (cp_is_fg)
                    fg = {(uint8_t)cp_r, (uint8_t)cp_g, (uint8_t)cp_b};
                else
                    bg = {(uint8_t)cp_r, (uint8_t)cp_g, (uint8_t)cp_b};
                show_color_picker = 0;
                set_status("Color set");
                return;
            }
            /* Cancel */
            SDL_Rect ca_btn = {bx + 170, by + 165, 80, 26};
            if (hit_rect(mx, my, &ca_btn)) {
                show_color_picker = 0;
                return;
            }
            /* Click outside box closes */
            SDL_Rect box = {bx, by, 300, 200};
            if (!hit_rect(mx, my, &box))
                show_color_picker = 0;
        }

        if (ev.type == SDL_MOUSEBUTTONUP)
            cp_slider_drag = 0;

        if (ev.type == SDL_MOUSEMOTION && cp_slider_drag > 0) {
            int mx = ev.motion.x;
            int i = cp_slider_drag - 1;
            int *vals[] = {&cp_r, &cp_g, &cp_b};
            *vals[i] = (int)((mx - (bx + 35)) * 255.0 / 180.0);
            if (*vals[i] < 0) *vals[i] = 0;
            if (*vals[i] > 255) *vals[i] = 255;
        }
    }

    void run() {
        SDL_Event ev;
        while (SDL_WaitEvent(&ev)) {
            if (ev.type == SDL_QUIT) break;
            handle_event(ev);
            render();
        }
    }
};

int main(int, char**) {
    SynPaint app;
    if (app.init() < 0) return 1;
    app.run();
    return 0;
}

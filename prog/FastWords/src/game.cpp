#include "game.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>

FastWords::FastWords() : win(nullptr), ren(nullptr), text_surf(nullptr), cr(nullptr),
    score(0), lives(3), streak(0), best_score(0), round(0), game_over(0), paused(0),
    word_index(0), time_left(0), time_total(0), last_tick(0),
    flash_correct(0), flash_wrong(0), flash_time(0), feedback_timer(0),
    words_correct(0), words_wrong(0), total_words(0) {
    feedback_msg[0] = 0;
    input.len = 0;
    input.text[0] = 0;
}

FastWords::~FastWords() { shutdown(); }

void FastWords::shutdown() {
    if (cr) cairo_destroy(cr);
    if (text_surf) cairo_surface_destroy(text_surf);
    if (ren) SDL_DestroyRenderer(ren);
    if (win) SDL_DestroyWindow(win);
    SDL_Quit();
}

int FastWords::init() {
    srand(time(nullptr));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return -1;
    }
    win = SDL_CreateWindow("FastWords - Type Fast!",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           WIN_W, WIN_H, SDL_WINDOW_SHOWN);
    if (!win) { fprintf(stderr, "Window failed\n"); return -1; }
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) { fprintf(stderr, "Renderer failed\n"); return -1; }

    text_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIN_W, WIN_H);
    cr = cairo_create(text_surf);
    SDL_ShowCursor(SDL_FALSE);

    best_score = 0;
    game_over = 0;
    lives = 3;
    score = 0;
    round = 0;
    last_tick = SDL_GetTicks();
    next_word();
    return 0;
}

void FastWords::next_word() {
    word_index = rand() % WORD_COUNT;
    current_word = get_word(word_index);
    time_left = current_word.time_sec;
    time_total = current_word.time_sec;
    input.len = 0;
    input.text[0] = 0;
    round++;
    total_words++;
}

void FastWords::reset_round() {
    if (lives <= 0) {
        game_over = 1;
        if (score > best_score) best_score = score;
        return;
    }
    next_word();
}

void FastWords::handle_event(SDL_Event &ev) {
    if (game_over) {
        if (ev.type == SDL_KEYDOWN) {
            if (ev.key.keysym.sym == SDLK_SPACE || ev.key.keysym.sym == SDLK_RETURN) {
                game_over = 0;
                lives = 3;
                score = 0;
                streak = 0;
                round = 0;
                words_correct = 0;
                words_wrong = 0;
                total_words = 0;
                last_tick = SDL_GetTicks();
                next_word();
            }
        }
        return;
    }

    if (ev.type == SDL_KEYDOWN) {
        if (ev.key.keysym.sym == SDLK_ESCAPE) {
            SDL_Event q; q.type = SDL_QUIT;
            SDL_PushEvent(&q);
            return;
        }
        if (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_KP_ENTER) {
            if (input.len > 0) {
                /* Check answer */
                if (strcmp(input.text, current_word.text) == 0) {
                    int bonus = (int)(time_left * 10) + current_word.len * 5;
                    score += bonus;
                    streak++;
                    words_correct++;
                    flash_correct = 1;
                    flash_time = SDL_GetTicks();
                    snprintf(feedback_msg, sizeof(feedback_msg),
                             "+%d  streak x%d", bonus, streak);
                    feedback_timer = SDL_GetTicks();
                } else {
                    lives--;
                    streak = 0;
                    words_wrong++;
                    flash_wrong = 1;
                    flash_time = SDL_GetTicks();
                    snprintf(feedback_msg, sizeof(feedback_msg),
                             "Wrong! It was \"%s\"", current_word.text);
                    feedback_timer = SDL_GetTicks();
                }
                reset_round();
            }
            return;
        }
        if (ev.key.keysym.sym == SDLK_BACKSPACE) {
            if (input.len > 0) {
                input.len--;
                input.text[input.len] = 0;
            }
            return;
        }
        /* Typeable characters */
        char c = (char)ev.key.keysym.sym;
        if (c >= 'a' && c <= 'z' && input.len < 60) {
            input.text[input.len++] = c;
            input.text[input.len] = 0;
        }
    }
}

void FastWords::update() {
    if (game_over) return;

    Uint32 now = SDL_GetTicks();
    float dt = (now - last_tick) / 1000.0f;
    last_tick = now;

    time_left -= dt;
    if (time_left <= 0) {
        lives--;
        streak = 0;
        words_wrong++;
        flash_wrong = 1;
        flash_time = SDL_GetTicks();
        snprintf(feedback_msg, sizeof(feedback_msg),
                 "Time! It was \"%s\"", current_word.text);
        feedback_timer = SDL_GetTicks();
        reset_round();
    }
}

void FastWords::render_center(const char *s, int y, float size, float r, float g, float b) {
    cairo_save(cr);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, size);
    cairo_text_extents_t te;
    cairo_text_extents(cr, s, &te);
    cairo_set_source_rgb(cr, r, g, b);
    cairo_move_to(cr, (WIN_W - te.width) / 2, y);
    cairo_show_text(cr, s);
    cairo_restore(cr);
}

void FastWords::render_center_wrap(const char *s, int y, float size, float r, float g, float b) {
    render_center(s, y, size, r, g, b);
}

void FastWords::render() {
    /* Clear cairo surface */
    cairo_set_source_rgb(cr, 0.08, 0.08, 0.12);
    cairo_paint(cr);

    if (game_over) {
        /* Game over screen */
        render_center("GAME OVER", 200, 56, 1.0, 0.2, 0.2);
        char buf[128];
        snprintf(buf, sizeof(buf), "Score: %d  |  Best: %d", score, best_score);
        render_center(buf, 270, 28, 0.8, 0.8, 0.8);
        snprintf(buf, sizeof(buf), "Words: %d correct, %d wrong",
                 words_correct, words_wrong);
        render_center(buf, 310, 22, 0.6, 0.6, 0.6);
        snprintf(buf, sizeof(buf), "Accuracy: %d%%",
                 total_words > 0 ? (words_correct * 100 / total_words) : 0);
        render_center(buf, 340, 20, 0.6, 0.6, 0.6);

        if (score == best_score && score > 0)
            render_center("NEW BEST SCORE!", 380, 26, 1.0, 0.8, 0.0);

        render_center("Press SPACE or ENTER to play again", 450, 18, 0.5, 0.5, 0.5);
        render_center("Press ESC to quit", 480, 16, 0.4, 0.4, 0.4);

        /* Copy to SDL texture */
        SDL_Texture *tex = SDL_CreateTextureFromSurface(ren,
            SDL_CreateRGBSurfaceFrom(cairo_image_surface_get_data(text_surf),
                WIN_W, WIN_H, 32, cairo_image_surface_get_stride(text_surf),
                0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000));
        if (tex) {
            SDL_RenderCopy(ren, tex, nullptr, nullptr);
            SDL_DestroyTexture(tex);
        }
        SDL_RenderPresent(ren);
        return;
    }

    /* ─── HUD ─── */
    char buf[128];
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 18);

    /* Score */
    snprintf(buf, sizeof(buf), "Score: %d", score);
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_move_to(cr, 20, 28);
    cairo_show_text(cr, buf);

    /* Lives */
    cairo_set_source_rgb(cr, 1.0, 0.3, 0.3);
    int lx = 700;
    for (int i = 0; i < 3; i++) {
        if (i < lives) cairo_set_source_rgb(cr, 1.0, 0.3, 0.3);
        else cairo_set_source_rgb(cr, 0.3, 0.1, 0.1);
        cairo_move_to(cr, lx + i * 28, 28);
        cairo_show_text(cr, "\xe2\x99\xa5"); /* heart */
    }

    /* Round */
    snprintf(buf, sizeof(buf), "Round %d", round);
    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    cairo_move_to(cr, 380, 28);
    cairo_show_text(cr, buf);

    /* Streak */
    if (streak > 1) {
        snprintf(buf, sizeof(buf), "Streak x%d", streak);
        cairo_set_source_rgb(cr, 1.0, 0.8, 0.0);
        cairo_move_to(cr, 500, 28);
        cairo_show_text(cr, buf);
    }

    /* ─── Timer bar ─── */
    float pct = std::max(0.0f, time_left / time_total);
    int bar_w = 600;
    int bar_h = 20;
    int bar_x = (WIN_W - bar_w) / 2;
    int bar_y = 60;

    cairo_set_source_rgb(cr, 0.2, 0.2, 0.25);
    cairo_rectangle(cr, bar_x, bar_y, bar_w, bar_h);
    cairo_fill(cr);

    float tr = 1.0f - pct; /* redder as time runs out */
    float tg = pct;
    cairo_set_source_rgb(cr, tr, tg, 0.2);
    cairo_rectangle(cr, bar_x, bar_y, (int)(bar_w * pct), bar_h);
    cairo_fill(cr);

    /* Time text */
    snprintf(buf, sizeof(buf), "%.1fs", std::max(0.0f, time_left));
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_set_font_size(cr, 14);
    cairo_text_extents_t te;
    cairo_text_extents(cr, buf, &te);
    cairo_move_to(cr, bar_x + bar_w + 15, bar_y + 16);
    cairo_show_text(cr, buf);

    /* ─── Word to type (big display) ─── */
    cairo_set_font_size(cr, 48);
    cairo_set_source_rgb(cr, 0.2, 0.6, 1.0);
    cairo_text_extents(cr, current_word.text, &te);
    int word_x = (WIN_W - (int)te.width) / 2;
    cairo_move_to(cr, word_x, 200);
    cairo_show_text(cr, current_word.text);

    /* Help hint for long words */
    char hint[24] = {0};
    int hl = current_word.len;
    snprintf(hint, sizeof(hint), "%d letters", hl);
    cairo_set_font_size(cr, 14);
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_text_extents(cr, hint, &te);
    cairo_move_to(cr, (WIN_W - te.width) / 2, 230);
    cairo_show_text(cr, hint);

    /* ─── Input display ─── */
    /* Input box */
    cairo_set_source_rgb(cr, 0.15, 0.15, 0.2);
    cairo_rectangle(cr, 200, 280, 400, 50);
    cairo_fill(cr);
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.4);
    cairo_rectangle(cr, 200, 280, 400, 50);
    cairo_set_line_width(cr, 2);
    cairo_stroke(cr);

    /* Cursor blink */
    int show_cursor = (SDL_GetTicks() / 500) % 2;

    /* Input text */
    cairo_set_font_size(cr, 32);
    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_move_to(cr, 220, 315);
    cairo_show_text(cr, input.text);

    if (show_cursor) {
        cairo_text_extents(cr, input.text, &te);
        int cx = 220 + (int)te.width + 4;
        if (cx < 590) {
            cairo_move_to(cr, cx, 290);
            cairo_line_to(cr, cx, 325);
            cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
            cairo_set_line_width(cr, 3);
            cairo_stroke(cr);
        }
    }

    /* ─── Feedback message ─── */
    if (SDL_GetTicks() - feedback_timer < 2000) {
        cairo_set_font_size(cr, 22);
        if (flash_correct)
            cairo_set_source_rgb(cr, 0.2, 0.9, 0.3);
        else if (flash_wrong)
            cairo_set_source_rgb(cr, 0.9, 0.2, 0.2);

        cairo_text_extents(cr, feedback_msg, &te);
        cairo_move_to(cr, (WIN_W - te.width) / 2, 380);
        cairo_show_text(cr, feedback_msg);
    }

    /* ─── Instructions ─── */
    cairo_set_font_size(cr, 14);
    cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
    const char *inst = "Type the word and press ENTER  |  ESC to quit";
    cairo_text_extents(cr, inst, &te);
    cairo_move_to(cr, (WIN_W - te.width) / 2, 450);
    cairo_show_text(cr, inst);

    /* ─── Stats ─── */
    snprintf(buf, sizeof(buf), "Accuracy: %d%%",
             total_words > 0 ? (words_correct * 100 / total_words) : 100);
    cairo_set_font_size(cr, 14);
    cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
    cairo_move_to(cr, 20, WIN_H - 20);
    cairo_show_text(cr, buf);

    /* ─── Sync to SDL ─── */
    SDL_Surface *sdl_surf = SDL_CreateRGBSurfaceFrom(
        cairo_image_surface_get_data(text_surf),
        WIN_W, WIN_H, 32, cairo_image_surface_get_stride(text_surf),
        0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    if (sdl_surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, sdl_surf);
        if (tex) {
            SDL_RenderCopy(ren, tex, nullptr, nullptr);
            SDL_DestroyTexture(tex);
        }
        SDL_FreeSurface(sdl_surf);
    }

    /* ─── Flash overlay ─── */
    if (flash_correct && SDL_GetTicks() - flash_time < 150) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 0, 255, 0, 40);
        SDL_Rect r = {0, 0, WIN_W, WIN_H};
        SDL_RenderFillRect(ren, &r);
    }
    if (flash_wrong && SDL_GetTicks() - flash_time < 200) {
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 255, 0, 0, 60);
        SDL_Rect r = {0, 0, WIN_W, WIN_H};
        SDL_RenderFillRect(ren, &r);
    }
    if (SDL_GetTicks() - flash_time > 300) {
        flash_correct = 0;
        flash_wrong = 0;
    }
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);

    SDL_RenderPresent(ren);
}

void FastWords::run() {
    SDL_Event ev;
    while (SDL_WaitEvent(&ev)) {
        if (ev.type == SDL_QUIT) break;
        handle_event(ev);
        update();
        render();
    }
}

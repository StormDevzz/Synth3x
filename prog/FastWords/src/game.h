#ifndef GAME_H
#define GAME_H

#include <SDL2/SDL.h>
#include <cairo/cairo.h>
#include "words.h"

struct InputBuffer {
    char text[64];
    int len;
};

class Sound {
public:
    Sound();
    ~Sound();
    int  init();
    void play_correct();
    void play_wrong();
    void play_gameover();
    void close();

private:
    SDL_AudioDeviceID dev;
    SDL_AudioSpec spec;
    void queue_tone(int freq, int duration_ms, float vol);
    void queue_silence(int ms);
};

class FastWords {
public:
    FastWords();
    ~FastWords();

    int  init();
    void run();

private:
    SDL_Window   *win;
    SDL_Renderer *ren;
    cairo_surface_t *text_surf;
    cairo_t *cr;
    Sound         sound;

    WordPicker    picker;

    int score;
    int lives;
    int streak;
    int best_score;
    int round;
    int game_over;
    int game_over_played;

    WordEntry current_word;
    InputBuffer input;
    float time_left;
    float time_total;
    Uint32 last_tick;

    int flash_correct;
    int flash_wrong;
    Uint32 flash_time;
    char feedback_msg[64];
    Uint32 feedback_timer;

    int words_correct;
    int words_wrong;
    int total_words;

    void next_word();
    void handle_event(SDL_Event &ev);
    void update();
    void render();
    void render_center(const char *s, int y, float size, float r, float g, float b);
    void shutdown();

    static const int WIN_W = 800;
    static const int WIN_H = 600;
};

#endif

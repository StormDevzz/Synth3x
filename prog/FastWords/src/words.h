#ifndef WORDS_H
#define WORDS_H

#include <cstring>
#include <cstdlib>
#include <algorithm>

#define WORD_COUNT 100

static const char *word_list[WORD_COUNT] = {
    "cat", "dog", "sun", "run", "big", "red", "hat", "cup", "box", "pen",
    "fish", "bird", "tree", "book", "hand", "door", "star", "ball", "bell", "duck",
    "house", "horse", "water", "chair", "table", "phone", "clock", "music", "bread", "fruit",
    "planet", "garden", "window", "bridge", "silver", "puzzle", "mirror", "candle", "rocket", "jungle",
    "adventure", "butterfly", "chocolate", "elephant", "happiness", "keyboard", "language", "mountain",
    "notebook", "rainbow", "sunshine", "treasure", "umbrella", "volcano", "whisper", "diamond",
    "atmosphere", "celebration", "discovery", "education", "friendship", "generation", "impossible",
    "laboratory", "mechanical", "naturally", "operation", "passenger", "beautiful", "dangerous",
    "experience", "challenge", "knowledge", "brilliant", "fantastic", "champion",
    "extraordinary", "unbelievable", "unforgettable", "revolution", "photography",
    "zero", "jazz", "kiwi", "moon", "fire", "snow", "lake", "wind",
    "swift", "pixel", "forge", "glow", "echo", "dawn",
    "cyber", "neon", "void", "core",
    "freedom",
};

struct WordEntry {
    const char *text;
    int len;
    float time_sec;
};

class WordPicker {
public:
    WordPicker() : pos(0) {
        for (int i = 0; i < WORD_COUNT; i++)
            order[i] = i;
    }

    WordEntry next() {
        if (pos >= WORD_COUNT) {
            /* Reshuffle when exhausted */
            for (int i = WORD_COUNT - 1; i > 0; i--) {
                int j = rand() % (i + 1);
                std::swap(order[i], order[j]);
            }
            pos = 0;
        }
        int idx = order[pos++];
        WordEntry we;
        we.text = word_list[idx];
        we.len = strlen(we.text);
        float t = (float)(we.len - 3) / 11.0f;
        we.time_sec = 4.0f + t * 6.0f;
        if (we.time_sec < 4.0f) we.time_sec = 4.0f;
        if (we.time_sec > 10.0f) we.time_sec = 10.0f;
        return we;
    }

private:
    int order[WORD_COUNT];
    int pos;
};

#endif

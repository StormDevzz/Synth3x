#ifndef WORDS_H
#define WORDS_H

static const char *word_list[] = {
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
};

static const int WORD_COUNT = sizeof(word_list) / sizeof(word_list[0]);
static const int MIN_WORD_LEN = 3;
static const int MAX_WORD_LEN = 14;

struct WordEntry {
    const char *text;
    int len;
    float time_sec;
};

inline WordEntry get_word(int index) {
    WordEntry we;
    we.text = word_list[index % WORD_COUNT];
    we.len = strlen(we.text);
    float t = we.len - MIN_WORD_LEN;
    float range = MAX_WORD_LEN - MIN_WORD_LEN;
    we.time_sec = 4.0f + (t / range) * 6.0f;
    if (we.time_sec < 4.0f) we.time_sec = 4.0f;
    if (we.time_sec > 10.0f) we.time_sec = 10.0f;
    return we;
}

#endif

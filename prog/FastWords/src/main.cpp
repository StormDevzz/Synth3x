#include "game.h"

int main(int, char**) {
    FastWords game;
    if (game.init() < 0)
        return 1;
    game.run();
    return 0;
}

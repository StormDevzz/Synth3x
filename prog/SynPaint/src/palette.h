#ifndef PALETTE_H
#define PALETTE_H

#include <SDL2/SDL.h>
#include <cstdint>

struct Color {
    uint8_t r, g, b;

    bool operator==(const Color &o) const {
        return r == o.r && g == o.g && b == o.b;
    }
    bool operator!=(const Color &o) const {
        return !(*this == o);
    }
};

static const Color palette[] = {
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
static const int PAL_H  = PAL_ROWS * (PAL_SW + 2) + 1;

#endif

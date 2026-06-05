/* SynPaint — S3n Paint
 * A simple drawing program for the Synth3x distribution.
 *
 * Dependencies: SDL2, Cairo
 * Build: make
 */

#include "app.h"

int main(int, char**) {
    SynPaint app;
    if (app.init() < 0)
        return 1;
    app.run();
    return 0;
}

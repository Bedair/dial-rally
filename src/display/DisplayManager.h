#pragma once
// Owns the single 240×240 frame buffer and draws each screen into it.
// Every frame: one draw*() call, then push() sends the whole buffer at once.
//
// M1 draws placeholder screens; the ring renderer (hazards, sprites) is M2.

#include <M5Dial.h>
#include "game/Player.h"

class DisplayManager {
public:
    // Allocates the canvas (16-bit, falls back to 8-bit) and logs free heap.
    bool begin();

    void drawMenu(int selected);
    void drawBest(uint32_t best);
    void drawReady(const Player& p, uint32_t score, int count);
    void drawPlaying(const Player& p, uint32_t score);
    void drawPaused(const Player& p, uint32_t score, int selected);
    void drawGameOver(uint32_t score, uint32_t best, bool newBest, int selected);

    void push();

private:
    void drawArena(const Player& p, uint32_t score);   // rim, altar, explorer
    void drawDimDisc(int radius);                      // panel behind overlay text
    void drawOptions(const char* const items[], int count, int selected, int y);

    M5Canvas canvas_{&M5Dial.Display};
};

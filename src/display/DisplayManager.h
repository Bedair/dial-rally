#pragma once
// Owns the single 240×240 frame buffer and draws each screen into it.
// Every frame: one draw*() call, then push() sends the whole buffer at once.
//
// Road renderer, back to front: grass verges, asphalt, kerbs and lane
// markings scrolling with the distance driven, obstacles, the player's car
// (with a shadow while jumping), score HUD, short-lived effects ("+10" coin
// popup, milestone banner). Art is built from primitives; pixel-art sprites
// follow in M6.
//
// Crash: white flash, explosion with debris and a decaying screen shake
// (the frame is shifted with canvas scroll), then the results screen.

#include <M5Dial.h>
#include "game/GameEngine.h"

class DisplayManager {
public:
    // Allocates the canvas (16-bit, falls back to 8-bit) and logs free heap.
    bool begin();

    void drawMenu(int selected);
    void drawBest(uint32_t best);
    void drawReady(const GameEngine& g, int count);
    void drawPlaying(const GameEngine& g, uint32_t nowMs);
    void drawPaused(const GameEngine& g, int selected);
    // sinceMs: time since the crash, 0..CRASH_ANIM_MS.
    void drawCrash(const GameEngine& g, uint32_t sinceMs);
    void drawGameOver(const GameEngine& g, uint32_t best, bool newBest, int selected,
                      uint32_t nowMs);

    // Start short visual effects (drawn by drawPlaying until they expire).
    void fxCoin(float carX, uint32_t nowMs);
    void fxMilestone(uint32_t nowMs);

    void push();

private:
    void drawArena(const GameEngine& g, bool showScore);
    void drawRoad(float scroll);
    void drawHazard(const Hazard& h);
    void drawBarrier(const Hazard& h);
    void drawOil(const Hazard& h);
    void drawCoin(const Hazard& h);
    // Top-down car centred on (x, y), nose up. Used for the player and traffic.
    void drawCar(int x, int y, uint16_t body, uint16_t stripe, bool faint);
    void drawHud(uint32_t score, bool highlight);
    void drawEffects(uint32_t nowMs);
    void drawFrame();                                   // background for menu screens
    void drawChequers(int y);                           // chequered flag strip
    void drawDimDisc(int radius);                       // panel behind overlay text
    void drawOptions(const char* const items[], int count, int selected, int y);

    // Effect timers (0 = not running).
    uint32_t coinFxMs_ = 0;
    float coinFxX_ = 0;
    uint32_t milestoneFxMs_ = 0;

    M5Canvas canvas_{&M5Dial.Display};
};

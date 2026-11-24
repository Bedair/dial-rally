#include "display/DisplayManager.h"
#include "config/GameConfig.h"
#include <math.h>

namespace {

// Temple palette (RGB565).
constexpr uint16_t COL_BG = 0x0000;          // black
constexpr uint16_t COL_RIM = 0x49E5;         // dark sandstone
constexpr uint16_t COL_RIM_TICK = 0x6AE8;    // lighter sandstone
constexpr uint16_t COL_ALTAR = 0x2945;       // dark stone
constexpr uint16_t COL_PANEL = 0x18C3;       // overlay panel
constexpr uint16_t COL_TEXT = 0xFFFF;        // white
constexpr uint16_t COL_DIM = 0x8410;         // grey
constexpr uint16_t COL_GOLD = 0xFEA0;        // gold
constexpr uint16_t COL_EXPLORER = 0xFC00;    // orange

constexpr int CX = cfg::CENTER_X;
constexpr int CY = cfg::CENTER_Y;

// Point on a circle around the screen centre for an angle in degrees.
void polar(float deg, float r, int& x, int& y) {
    const float rad = deg * (float)DEG_TO_RAD;
    x = CX + (int)lroundf(r * sinf(rad));
    y = CY - (int)lroundf(r * cosf(rad));
}

}  // namespace

bool DisplayManager::begin() {
    Serial.printf("[MQ] free heap before canvas: %u B\n", (unsigned)ESP.getFreeHeap());
    canvas_.setColorDepth(16);
    bool ok = canvas_.createSprite(cfg::SCREEN_W, cfg::SCREEN_H);
    if (!ok) {
        Serial.println("[MQ] 16-bit canvas failed, falling back to 8-bit");
        canvas_.setColorDepth(8);
        ok = canvas_.createSprite(cfg::SCREEN_W, cfg::SCREEN_H);
    }
    Serial.printf("[MQ] canvas %s %dx%d @ %d bit, free heap: %u B\n", ok ? "ok" : "FAILED",
                  (int)canvas_.width(), (int)canvas_.height(), (int)canvas_.getColorDepth(),
                  (unsigned)ESP.getFreeHeap());
    canvas_.setTextDatum(middle_center);
    return ok;
}

void DisplayManager::push() {
    canvas_.pushSprite(0, 0);
}

// ---------------------------------------------------------------------------
// Building blocks
// ---------------------------------------------------------------------------
void DisplayManager::drawArena(const Player& p, uint32_t score) {
    canvas_.fillScreen(COL_BG);

    // Rim track with a tick every 30°.
    const int rOut = cfg::RIM_RADIUS + cfg::RIM_WIDTH / 2;
    const int rIn = cfg::RIM_RADIUS - cfg::RIM_WIDTH / 2;
    canvas_.fillArc(CX, CY, rOut, rIn, 0, 360, COL_RIM);
    for (int deg = 0; deg < 360; deg += 30) {
        int x0, y0, x1, y1;
        polar(deg, rIn, x0, y0);
        polar(deg, rOut, x1, y1);
        canvas_.drawLine(x0, y0, x1, y1, COL_RIM_TICK);
    }

    // Altar with the score.
    canvas_.fillCircle(CX, CY, cfg::ALTAR_RADIUS, COL_ALTAR);
    canvas_.setFont(&fonts::Font4);
    canvas_.setTextColor(COL_TEXT);
    canvas_.drawNumber(score, CX, CY);

    // Explorer (placeholder dot until the sprite).
    int ex, ey;
    polar(p.drawAngle(), cfg::RIM_RADIUS, ex, ey);
    canvas_.fillCircle(ex, ey, cfg::EXPLORER_RADIUS, COL_EXPLORER);
}

void DisplayManager::drawDimDisc(int radius) {
    canvas_.fillCircle(CX, CY, radius, COL_PANEL);
    canvas_.drawCircle(CX, CY, radius, COL_GOLD);
}

// Vertical list; the selected item is gold with markers on both sides.
void DisplayManager::drawOptions(const char* const items[], int count, int selected, int y) {
    canvas_.setFont(&fonts::FreeSansBold9pt7b);
    for (int i = 0; i < count; i++) {
        const bool sel = i == selected;
        const int iy = y + i * 26;
        canvas_.setTextColor(sel ? COL_GOLD : COL_DIM);
        canvas_.drawString(items[i], CX, iy);
        if (sel) {
            const int half = canvas_.textWidth(items[i]) / 2 + 12;
            canvas_.fillTriangle(CX - half, iy - 5, CX - half, iy + 5, CX - half + 7, iy, COL_GOLD);
            canvas_.fillTriangle(CX + half, iy - 5, CX + half, iy + 5, CX + half - 7, iy, COL_GOLD);
        }
    }
}

// ---------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------
void DisplayManager::drawMenu(int selected) {
    canvas_.fillScreen(COL_BG);
    canvas_.fillArc(CX, CY, 119, 113, 0, 360, COL_RIM);

    canvas_.setFont(&fonts::FreeSansBold9pt7b);
    canvas_.setTextColor(COL_DIM);
    canvas_.drawString("THE", CX, 52);
    canvas_.setFont(&fonts::FreeSansBold12pt7b);
    canvas_.setTextColor(COL_GOLD);
    canvas_.drawString("LOST TEMPLE", CX, 76);

    static const char* const items[] = {"Play", "High score"};
    drawOptions(items, 2, selected, 128);

    canvas_.setFont(&fonts::Font2);
    canvas_.setTextColor(COL_DIM);
    canvas_.drawString("turn to choose, press", CX, 188);
}

void DisplayManager::drawBest(uint32_t best) {
    canvas_.fillScreen(COL_BG);
    canvas_.fillArc(CX, CY, 119, 113, 0, 360, COL_RIM);
    canvas_.setFont(&fonts::FreeSansBold12pt7b);
    canvas_.setTextColor(COL_GOLD);
    canvas_.drawString("HIGH SCORE", CX, 72);
    canvas_.setFont(&fonts::Font7);
    canvas_.setTextColor(COL_TEXT);
    canvas_.drawNumber(best, CX, 125);
    canvas_.setFont(&fonts::Font2);
    canvas_.setTextColor(COL_DIM);
    canvas_.drawString("press to go back", CX, 180);
}

void DisplayManager::drawReady(const Player& p, uint32_t score, int count) {
    drawArena(p, score);
    canvas_.fillCircle(CX, CY, cfg::ALTAR_RADIUS, COL_ALTAR);   // countdown replaces score
    canvas_.setFont(&fonts::Font7);
    canvas_.setTextColor(COL_GOLD);
    canvas_.drawNumber(count, CX, CY);
}

void DisplayManager::drawPlaying(const Player& p, uint32_t score) {
    drawArena(p, score);
}

void DisplayManager::drawPaused(const Player& p, uint32_t score, int selected) {
    drawArena(p, score);
    drawDimDisc(80);
    canvas_.setFont(&fonts::FreeSansBold12pt7b);
    canvas_.setTextColor(COL_TEXT);
    canvas_.drawString("PAUSED", CX, 82);
    static const char* const items[] = {"Resume", "End run"};
    drawOptions(items, 2, selected, 122);
}

void DisplayManager::drawGameOver(uint32_t score, uint32_t best, bool newBest, int selected) {
    canvas_.fillScreen(COL_BG);
    canvas_.fillArc(CX, CY, 119, 113, 0, 360, COL_RIM);

    canvas_.setFont(&fonts::FreeSansBold12pt7b);
    canvas_.setTextColor(COL_GOLD);
    canvas_.drawString("GAME OVER", CX, 50);

    canvas_.setFont(&fonts::Font7);
    canvas_.setTextColor(COL_TEXT);
    canvas_.drawNumber(score, CX, 98);

    canvas_.setFont(&fonts::Font2);
    canvas_.setTextColor(newBest ? COL_GOLD : COL_DIM);
    char line[32];
    if (newBest) snprintf(line, sizeof(line), "NEW HIGH SCORE!");
    else         snprintf(line, sizeof(line), "best %lu", (unsigned long)best);
    canvas_.drawString(line, CX, 136);

    static const char* const items[] = {"Play again", "Menu"};
    drawOptions(items, 2, selected, 166);
}

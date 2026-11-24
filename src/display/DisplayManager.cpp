#include "display/DisplayManager.h"
#include "config/GameConfig.h"
#include <math.h>

namespace {

// Palette (RGB565).
constexpr uint16_t COL_GRASS = 0x2B25;        // verge
constexpr uint16_t COL_GRASS_DARK = 0x22A4;   // mowing stripes
constexpr uint16_t COL_ASPHALT = 0x39E8;
constexpr uint16_t COL_LINE = 0xDEFB;         // lane markings
constexpr uint16_t COL_KERB_RED = 0xD943;
constexpr uint16_t COL_WHITE = 0xFFFF;
constexpr uint16_t COL_BLACK = 0x0000;
constexpr uint16_t COL_DIM = 0x8410;          // grey text
constexpr uint16_t COL_ACCENT = 0xFEA0;       // yellow (titles, selection)
constexpr uint16_t COL_PANEL = 0x18E3;        // overlay panel

constexpr uint16_t COL_BARRIER = 0xF800;      // roadblock red, white stripes
constexpr uint16_t COL_OIL = 0x10A3;          // near-black
constexpr uint16_t COL_OIL_SHEEN = 0x41ED;    // purple sheen
constexpr uint16_t COL_COIN = 0xFEA0;
constexpr uint16_t COL_COIN_EDGE = 0xC4A0;

constexpr uint16_t COL_PLAYER = 0xE8E4;       // red
constexpr uint16_t COL_WINDOW = 0x1948;       // dark blue glass
constexpr uint16_t COL_WHEEL = 0x0841;
constexpr uint16_t TRAFFIC_COLS[] = {0x2B7F, 0xFE60, 0x4648};   // blue, yellow, green

constexpr int CX = cfg::CENTER_X;
constexpr int CY = cfg::CENTER_Y;
constexpr int ROAD_W = cfg::ROAD_RIGHT - cfg::ROAD_LEFT;
constexpr float LANE_W = ROAD_W / (float)cfg::ROAD_LANES;
constexpr int INNER_LEFT = cfg::ROAD_LEFT + cfg::KERB_W;     // drivable asphalt
constexpr int INNER_RIGHT = cfg::ROAD_RIGHT - cfg::KERB_W;

// Half brightness: faint (telegraph) version of any colour.
inline uint16_t dim(uint16_t c) { return (c >> 1) & 0x7BEF; }

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
// Arena: road, obstacles, car, HUD
// ---------------------------------------------------------------------------
void DisplayManager::drawArena(const GameEngine& g, bool showScore) {
    drawRoad(g.scroll());

    const HazardField& hz = g.hazards();
    for (int i = 0; i < HazardField::MAX; i++) {
        if (hz[i].active) drawHazard(hz[i]);
    }

    drawCar((int)lroundf(g.player().drawX()), cfg::CAR_Y, COL_PLAYER, COL_WHITE, false);
    if (showScore) drawHud(g.score());
}

// Everything that scrolls uses `scroll` (px driven) modulo its own period,
// so the markings move down the screen at the road speed.
void DisplayManager::drawRoad(float scroll) {
    // Grass with mowing stripes.
    constexpr int GRASS_BAND = 24;
    canvas_.fillScreen(COL_GRASS);
    const int gOff = (int)fmodf(scroll, 2.0f * GRASS_BAND);
    for (int y = gOff - 2 * GRASS_BAND; y < cfg::SCREEN_H; y += 2 * GRASS_BAND) {
        canvas_.fillRect(0, y, cfg::SCREEN_W, GRASS_BAND, COL_GRASS_DARK);
    }

    // Asphalt.
    canvas_.fillRect(cfg::ROAD_LEFT, 0, ROAD_W, cfg::SCREEN_H, COL_ASPHALT);

    // Kerbs: alternating red / white blocks on both edges.
    const int seg = cfg::KERB_SEGMENT;
    const int kOff = (int)fmodf(scroll, 2.0f * seg);
    for (int y = kOff - 2 * seg; y < cfg::SCREEN_H; y += 2 * seg) {
        canvas_.fillRect(cfg::ROAD_LEFT, y, cfg::KERB_W, seg, COL_KERB_RED);
        canvas_.fillRect(cfg::ROAD_LEFT, y + seg, cfg::KERB_W, seg, COL_WHITE);
        canvas_.fillRect(INNER_RIGHT, y, cfg::KERB_W, seg, COL_KERB_RED);
        canvas_.fillRect(INNER_RIGHT, y + seg, cfg::KERB_W, seg, COL_WHITE);
    }

    // Dashed lane markings between lanes.
    const int period = cfg::DASH_LEN + cfg::DASH_GAP;
    const int dOff = (int)fmodf(scroll, (float)period);
    for (int lane = 1; lane < cfg::ROAD_LANES; lane++) {
        const int x = cfg::ROAD_LEFT + (int)lroundf(LANE_W * lane) - 1;
        for (int y = dOff - period; y < cfg::SCREEN_H; y += period) {
            canvas_.fillRect(x, y, 2, cfg::DASH_LEN, COL_LINE);
        }
    }
}

void DisplayManager::drawHazard(const Hazard& h) {
    switch (h.type) {
        case HazardType::Barrier: drawBarrier(h); break;
        case HazardType::Oil:     drawOil(h);     break;
        case HazardType::Coin:    drawCoin(h);    break;
        case HazardType::Traffic: {
            const uint16_t body = TRAFFIC_COLS[h.variant % 3];
            drawCar((int)lroundf(h.x), (int)lroundf(h.y), body, COL_WHITE, !h.armed);
            break;
        }
    }
}

// Roadblock: red/white striped blocks across the road, one gap left open.
void DisplayManager::drawBarrier(const Hazard& h) {
    const int top = (int)lroundf(h.y) - cfg::BARRIER_H / 2;
    const int gapL = (int)lroundf(h.x - h.w / 2);
    const int gapR = (int)lroundf(h.x + h.w / 2);
    const uint16_t red = h.armed ? COL_BARRIER : dim(COL_BARRIER);
    const uint16_t white = h.armed ? COL_WHITE : dim(COL_WHITE);

    struct Span { int l, r; };
    const Span spans[] = {{INNER_LEFT, gapL}, {gapR, INNER_RIGHT}};
    for (const Span& s : spans) {
        if (s.r <= s.l) continue;
        canvas_.fillRect(s.l, top, s.r - s.l, cfg::BARRIER_H, red);
        for (int x = s.l + 3; x < s.r - 2; x += 8) {                 // stripes
            canvas_.fillRect(x, top + 1, 3, cfg::BARRIER_H - 2, white);
        }
        canvas_.drawRect(s.l, top, s.r - s.l, cfg::BARRIER_H, COL_BLACK);
    }
}

// Oil spill across the whole road, with a purple sheen. Faint = outline only.
void DisplayManager::drawOil(const Hazard& h) {
    const int top = (int)lroundf(h.y) - cfg::OIL_H / 2;
    const int w = INNER_RIGHT - INNER_LEFT;
    if (!h.armed) {
        canvas_.drawRoundRect(INNER_LEFT, top, w, cfg::OIL_H, 8, COL_OIL);
        return;
    }
    canvas_.fillRoundRect(INNER_LEFT, top, w, cfg::OIL_H, 8, COL_OIL);
    const int cy = top + cfg::OIL_H / 2;
    canvas_.fillEllipse(INNER_LEFT + w / 4, cy - 2, 10, 2, COL_OIL_SHEEN);
    canvas_.fillEllipse(INNER_LEFT + (w * 2) / 3, cy + 3, 14, 2, COL_OIL_SHEEN);
}

void DisplayManager::drawCoin(const Hazard& h) {
    const int x = (int)lroundf(h.x), y = (int)lroundf(h.y);
    if (!h.armed) {
        canvas_.fillCircle(x, y, cfg::COIN_R, dim(COL_COIN));
        return;
    }
    canvas_.fillCircle(x, y, cfg::COIN_R, COL_COIN);
    canvas_.drawCircle(x, y, cfg::COIN_R, COL_COIN_EDGE);
    canvas_.drawCircle(x, y, cfg::COIN_R - 2, COL_COIN_EDGE);
    canvas_.drawPixel(x - 2, y - 2, COL_WHITE);   // glint
}

// Top-down car, nose up: four wheels, body, windscreen, rear window and a
// centre stripe. Sized by CAR_W × CAR_H.
void DisplayManager::drawCar(int x, int y, uint16_t body, uint16_t stripe, bool faint) {
    const int w = cfg::CAR_W, hgt = cfg::CAR_H;
    const int l = x - w / 2, t = y - hgt / 2;
    const uint16_t wheel = faint ? dim(COL_WHEEL) : COL_WHEEL;
    const uint16_t bodyC = faint ? dim(body) : body;
    const uint16_t glass = faint ? dim(COL_WINDOW) : COL_WINDOW;
    const uint16_t stripeC = faint ? dim(stripe) : stripe;

    // Wheels stick out 2 px on each side.
    canvas_.fillRect(l - 2, t + 4, 3, 7, wheel);
    canvas_.fillRect(l + w - 1, t + 4, 3, 7, wheel);
    canvas_.fillRect(l - 2, t + hgt - 11, 3, 7, wheel);
    canvas_.fillRect(l + w - 1, t + hgt - 11, 3, 7, wheel);

    canvas_.fillRoundRect(l, t, w, hgt, 5, bodyC);
    canvas_.fillRect(x - 1, t + 1, 3, hgt - 2, stripeC);              // racing stripe
    canvas_.fillRoundRect(l + 3, t + 7, w - 6, 6, 2, glass);          // windscreen
    canvas_.fillRect(l + 3, t + hgt - 9, w - 6, 4, glass);            // rear window
    if (!faint) {
        canvas_.drawPixel(l + 3, t + 1, COL_WHITE);                   // headlights
        canvas_.drawPixel(l + w - 4, t + 1, COL_WHITE);
    }
}

void DisplayManager::drawHud(uint32_t score) {
    canvas_.fillRoundRect(CX - 28, 12, 56, 24, 10, COL_BLACK);
    canvas_.setFont(&fonts::Font4);
    canvas_.setTextColor(COL_WHITE);
    canvas_.drawNumber(score, CX, 25);
}

void DisplayManager::drawFrame() {
    canvas_.fillScreen(COL_ASPHALT);
    // Static centre line, like standing on the grid.
    for (int y = 0; y < cfg::SCREEN_H; y += cfg::DASH_LEN + cfg::DASH_GAP) {
        canvas_.fillRect(CX - 1, y, 2, cfg::DASH_LEN, dim(COL_LINE));
    }
}

void DisplayManager::drawChequers(int y) {
    constexpr int SQ = 6;
    for (int row = 0; row < 2; row++) {
        for (int x = 54; x < 186; x += SQ) {
            const bool white = ((x / SQ) + row) % 2 == 0;
            canvas_.fillRect(x, y + row * SQ, SQ, SQ, white ? COL_WHITE : COL_BLACK);
        }
    }
}

void DisplayManager::drawDimDisc(int radius) {
    canvas_.fillCircle(CX, CY, radius, COL_PANEL);
    canvas_.drawCircle(CX, CY, radius, COL_ACCENT);
}

// Vertical list; the selected item is highlighted with markers on both sides.
void DisplayManager::drawOptions(const char* const items[], int count, int selected, int y) {
    canvas_.setFont(&fonts::FreeSansBold9pt7b);
    for (int i = 0; i < count; i++) {
        const bool sel = i == selected;
        const int iy = y + i * 26;
        if (sel) {
            const int half = canvas_.textWidth(items[i]) / 2 + 14;
            canvas_.fillRoundRect(CX - half, iy - 11, 2 * half, 22, 8, COL_BLACK);
            canvas_.fillTriangle(CX - half + 4, iy - 5, CX - half + 4, iy + 5, CX - half + 10, iy,
                                 COL_ACCENT);
            canvas_.fillTriangle(CX + half - 4, iy - 5, CX + half - 4, iy + 5, CX + half - 10, iy,
                                 COL_ACCENT);
        }
        canvas_.setTextColor(sel ? COL_ACCENT : COL_DIM);
        canvas_.drawString(items[i], CX, iy);
    }
}

// ---------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------
void DisplayManager::drawMenu(int selected) {
    drawFrame();
    drawChequers(34);
    canvas_.fillRoundRect(40, 54, 160, 36, 10, COL_BLACK);
    canvas_.setFont(&fonts::FreeSansBold12pt7b);
    canvas_.setTextColor(COL_ACCENT);
    canvas_.drawString("DIAL RALLY", CX, 72);

    static const char* const items[] = {"Play", "High score"};
    drawOptions(items, 2, selected, 128);

    canvas_.setFont(&fonts::Font2);
    canvas_.setTextColor(COL_WHITE);
    canvas_.drawString("turn to choose, press", CX, 190);
}

void DisplayManager::drawBest(uint32_t best) {
    drawFrame();
    drawChequers(40);
    canvas_.fillRoundRect(50, 60, 140, 100, 12, COL_BLACK);
    canvas_.setFont(&fonts::FreeSansBold12pt7b);
    canvas_.setTextColor(COL_ACCENT);
    canvas_.drawString("HIGH SCORE", CX, 80);
    canvas_.setFont(&fonts::Font7);
    canvas_.setTextColor(COL_WHITE);
    canvas_.drawNumber(best, CX, 126);
    canvas_.setFont(&fonts::Font2);
    canvas_.setTextColor(COL_WHITE);
    canvas_.drawString("press to go back", CX, 184);
}

void DisplayManager::drawReady(const GameEngine& g, int count) {
    drawArena(g, false);
    canvas_.fillCircle(CX, CY - 10, 34, COL_BLACK);
    canvas_.drawCircle(CX, CY - 10, 34, COL_ACCENT);
    canvas_.setFont(&fonts::Font7);
    canvas_.setTextColor(COL_ACCENT);
    canvas_.drawNumber(count, CX, CY - 10);
}

void DisplayManager::drawPlaying(const GameEngine& g) {
    drawArena(g, true);
}

void DisplayManager::drawPaused(const GameEngine& g, int selected) {
    drawArena(g, true);
    drawDimDisc(80);
    canvas_.setFont(&fonts::FreeSansBold12pt7b);
    canvas_.setTextColor(COL_WHITE);
    canvas_.drawString("PAUSED", CX, 82);
    static const char* const items[] = {"Resume", "End run"};
    drawOptions(items, 2, selected, 122);
}

void DisplayManager::drawGameOver(uint32_t score, uint32_t best, bool newBest, int selected) {
    drawFrame();
    canvas_.fillRoundRect(36, 30, 168, 122, 14, COL_BLACK);
    canvas_.setFont(&fonts::FreeSansBold12pt7b);
    canvas_.setTextColor(COL_ACCENT);
    canvas_.drawString("GAME OVER", CX, 50);

    canvas_.setFont(&fonts::Font7);
    canvas_.setTextColor(COL_WHITE);
    canvas_.drawNumber(score, CX, 98);

    canvas_.setFont(&fonts::Font2);
    canvas_.setTextColor(newBest ? COL_ACCENT : COL_DIM);
    char line[32];
    if (newBest) snprintf(line, sizeof(line), "NEW HIGH SCORE!");
    else         snprintf(line, sizeof(line), "best %lu", (unsigned long)best);
    canvas_.drawString(line, CX, 136);

    static const char* const items[] = {"Play again", "Menu"};
    drawOptions(items, 2, selected, 172);
}

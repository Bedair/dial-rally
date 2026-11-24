#include "display/DisplayManager.h"
#include "display/sprites/Sprites.h"
#include "config/GameConfig.h"
#include <math.h>

namespace {

// Palette (RGB565) for everything drawn with primitives.
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

constexpr uint16_t COL_BARRIER = 0xF800;      // crash fireball red
constexpr uint16_t COL_COIN = 0xFEA0;         // "+10" popup
constexpr uint16_t COL_PLAYER = 0xE8E4;       // debris red
constexpr uint16_t COL_SHADOW = 0x18C3;       // shadow under a jumping car
constexpr uint16_t COL_FIRE = 0xFD20;         // explosion orange
constexpr uint16_t COL_SMOKE = 0x52AA;        // grey smoke
constexpr uint16_t COL_CAUSE = 0xFB2C;        // light red: what ended the run

constexpr int CX = cfg::CENTER_X;
constexpr int CY = cfg::CENTER_Y;
constexpr int ROAD_W = cfg::ROAD_RIGHT - cfg::ROAD_LEFT;
constexpr float LANE_W = ROAD_W / (float)cfg::ROAD_LANES;
constexpr int INNER_LEFT = cfg::ROAD_LEFT + cfg::KERB_W;     // drivable asphalt
constexpr int INNER_RIGHT = cfg::ROAD_RIGHT - cfg::KERB_W;

const Sprite* const TRAFFIC[] = {&spr::CAR_BLUE, &spr::CAR_YELLOW, &spr::CAR_GREEN};
const Sprite* const COIN_FRAMES[] = {&spr::COIN_0, &spr::COIN_1, &spr::COIN_2, &spr::COIN_3};

// Roadside trees and bushes. One slot every VERGE_SPACING px of road; the
// pattern repeats every 9 x 80 = 720 px, the scroll wrap, so it never jumps.
// x is the distance from the road edge (+ right verge, - left verge).
constexpr int VERGE_SPACING = 80;
constexpr int VERGE_SLOTS = 9;
struct VergeItem { int8_t x; bool tree; };
constexpr VergeItem VERGE[VERGE_SLOTS] = {
    {18, true}, {-16, false}, {10, false}, {-22, true}, {24, false},
    {-12, true}, {14, true}, {-20, false}, {20, false},
};

// Half brightness: faint version of any colour.
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
    canvas_.setSwapBytes(true);   // sprite data is native-order RGB565 (see Sprite.h)
    return ok;
}

void DisplayManager::push() {
    canvas_.pushSprite(0, 0);
    frame_++;
}

// Sprite centred on (cx, cy); faint = the half-brightness copy.
void DisplayManager::drawSprite(const Sprite& s, int cx, int cy, bool faint) {
    canvas_.pushImage(cx - s.w / 2, cy - s.h / 2, s.w, s.h, faint ? s.dim : s.px, SPRITE_KEY);
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

    // Player car. While jumping it rises and leaves a shadow on the road.
    const Player& p = g.player();
    const int px = (int)lroundf(p.drawX());
    const float lift = p.jumpHeight() * cfg::JUMP_LIFT_PX;
    if (lift > 0.5f) {
        canvas_.fillRoundRect(px - cfg::CAR_W / 2 + 2, cfg::CAR_Y - cfg::CAR_H / 2 + 3,
                              cfg::CAR_W, cfg::CAR_H, 5, COL_SHADOW);
    }
    drawSprite(spr::CAR_PLAYER, px, cfg::CAR_Y - (int)lroundf(lift), false);
    if (showScore) drawHud(g.score(), milestoneFxMs_ != 0);
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

    // Trees and bushes on the verges.
    constexpr int VERGE_WRAP = VERGE_SPACING * VERGE_SLOTS;
    const int vOff = (int)fmodf(scroll, (float)VERGE_WRAP);
    for (int i = 0; i < VERGE_SLOTS; i++) {
        int y = (vOff + i * VERGE_SPACING) % VERGE_WRAP - 20;    // -20 .. 699
        if (y > cfg::SCREEN_H + 20) continue;
        const VergeItem& v = VERGE[i];
        const Sprite& s = v.tree ? spr::TREE : spr::BUSH;
        const int x = v.x > 0 ? cfg::ROAD_RIGHT + v.x + s.w / 2 : cfg::ROAD_LEFT + v.x - s.w / 2;
        drawSprite(s, x, y, false);
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
        case HazardType::Traffic:
            drawSprite(*TRAFFIC[h.variant % 3], (int)lroundf(h.x), (int)lroundf(h.y), !h.armed);
            break;
    }
}

// Roadblock: striped barrier tiles across the road, one gap left open. Each
// side is tiled from the road edge and clipped where the gap starts.
void DisplayManager::drawBarrier(const Hazard& h) {
    const Sprite& t = spr::BARRIER;
    const int top = (int)lroundf(h.y) - t.h / 2;
    const int gapL = (int)lroundf(h.x - h.w / 2);
    const int gapR = (int)lroundf(h.x + h.w / 2);
    const uint16_t* px = h.armed ? t.px : t.dim;

    struct Span { int l, r; };
    const Span spans[] = {{INNER_LEFT, gapL}, {gapR, INNER_RIGHT}};
    for (const Span& s : spans) {
        if (s.r <= s.l) continue;
        canvas_.setClipRect(s.l, top, s.r - s.l, t.h);
        for (int x = s.l; x < s.r; x += t.w) canvas_.pushImage(x, top, t.w, t.h, px, SPRITE_KEY);
        canvas_.clearClipRect();
    }
}

// Oil spill across the whole road: blob tiles, alternating two shapes.
void DisplayManager::drawOil(const Hazard& h) {
    const int y = (int)lroundf(h.y);
    const int tileW = spr::OIL_A.w;
    int i = 0;
    for (int x = INNER_LEFT + tileW / 2; x - tileW / 2 < INNER_RIGHT; x += tileW, i++) {
        drawSprite(i % 2 ? spr::OIL_B : spr::OIL_A, x, y, !h.armed);
    }
}

// Spinning coin: 4 frames, each coin's phase offset by its x position.
void DisplayManager::drawCoin(const Hazard& h) {
    const int frame = (int)((frame_ / 5 + (uint32_t)h.x) % 4);
    drawSprite(*COIN_FRAMES[frame], (int)lroundf(h.x), (int)lroundf(h.y), !h.armed);
}

// Score box at the top; highlighted (yellow frame + text) during a milestone.
void DisplayManager::drawHud(uint32_t score, bool highlight) {
    canvas_.fillRoundRect(CX - 28, 12, 56, 24, 10, COL_BLACK);
    if (highlight) canvas_.drawRoundRect(CX - 28, 12, 56, 24, 10, COL_ACCENT);
    canvas_.setFont(&fonts::Font4);
    canvas_.setTextColor(highlight ? COL_ACCENT : COL_WHITE);
    canvas_.drawNumber(score, CX, 25);
}

// ---------------------------------------------------------------------------
// Effects
// ---------------------------------------------------------------------------
void DisplayManager::fxCoin(float carX, uint32_t nowMs) {
    coinFxMs_ = nowMs ? nowMs : 1;
    coinFxX_ = carX;
}

void DisplayManager::fxMilestone(uint32_t nowMs) {
    milestoneFxMs_ = nowMs ? nowMs : 1;
}

void DisplayManager::drawEffects(uint32_t nowMs) {
    // "+10" rising from the car, fading out.
    if (coinFxMs_) {
        const uint32_t t = nowMs - coinFxMs_;
        if (t >= cfg::COIN_POPUP_MS) {
            coinFxMs_ = 0;
        } else {
            const float k = (float)t / cfg::COIN_POPUP_MS;
            const int y = cfg::CAR_Y - cfg::CAR_H / 2 - 8 - (int)(22 * k);
            canvas_.setFont(&fonts::Font2);
            canvas_.setTextColor(k < 0.6f ? COL_COIN : dim(COL_COIN));
            char txt[8];
            snprintf(txt, sizeof(txt), "+%lu", (unsigned long)cfg::POINTS_PER_COIN);
            canvas_.drawString(txt, (int)coinFxX_, y);
        }
    }
    // Milestone banner under the score, blinking at first.
    if (milestoneFxMs_) {
        const uint32_t t = nowMs - milestoneFxMs_;
        if (t >= cfg::MILESTONE_BANNER_MS) {
            milestoneFxMs_ = 0;
        } else if (t > 400 || (t / 100) % 2 == 0) {
            canvas_.fillRoundRect(CX - 50, 42, 100, 22, 8, COL_ACCENT);
            canvas_.setFont(&fonts::FreeSansBold9pt7b);
            canvas_.setTextColor(COL_BLACK);
            char txt[16];
            snprintf(txt, sizeof(txt), "+%lu BONUS", (unsigned long)cfg::MILESTONE_BONUS);
            canvas_.drawString(txt, CX, 53);
        }
    }
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

void DisplayManager::drawPlaying(const GameEngine& g, uint32_t nowMs) {
    drawArena(g, true);
    drawEffects(nowMs);
}

// The road freezes; a white flash, then an explosion with flying debris at
// the car, "CRASH!", and a screen shake that decays to zero.
void DisplayManager::drawCrash(const GameEngine& g, uint32_t sinceMs) {
    if (sinceMs < cfg::CRASH_FLASH_MS) {
        canvas_.fillScreen(COL_WHITE);
        return;
    }
    coinFxMs_ = milestoneFxMs_ = 0;
    drawArena(g, true);

    const float k = (float)sinceMs / cfg::CRASH_ANIM_MS;          // 0..1
    const int x = (int)lroundf(g.player().drawX()), y = cfg::CAR_Y;

    // Debris: 10 fragments flying outward on fixed directions.
    for (int i = 0; i < 10; i++) {
        const float a = i * 0.628f + 0.3f;                        // ~36° apart
        const float d = 10 + 70 * k * (0.6f + 0.4f * ((i * 7) % 5) / 4.0f);
        const int fx = x + (int)(cosf(a) * d), fy = y + (int)(sinf(a) * d * 0.8f);
        canvas_.fillRect(fx - 1, fy - 1, 3, 3, i % 2 ? COL_PLAYER : COL_DIM);
    }
    // Fireball, then smoke.
    if (k < 0.45f) {
        const int r = 8 + (int)(40 * k);
        canvas_.fillCircle(x, y, r, COL_BARRIER);
        canvas_.fillCircle(x, y, r * 2 / 3, COL_FIRE);
        canvas_.fillCircle(x, y, r / 3, COL_WHITE);
    } else {
        const int r = 26 + (int)(16 * k);
        canvas_.fillCircle(x, y, r, COL_SMOKE);
        canvas_.drawCircle(x, y, r, dim(COL_SMOKE));
    }
    if (sinceMs > 250) {
        canvas_.fillRoundRect(CX - 56, 86, 112, 34, 10, COL_BLACK);
        canvas_.setFont(&fonts::FreeSansBold12pt7b);
        canvas_.setTextColor(COL_BARRIER);
        canvas_.drawString("CRASH!", CX, 103);
    }

    // Shake: shift the finished frame; scroll fills the uncovered edge black.
    if (sinceMs < cfg::SHAKE_MS) {
        const float amp = cfg::SHAKE_PX * (1.0f - (float)sinceMs / cfg::SHAKE_MS);
        const uint32_t step = sinceMs / 16;                       // changes every frame
        const int dx = (step % 2 ? 1 : -1) * (int)lroundf(amp);
        const int dy = (int)((step * 7) % 3) - 1;
        canvas_.scroll(dx, dy * (int)lroundf(amp * 0.6f));
    }
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

// Results: what ended the run, the score, best / new high score (blinking),
// and the run's stats, then Play again / Menu.
void DisplayManager::drawGameOver(const GameEngine& g, uint32_t best, bool newBest, int selected,
                                  uint32_t nowMs) {
    drawFrame();
    drawChequers(14);
    canvas_.fillRoundRect(30, 28, 180, 134, 14, COL_BLACK);

    canvas_.setFont(&fonts::FreeSansBold9pt7b);
    canvas_.setTextColor(COL_ACCENT);
    canvas_.drawString("GAME OVER", CX, 42);

    const char* cause = g.crashCause();
    const char* why = !cause                       ? "run ended"
                    : cause[0] == 'r'              ? "hit a roadblock"
                    : cause[0] == 'o'              ? "slipped on oil"
                                                   : "crashed into traffic";
    canvas_.setFont(&fonts::Font2);
    canvas_.setTextColor(COL_CAUSE);
    canvas_.drawString(why, CX, 60);

    canvas_.setFont(&fonts::Font7);
    canvas_.setTextColor(COL_WHITE);
    canvas_.drawNumber(g.score(), CX, 98);

    canvas_.setFont(&fonts::Font2);
    char line[40];
    if (newBest) {
        const bool on = (nowMs / 300) % 2 == 0;
        canvas_.setTextColor(on ? COL_ACCENT : COL_WHITE);
        canvas_.drawString("NEW HIGH SCORE!", CX, 134);
    } else {
        canvas_.setTextColor(COL_DIM);
        snprintf(line, sizeof(line), "best %lu", (unsigned long)best);
        canvas_.drawString(line, CX, 134);
    }
    canvas_.setTextColor(COL_DIM);
    snprintf(line, sizeof(line), "%lu passed  %lu coins  %lus", (unsigned long)g.passed(),
             (unsigned long)g.coins(), (unsigned long)(g.timeMs() / 1000));
    canvas_.drawString(line, CX, 151);

    static const char* const items[] = {"Play again", "Menu"};
    drawOptions(items, 2, selected, 182);
}

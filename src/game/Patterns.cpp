#include "game/Patterns.h"
#include "game/Difficulty.h"
#include "config/GameConfig.h"
#include <Arduino.h>
#include <math.h>

namespace {

using Span = PatternSpawner::Span;

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------
constexpr float LANE_W = (cfg::ROAD_RIGHT - cfg::ROAD_LEFT) / (float)cfg::ROAD_LANES;
constexpr float INNER_L = cfg::ROAD_LEFT + cfg::KERB_W;
constexpr float INNER_R = cfg::ROAD_RIGHT - cfg::KERB_W;
constexpr float CAR_HALF = cfg::CAR_W / 2.0f - cfg::HITBOX_MARGIN;      // car hit box
constexpr float MIN_X = INNER_L + cfg::CAR_W / 2.0f;                    // car centre limits
constexpr float MAX_X = INNER_R - cfg::CAR_W / 2.0f;
// The car clears a traffic car when |x - trafficX| >= TRAFFIC_CLEAR.
constexpr float TRAFFIC_CLEAR = cfg::TRAFFIC_W / 2.0f - cfg::HITBOX_MARGIN + CAR_HALF;

constexpr float REACH_PX_PER_MS = cfg::FAIR_STEPS_PER_S * cfg::KNOB_PX_PER_STEP / 1000.0f;
constexpr uint32_t COIN_SPACING_MS = 160;

inline float laneX(int lane) { return cfg::ROAD_LEFT + LANE_W * (lane + 0.5f); }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Gap centre limits and slack for a roadblock gap of width w: the car fits
// through when |x - gapCentre| <= slack.
inline float gateMin(float w) { return INNER_L + w / 2; }
inline float gateMax(float w) { return INNER_R - w / 2; }
inline float gateSlack(float w) { return w / 2 - CAR_HALF; }

// ---------------------------------------------------------------------------
// Timing: how long things take at the speed of the moment
// ---------------------------------------------------------------------------
float heightOf(HazardType t) {
    switch (t) {
        case HazardType::Barrier: return cfg::BARRIER_H;
        case HazardType::Oil:     return cfg::OIL_H;
        case HazardType::Traffic: return cfg::TRAFFIC_H;
        case HazardType::Coin:    return 2 * cfg::COIN_R;
    }
    return 0;
}

// From the spawn line to the car's line at the speed of time tMs, ms. The
// speed changes only slightly within one travel time, so this is close enough
// for planning; spawning itself uses the exact position (obstacleY).
uint32_t travelMs(HazardType t, uint32_t tMs) {
    return (uint32_t)((cfg::CAR_Y - cfg::OBSTACLE_SPAWN_Y) * 1000.0f /
                      difficulty::approachSpeed(t, tMs));
}

// Half of the time the obstacle's hit box overlaps the car's vertically, ms.
uint32_t halfWindowMs(HazardType t, uint32_t tMs) {
    const float overlapPx = (cfg::CAR_H + heightOf(t)) / 2.0f - 2 * cfg::HITBOX_MARGIN;
    return (uint32_t)(overlapPx * 1000.0f / difficulty::approachSpeed(t, tMs));
}

// Plan this far ahead: the slowest obstacle (traffic at the start speed) must
// be planned before it has to appear at the top.
const uint32_t LOOKAHEAD_MS = travelMs(HazardType::Traffic, 0) + 800;

// Free x region for the car centre when traffic cars sit at the given lanes.
int freeSpans(const int* lanes, int n, Span out[3]) {
    Span cur[3] = {{MIN_X, MAX_X}};
    int count = 1;
    for (int i = 0; i < n; i++) {
        const float bl = laneX(lanes[i]) - TRAFFIC_CLEAR, br = laneX(lanes[i]) + TRAFFIC_CLEAR;
        Span next[3];
        int m = 0;
        for (int k = 0; k < count; k++) {
            if (cur[k].l < bl && m < 3) next[m++] = {cur[k].l, fminf(cur[k].r, bl)};
            if (cur[k].r > br && m < 3) next[m++] = {fmaxf(cur[k].l, br), cur[k].r};
        }
        count = m;
        for (int k = 0; k < m; k++) cur[k] = next[k];
    }
    for (int k = 0; k < count; k++) out[k] = cur[k];
    return count;
}

float distTo(float p, const Span* d, int n) {
    float best = 1e9f;
    for (int i = 0; i < n; i++) {
        const float dd = p < d[i].l ? d[i].l - p : (p > d[i].r ? p - d[i].r : 0.0f);
        if (dd < best) best = dd;
    }
    return best;
}

// ---------------------------------------------------------------------------
// Pattern table
// ---------------------------------------------------------------------------
enum class Kind : uint8_t { Gate, Car, Wall, Oil, Coins };
constexpr int8_t ANY = -1;    // spawner picks (random, within reach)
constexpr int8_t SAME = -2;   // same gap position as the previous gate

struct Step {
    Kind kind;
    int8_t lane;        // Gate: gap lane; Car: traffic lane; Wall: the FREE lane; Coins: lane
    uint16_t afterMs;   // gap after the previous step's window ends (× spacingScale)
};
struct Pattern {
    const char* name;
    const Step* steps;
    uint8_t count;
};

constexpr Step P_GATE[] = {{Kind::Gate, ANY, 0}};
constexpr Step P_SLALOM[] = {{Kind::Gate, 0, 0}, {Kind::Gate, 2, 350}, {Kind::Gate, 0, 350}};
constexpr Step P_TUNNEL[] = {{Kind::Gate, ANY, 0}, {Kind::Gate, SAME, 120}, {Kind::Gate, SAME, 120}};
constexpr Step P_CAR[] = {{Kind::Car, ANY, 0}};
constexpr Step P_OVERTAKE[] = {{Kind::Car, 0, 0}, {Kind::Car, 2, 250}};
constexpr Step P_WALL[] = {{Kind::Wall, ANY, 0}};
constexpr Step P_OIL[] = {{Kind::Oil, ANY, 0}, {Kind::Gate, ANY, 300}};
constexpr Step P_COINS[] = {{Kind::Coins, ANY, 0}};
constexpr Step P_BONUS[] = {{Kind::Gate, ANY, 0}, {Kind::Coins, SAME, 150}};

#define PATTERN(name, steps) {name, steps, sizeof(steps) / sizeof(steps[0])}
constexpr Pattern PATTERNS[] = {
    PATTERN("gate", P_GATE),       PATTERN("slalom", P_SLALOM), PATTERN("tunnel", P_TUNNEL),
    PATTERN("car", P_CAR),         PATTERN("overtake", P_OVERTAKE), PATTERN("wall", P_WALL),
    PATTERN("oil", P_OIL),         PATTERN("coins", P_COINS),   PATTERN("bonus", P_BONUS),
};
constexpr int NUM_PATTERNS = sizeof(PATTERNS) / sizeof(PATTERNS[0]);

}  // namespace

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------
void PatternSpawner::reset(uint32_t timeMs) {
    queued_ = 0;
    // Start planning one full traffic travel time ahead, so the first
    // obstacles of every type appear at the top, never mid-screen.
    planEndMs_ = timeMs + travelMs(HazardType::Traffic, timeMs);
    // The player can drive anywhere until the first window: assume the worst.
    safe_ = {MIN_X, MAX_X};
    lastGateX_ = laneX(1);
    lastStaticArrivalMs_ = lastOilArrivalMs_ = 0;
    lastPattern_ = -1;
    patternName_ = "";
}

void PatternSpawner::update(uint32_t timeMs, float dist, HazardField& field) {
    while (planEndMs_ < timeMs + LOOKAHEAD_MS && queued_ < QUEUE - 6) planPattern();

    for (int i = 0; i < queued_;) {
        const Hazard& e = queue_[i];
        const float y = difficulty::obstacleY(e.type, e.arrivalMs, e.arrivalDist, timeMs, dist);
        if (y < cfg::OBSTACLE_SPAWN_Y) { i++; continue; }    // not on screen yet
        if (!field.spawn(e, timeMs, dist)) { i++; continue; } // pool full, retry
        queue_[i] = queue_[--queued_];                         // remove (order not needed)
    }
}

// ---------------------------------------------------------------------------
// Planning
// ---------------------------------------------------------------------------
void PatternSpawner::planPattern() {
    int p;
    do { p = random(NUM_PATTERNS); } while (p == lastPattern_);
    lastPattern_ = p;
    patternName_ = PATTERNS[p].name;
    if (cfg::DEBUG_LOG) Serial.printf("[MQ] plan %s\n", patternName_);

    for (int i = 0; i < PATTERNS[p].count; i++) {
        const Step& s = PATTERNS[p].steps[i];
        const uint16_t base = i == 0 ? cfg::PATTERN_GAP_MS : s.afterMs;
        const uint16_t after = (uint16_t)(base * difficulty::spacingScale(planEndMs_));
        switch (s.kind) {
            case Kind::Gate:  planGate(s.lane, after);           break;
            case Kind::Car:   planTraffic(s.lane, false, after); break;
            case Kind::Wall:  planTraffic(s.lane, true, after);  break;
            case Kind::Oil:   planOil(after);                    break;
            case Kind::Coins: planCoins(s.lane, after);          break;
        }
    }
}

// Roadblock with a gap. Start from the pattern's position and slide the gap
// toward the player until every possible position can reach it in time.
void PatternSpawner::planGate(int8_t lane, uint16_t afterMs) {
    uint32_t start = planEndMs_ + afterMs;
    const float w = difficulty::gapPx(start);
    const float gMin = gateMin(w), gMax = gateMax(w), slack = gateSlack(w);
    float g = lane == ANY  ? (float)random((long)gMin, (long)gMax + 1)
            : lane == SAME ? lastGateX_
                           : laneX(lane);
    g = clampf(g, gMin, gMax);
    const float toward = clampf((safe_.l + safe_.r) / 2, gMin, gMax);

    Span d;
    float need = 0;
    for (int i = 0; i < 60; i++) {
        d = {clampf(g - slack, MIN_X, MAX_X), clampf(g + slack, MIN_X, MAX_X)};
        need = requiredShift(&d, 1);
        if (need <= shiftAvailable(start) || fabsf(g - toward) < 0.5f) break;
        g += (toward > g) ? fminf(2.0f, toward - g) : -fminf(2.0f, g - toward);
    }
    if (need > shiftAvailable(start)) start = startForShift(need);   // still too far: wait

    const uint32_t half = halfWindowMs(HazardType::Barrier, start);
    addEvent(HazardType::Barrier, g, w, start + half);
    lastGateX_ = g;
    lastStaticArrivalMs_ = start + half;
    commit(&d, 1, start, start + 2 * half);
}

// One traffic car (lane = its lane) or a wall of two (lane = the free lane).
// Try the pattern's lane first, then the others; pick the first that is
// fair, else the least demanding one, delayed until it is fair.
void PatternSpawner::planTraffic(int8_t lane, bool wall, uint16_t afterMs) {
    uint32_t start = planEndMs_ + afterMs;
    // No overtaking on screen: a slow car must appear at the top after every
    // faster obstacle already planned has appeared.
    if (lastStaticArrivalMs_) {
        const uint32_t gap = travelMs(HazardType::Traffic, lastStaticArrivalMs_) -
                             travelMs(HazardType::Barrier, lastStaticArrivalMs_);
        const uint32_t half = halfWindowMs(HazardType::Traffic, lastStaticArrivalMs_);
        if (start + half < lastStaticArrivalMs_ + gap) start = lastStaticArrivalMs_ + gap - half;
    }

    const int first = lane >= 0 ? lane : random(cfg::ROAD_LANES);
    int bestLane = first;
    float bestNeed = 1e9f;
    Span bestD[3];
    int bestN = 0;
    for (int k = 0; k < cfg::ROAD_LANES; k++) {
        const int l = (first + k) % cfg::ROAD_LANES;
        int lanes[2], n = 0;
        if (wall) { for (int j = 0; j < cfg::ROAD_LANES; j++) if (j != l) lanes[n++] = j; }
        else      { lanes[n++] = l; }
        Span d[3];
        const int dn = freeSpans(lanes, n, d);
        if (dn == 0) continue;
        const float need = requiredShift(d, dn);
        if (need < bestNeed) {
            bestNeed = need; bestLane = l; bestN = dn;
            for (int j = 0; j < dn; j++) bestD[j] = d[j];
        }
        if (need <= shiftAvailable(start)) break;
    }
    if (bestNeed > shiftAvailable(start)) start = startForShift(bestNeed);

    const uint32_t half = halfWindowMs(HazardType::Traffic, start);
    const uint32_t arrival = start + half;
    if (wall) {
        for (int j = 0; j < cfg::ROAD_LANES; j++)
            if (j != bestLane) addEvent(HazardType::Traffic, laneX(j), 0, arrival, random(3));
    } else {
        addEvent(HazardType::Traffic, laneX(bestLane), 0, arrival, random(3));
    }
    commit(bestD, bestN, start, start + 2 * half);
}

// Oil across the whole road: no position constraint, but it must be spaced
// so the previous jump has landed.
void PatternSpawner::planOil(uint16_t afterMs) {
    uint32_t start = planEndMs_ + afterMs;
    const uint32_t half = halfWindowMs(HazardType::Oil, start);
    if (lastOilArrivalMs_ && start + half < lastOilArrivalMs_ + cfg::OIL_MIN_SPACING_MS)
        start = lastOilArrivalMs_ + cfg::OIL_MIN_SPACING_MS - half;
    const float mid = (cfg::ROAD_LEFT + cfg::ROAD_RIGHT) / 2.0f;
    addEvent(HazardType::Oil, mid, 0, start + half);
    lastOilArrivalMs_ = lastStaticArrivalMs_ = start + half;
    const Span all = {MIN_X, MAX_X};
    commit(&all, 1, start, start + 2 * half);
}

// A row of three coins, always within reach. Not a constraint: the player may
// ignore them, so afterwards they may be anywhere they could reach.
void PatternSpawner::planCoins(int8_t lane, uint16_t afterMs) {
    const uint32_t start = planEndMs_ + afterMs;
    const uint32_t half = halfWindowMs(HazardType::Coin, start);
    const float reach = shiftAvailable(start);
    float x = lane == ANY  ? (float)random((long)MIN_X, (long)MAX_X + 1)
            : lane == SAME ? lastGateX_
                           : laneX(lane);
    x = clampf(x, fmaxf(MIN_X, safe_.l - reach), fminf(MAX_X, safe_.r + reach));
    uint32_t arrival = start + half;
    for (int i = 0; i < 3; i++, arrival += COIN_SPACING_MS) addEvent(HazardType::Coin, x, 0, arrival);
    lastStaticArrivalMs_ = arrival - COIN_SPACING_MS;
    const Span all = {MIN_X, MAX_X};
    commit(&all, 1, start, arrival - COIN_SPACING_MS + half);
}

// ---------------------------------------------------------------------------
// Fairness helpers
// ---------------------------------------------------------------------------
// How far the player can move between the end of the last window and startMs.
float PatternSpawner::shiftAvailable(uint32_t startMs) const {
    const int32_t t = (int32_t)(startMs - planEndMs_) - (int32_t)cfg::FAIR_REACTION_MS;
    return t > 0 ? t * REACH_PX_PER_MS : 0.0f;
}

uint32_t PatternSpawner::startForShift(float shiftPx) const {
    return planEndMs_ + cfg::FAIR_REACTION_MS + (uint32_t)ceilf(shiftPx / REACH_PX_PER_MS);
}

// The largest distance any possible player position has to travel to reach
// the safe region d (the worst case decides fairness).
float PatternSpawner::requiredShift(const Span* d, int n) const {
    float worst = 0;
    for (float p = safe_.l;; p += 2.0f) {
        if (p > safe_.r) p = safe_.r;
        const float dd = distTo(p, d, n);
        if (dd > worst) worst = dd;
        if (p >= safe_.r) break;
    }
    return worst;
}

// After the window the player is somewhere in d that they could have reached.
void PatternSpawner::commit(const Span* d, int n, uint32_t startMs, uint32_t endMs) {
    const float shift = shiftAvailable(startMs);
    const float lo = safe_.l - shift, hi = safe_.r + shift;
    Span hull = {1e9f, -1e9f};
    for (int i = 0; i < n; i++) {
        const float l = fmaxf(d[i].l, lo), r = fminf(d[i].r, hi);
        if (l > r) continue;
        hull.l = fminf(hull.l, l);
        hull.r = fmaxf(hull.r, r);
    }
    if (hull.l > hull.r) hull = {d[0].l, d[0].r};   // cannot happen when fair; stay safe
    safe_ = hull;
    planEndMs_ = endMs;
}

bool PatternSpawner::addEvent(HazardType t, float x, float w, uint32_t arrivalMs, uint8_t variant) {
    if (queued_ >= QUEUE) return false;
    Hazard& e = queue_[queued_++];
    e = Hazard{};
    e.type = t;
    e.x = x;
    e.w = w;
    e.arrivalMs = arrivalMs;
    e.arrivalDist = difficulty::distancePx(arrivalMs);
    e.variant = variant;
    e.pattern = patternName_;
    return true;
}

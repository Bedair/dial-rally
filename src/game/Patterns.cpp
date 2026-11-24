#include "game/Patterns.h"
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
constexpr float GATE_MIN = INNER_L + cfg::GAP_PX / 2;                   // gap centre limits
constexpr float GATE_MAX = INNER_R - cfg::GAP_PX / 2;
// The car fits through a gap when |x - gapCentre| <= GATE_SLACK.
constexpr float GATE_SLACK = cfg::GAP_PX / 2 - CAR_HALF;
// The car clears a traffic car when |x - trafficX| >= TRAFFIC_CLEAR.
constexpr float TRAFFIC_CLEAR = cfg::TRAFFIC_W / 2.0f - cfg::HITBOX_MARGIN + CAR_HALF;

constexpr float REACH_PX_PER_MS = cfg::FAIR_STEPS_PER_S * cfg::KNOB_PX_PER_STEP / 1000.0f;
constexpr uint32_t COIN_SPACING_MS = 160;

inline float laneX(int lane) { return cfg::ROAD_LEFT + LANE_W * (lane + 0.5f); }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ---------------------------------------------------------------------------
// Timing: how fast each type comes down, how long it overlaps the car's line
// ---------------------------------------------------------------------------
float speedOf(HazardType t) {
    return t == HazardType::Traffic ? cfg::ROAD_SPEED_PX_S - cfg::TRAFFIC_PX_S
                                    : cfg::ROAD_SPEED_PX_S;
}

float heightOf(HazardType t) {
    switch (t) {
        case HazardType::Barrier: return cfg::BARRIER_H;
        case HazardType::Oil:     return cfg::OIL_H;
        case HazardType::Traffic: return cfg::TRAFFIC_H;
        case HazardType::Coin:    return 2 * cfg::COIN_R;
    }
    return 0;
}

// From spawn line to the car's line, ms.
uint32_t travelMs(HazardType t) {
    return (uint32_t)((cfg::CAR_Y - cfg::OBSTACLE_SPAWN_Y) * 1000.0f / speedOf(t));
}

// Half of the time the obstacle's box overlaps the car's box vertically, ms.
uint32_t halfWindowMs(HazardType t) {
    const float overlapPx = (cfg::CAR_H + heightOf(t)) / 2.0f - 2 * cfg::HITBOX_MARGIN;
    return (uint32_t)(overlapPx * 1000.0f / speedOf(t));
}

// Plan this far ahead: the slowest obstacle (traffic) must be planned before
// it has to appear at the top.
const uint32_t LOOKAHEAD_MS = travelMs(HazardType::Traffic) + 800;

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
    uint16_t afterMs;   // gap after the previous step's window ends
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
    planEndMs_ = timeMs + travelMs(HazardType::Traffic);
    // The player can drive anywhere until the first window: assume the worst.
    safe_ = {MIN_X, MAX_X};
    lastGateX_ = laneX(1);
    lastStaticArrivalMs_ = lastOilArrivalMs_ = 0;
    lastPattern_ = -1;
}

void PatternSpawner::update(uint32_t timeMs, HazardField& field) {
    while (planEndMs_ < timeMs + LOOKAHEAD_MS && queued_ < QUEUE - 6) planPattern();

    for (int i = 0; i < queued_;) {
        Event& e = queue_[i];
        const uint32_t travel = travelMs(e.type);
        if (timeMs + travel < e.arrivalMs) { i++; continue; }     // not yet
        Hazard* h = field.spawn(e.type, e.x, e.w);
        if (!h) { i++; continue; }                                 // pool full, retry
        // Place it exactly where it must be to arrive on time.
        h->y = cfg::CAR_Y - speedOf(e.type) * (float)(e.arrivalMs - timeMs) / 1000.0f;
        h->variant = e.variant;
        queue_[i] = queue_[--queued_];                             // remove (order not needed)
    }
}

// ---------------------------------------------------------------------------
// Planning
// ---------------------------------------------------------------------------
void PatternSpawner::planPattern() {
    int p;
    do { p = random(NUM_PATTERNS); } while (p == lastPattern_);
    lastPattern_ = p;
    if (cfg::DEBUG_LOG) Serial.printf("[MQ] pattern %s\n", PATTERNS[p].name);

    for (int i = 0; i < PATTERNS[p].count; i++) {
        const Step& s = PATTERNS[p].steps[i];
        const uint16_t after = i == 0 ? cfg::PATTERN_GAP_MS : s.afterMs;
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
    float g = lane == ANY  ? (float)random((long)GATE_MIN, (long)GATE_MAX + 1)
            : lane == SAME ? lastGateX_
                           : laneX(lane);
    g = clampf(g, GATE_MIN, GATE_MAX);
    const float toward = clampf((safe_.l + safe_.r) / 2, GATE_MIN, GATE_MAX);

    Span d;
    float need = 0;
    for (int i = 0; i < 60; i++) {
        d = {clampf(g - GATE_SLACK, MIN_X, MAX_X), clampf(g + GATE_SLACK, MIN_X, MAX_X)};
        need = requiredShift(&d, 1);
        if (need <= shiftAvailable(start) || fabsf(g - toward) < 0.5f) break;
        g += (toward > g) ? fminf(2.0f, toward - g) : -fminf(2.0f, g - toward);
    }
    if (need > shiftAvailable(start)) start = startForShift(need);   // still too far: wait

    const uint32_t half = halfWindowMs(HazardType::Barrier);
    addEvent(HazardType::Barrier, g, cfg::GAP_PX, start + half);
    lastGateX_ = g;
    lastStaticArrivalMs_ = start + half;
    commit(&d, 1, start, start + 2 * half);
}

// One traffic car (lane = its lane) or a wall of two (lane = the free lane).
// Try the pattern's lane first, then the others; pick the first that is
// fair, else the least demanding one, delayed until it is fair.
void PatternSpawner::planTraffic(int8_t lane, bool wall, uint16_t afterMs) {
    const uint32_t half = halfWindowMs(HazardType::Traffic);
    uint32_t start = planEndMs_ + afterMs;
    // No overtaking on screen: a slow car must appear at the top after every
    // faster obstacle already planned has appeared.
    const uint32_t gap = travelMs(HazardType::Traffic) - travelMs(HazardType::Barrier);
    if (lastStaticArrivalMs_ && start + half < lastStaticArrivalMs_ + gap)
        start = lastStaticArrivalMs_ + gap - half;

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
    const uint32_t half = halfWindowMs(HazardType::Oil);
    uint32_t start = planEndMs_ + afterMs;
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
    const uint32_t half = halfWindowMs(HazardType::Coin);
    const uint32_t start = planEndMs_ + afterMs;
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
    queue_[queued_++] = {t, x, w, arrivalMs, variant};
    return true;
}

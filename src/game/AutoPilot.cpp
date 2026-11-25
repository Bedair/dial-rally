#include "game/AutoPilot.h"
#include "game/Difficulty.h"
#include "config/GameConfig.h"
#include <math.h>

namespace {
constexpr float JUMP_LEAD_S = 0.30f;   // press when an oil spill is this far away (time)
constexpr float MIN_X = cfg::ROAD_LEFT + cfg::KERB_W + cfg::CAR_W / 2.0f;
constexpr float MAX_X = cfg::ROAD_RIGHT - cfg::KERB_W - cfg::CAR_W / 2.0f;
constexpr float TRAFFIC_CLEAR = cfg::TRAFFIC_W / 2.0f + cfg::CAR_W / 2.0f;   // with margin
}

void AutoPilot::reset() {
    budget_ = 0;
    for (bool& j : jumpedFor_) j = false;
}

AutoPilot::Decision AutoPilot::drive(const GameEngine& g, uint32_t dtMs) {
    Decision d;
    const HazardField& hz = g.hazards();
    const float px = g.player().targetX();

    // Jump once for every oil spill that is about to arrive.
    for (int i = 0; i < HazardField::MAX; i++) {
        const Hazard& h = hz[i];
        if (!h.active || h.type != HazardType::Oil) { jumpedFor_[i] = false; continue; }
        const float eta = (cfg::CAR_Y - h.y) / difficulty::approachSpeed(h.type, g.timeMs());
        if (eta > 0 && eta < JUMP_LEAD_S && !jumpedFor_[i]) {
            d.jump = true;
            jumpedFor_[i] = true;
        }
    }

    // The obstacle that needs the car's attention next: anything still
    // overlapping the car first, otherwise the one arriving soonest.
    int next = -1;
    float nextEta = 1e9f;
    for (int i = 0; i < HazardField::MAX; i++) {
        const Hazard& h = hz[i];
        if (!h.active || h.type == HazardType::Coin || h.type == HazardType::Oil) continue;
        const float top = cfg::CAR_Y - cfg::CAR_H / 2.0f, bottom = cfg::CAR_Y + cfg::CAR_H / 2.0f;
        float eta;
        if (h.y + 20 < top) eta = (cfg::CAR_Y - h.y) / difficulty::approachSpeed(h.type, g.timeMs());
        else if (h.y - 20 < bottom) eta = -1;          // overlapping now
        else continue;                                 // already behind the car
        if (eta < nextEta) { nextEta = eta; next = i; }
    }

    float target = px;
    if (next >= 0) {
        const Hazard& h = hz[next];
        if (h.type == HazardType::Barrier) {
            target = h.x;
        } else {
            // Traffic: nearest x that clears every traffic car on the same row.
            float best = px, bestDist = 1e9f;
            for (float x = MIN_X; x <= MAX_X; x += cfg::KNOB_PX_PER_STEP) {
                bool clear = true;
                for (int j = 0; j < HazardField::MAX && clear; j++) {
                    const Hazard& o = hz[j];
                    if (o.active && o.type == HazardType::Traffic && fabsf(o.y - h.y) < 40 &&
                        fabsf(x - o.x) < TRAFFIC_CLEAR)
                        clear = false;
                }
                if (clear && fabsf(x - px) < bestDist) { bestDist = fabsf(x - px); best = x; }
            }
            target = best;
        }
    }

    // Turn toward the target no faster than the fairness promise allows.
    budget_ += cfg::FAIR_STEPS_PER_S * dtMs / 1000.0f;
    if (budget_ > 2.0f) budget_ = 2.0f;
    int32_t steps = (int32_t)lroundf((target - px) / cfg::KNOB_PX_PER_STEP);
    const int32_t allowed = (int32_t)budget_;
    if (steps > allowed) steps = allowed;
    if (steps < -allowed) steps = -allowed;
    budget_ -= (float)(steps < 0 ? -steps : steps);
    d.steps = steps;
    return d;
}

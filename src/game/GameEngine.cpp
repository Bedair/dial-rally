#include "game/GameEngine.h"
#include "game/Difficulty.h"
#include "config/GameConfig.h"
#include <Arduino.h>
#include <math.h>

namespace {
// Road markings repeat every this many px, so the scroll can wrap without a jump.
constexpr float SCROLL_WRAP = (cfg::DASH_LEN + cfg::DASH_GAP) * (float)cfg::KERB_SEGMENT * 2;

constexpr float M = cfg::HITBOX_MARGIN;
constexpr float CAR_HALF_W = cfg::CAR_W / 2.0f - M;
constexpr float CAR_TOP = cfg::CAR_Y - cfg::CAR_H / 2.0f + M;
constexpr float CAR_BOTTOM = cfg::CAR_Y + cfg::CAR_H / 2.0f - M;

// Half height of an obstacle's hit box.
float halfHeight(HazardType t) {
    switch (t) {
        case HazardType::Barrier: return cfg::BARRIER_H / 2.0f - M;
        case HazardType::Oil:     return cfg::OIL_H / 2.0f - M;
        case HazardType::Traffic: return cfg::TRAFFIC_H / 2.0f - M;
        case HazardType::Coin:    return cfg::COIN_R;
    }
    return 0;
}
}  // namespace

void GameEngine::reset() {
    player_.reset();
    hazards_.clear();
    timeMs_ = 0;
    spawner_.reset(timeMs_);
    passed_ = coins_ = milestones_ = 0;
    scroll_ = 0.0f;
    crashCause_ = nullptr;
    crashPattern_ = "";
}

void GameEngine::steer(int32_t knobSteps) {
    player_.steer(knobSteps * cfg::KNOB_PX_PER_STEP);
}

uint32_t GameEngine::score() const {
    return passed_ * cfg::POINTS_PER_OBSTACLE + coins_ * cfg::POINTS_PER_COIN +
           milestones_ * cfg::MILESTONE_BONUS;
}

float GameEngine::roadSpeed() const {
    return difficulty::roadSpeed(timeMs_);
}

void GameEngine::update(uint32_t dtMs, bool world) {
    player_.update(dtMs);
    if (!world || crashed()) return;

    timeMs_ += dtMs;
    const float dist = difficulty::distancePx(timeMs_);
    scroll_ = fmodf(dist, SCROLL_WRAP);

    // Move first, then spawn: new obstacles are placed for the current time.
    passed_ += hazards_.update(timeMs_, dist);
    spawner_.update(timeMs_, dist, hazards_);
    checkCollisions();

    // Milestone bonus every MILESTONE_EVERY obstacles.
    while (passed_ >= (milestones_ + 1) * cfg::MILESTONE_EVERY) {
        milestones_++;
        if (cfg::DEBUG_LOG)
            Serial.printf("[MQ] milestone %lu: +%lu, speed %.0f px/s, gap %.0f px\n",
                          (unsigned long)milestones_, (unsigned long)cfg::MILESTONE_BONUS,
                          difficulty::roadSpeed(timeMs_), difficulty::gapPx(timeMs_));
    }
}

// Uses the knob's target x (see Player.h) and slightly shrunk hit boxes.
void GameEngine::checkCollisions() {
    const float px = player_.targetX();
    for (int i = 0; i < HazardField::MAX; i++) {
        Hazard& h = hazards_[i];
        if (!h.active || !h.armed) continue;
        const float hh = halfHeight(h.type);
        if (h.y + hh < CAR_TOP || h.y - hh > CAR_BOTTOM) continue;   // no vertical overlap

        switch (h.type) {
            case HazardType::Barrier:
                if (px - CAR_HALF_W < h.x - h.w / 2 || px + CAR_HALF_W > h.x + h.w / 2)
                    crashCause_ = "roadblock";
                break;
            case HazardType::Oil:
                if (!player_.airborne()) crashCause_ = "oil";
                break;
            case HazardType::Traffic:
                if (fabsf(px - h.x) < cfg::TRAFFIC_W / 2.0f - M + CAR_HALF_W)
                    crashCause_ = "traffic";
                break;
            case HazardType::Coin:
                if (fabsf(px - h.x) < cfg::COIN_R + CAR_HALF_W) {
                    coins_++;
                    h.active = false;
                    if (cfg::DEBUG_LOG) Serial.printf("[MQ] coin (%lu)\n", (unsigned long)coins_);
                }
                break;
        }
        if (crashed()) {
            crashPattern_ = h.pattern;
            return;
        }
    }
}

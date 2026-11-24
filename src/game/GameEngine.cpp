#include "game/GameEngine.h"
#include "config/GameConfig.h"
#include <Arduino.h>

namespace {
constexpr float LANE_W = (cfg::ROAD_RIGHT - cfg::ROAD_LEFT) / (float)cfg::ROAD_LANES;
inline float laneCentre(int lane) { return cfg::ROAD_LEFT + LANE_W * (lane + 0.5f); }
// Road markings repeat every this many px, so scroll_ can wrap without a jump.
constexpr float SCROLL_WRAP = (cfg::DASH_LEN + cfg::DASH_GAP) * (float)cfg::KERB_SEGMENT * 2;
}

void GameEngine::reset() {
    player_.reset();
    hazards_.clear();
    score_ = 0;
    scroll_ = 0.0f;
    spawnTimerMs_ = 0;
    demoIndex_ = 0;
}

void GameEngine::steer(int32_t knobSteps) {
    player_.steer(knobSteps * cfg::KNOB_PX_PER_STEP);
}

void GameEngine::update(uint32_t dtMs, bool world) {
    player_.update(dtMs);
    if (!world) return;

    scroll_ += cfg::DEMO_SPEED_PX_S * dtMs / 1000.0f;
    if (scroll_ >= SCROLL_WRAP) scroll_ -= SCROLL_WRAP;

    spawnTimerMs_ += dtMs;
    if (spawnTimerMs_ >= cfg::DEMO_SPAWN_MS) {
        spawnTimerMs_ -= cfg::DEMO_SPAWN_MS;
        demoSpawn();
    }
    score_ += hazards_.update(dtMs, cfg::DEMO_SPEED_PX_S, cfg::DEMO_TRAFFIC_PX_S);
}

// M2 only: replaced by fair predefined patterns in M3.
void GameEngine::demoSpawn() {
    static const HazardType order[] = {HazardType::Barrier, HazardType::Coin, HazardType::Oil,
                                       HazardType::Traffic};
    const HazardType t = order[demoIndex_++ % 4];
    switch (t) {
        case HazardType::Barrier: {
            const float half = cfg::DEMO_GAP_PX / 2;
            const float x = random((long)(cfg::ROAD_LEFT + cfg::KERB_W + half),
                                   (long)(cfg::ROAD_RIGHT - cfg::KERB_W - half));
            hazards_.spawn(t, x, cfg::DEMO_GAP_PX);
            break;
        }
        case HazardType::Traffic: {
            Hazard* h = hazards_.spawn(t, laneCentre(random(cfg::ROAD_LANES)));
            if (h) h->variant = random(3);
            break;
        }
        case HazardType::Coin:
            hazards_.spawn(t, laneCentre(random(cfg::ROAD_LANES)));
            break;
        case HazardType::Oil:
            hazards_.spawn(t, (cfg::ROAD_LEFT + cfg::ROAD_RIGHT) / 2.0f);
            break;
    }
}

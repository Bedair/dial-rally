#pragma once
// Difficulty curve as pure functions of game time (ms since GO, frozen while
// paused). Everything ramps linearly from its start value to its cap over
// DIFF_RAMP_MS, then stays at the cap.
//
// Because road speed is a known function of time, the distance driven is
// too (distancePx). Obstacle positions are computed from it every frame
// instead of being integrated, so an obstacle planned to reach the car at
// time T arrives exactly at T even while the road accelerates. The fairness
// planner relies on that.

#include <stdint.h>
#include "config/GameConfig.h"
#include "game/Hazard.h"

namespace difficulty {

// 0 at GO, 1 once the ramp is complete.
inline float level(uint32_t tMs) {
    return tMs >= cfg::DIFF_RAMP_MS ? 1.0f : (float)tMs / cfg::DIFF_RAMP_MS;
}

inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

// Road speed, px/s.
inline float roadSpeed(uint32_t tMs) {
    return lerp(cfg::ROAD_SPEED_START, cfg::ROAD_SPEED_MAX, level(tMs));
}

// Distance driven since GO, px: the integral of roadSpeed.
inline float distancePx(uint32_t tMs) {
    const float v0 = cfg::ROAD_SPEED_START, v1 = cfg::ROAD_SPEED_MAX;
    const float T = cfg::DIFF_RAMP_MS / 1000.0f;
    const float t = tMs / 1000.0f;
    if (t <= T) return v0 * t + (v1 - v0) * t * t / (2 * T);
    return v0 * T + (v1 - v0) * T / 2 + v1 * (t - T);
}

// Roadblock gap width, px.
inline float gapPx(uint32_t tMs) { return lerp(cfg::GAP_START_PX, cfg::GAP_MIN_PX, level(tMs)); }

// Factor for the time between pattern steps (1 at GO, shrinking to the cap).
inline float spacingScale(uint32_t tMs) { return lerp(1.0f, cfg::SPACING_MIN_SCALE, level(tMs)); }

// How fast an obstacle comes down the screen at time t, px/s.
inline float approachSpeed(HazardType type, uint32_t tMs) {
    const float v = roadSpeed(tMs);
    return type == HazardType::Traffic ? v - cfg::TRAFFIC_PX_S : v;
}

// Screen y of an obstacle that reaches the car's line (CAR_Y) at arrivalMs.
// arrivalDist = distancePx(arrivalMs), dist = distancePx(tMs).
// Static obstacles stay at a fixed road position; traffic also drives forward.
inline float obstacleY(HazardType type, uint32_t arrivalMs, float arrivalDist,
                       uint32_t tMs, float dist) {
    float y = cfg::CAR_Y - (arrivalDist - dist);
    if (type == HazardType::Traffic)
        y += cfg::TRAFFIC_PX_S * ((float)arrivalMs - (float)tMs) / 1000.0f;
    return y;
}

}  // namespace difficulty

#pragma once
// Obstacles come down the road toward the car.
//   Barrier  roadblock across the road with one gap (x = gap centre, w = gap width)
//   Oil      oil spill across the whole road: press to jump over it
//   Traffic  slower car in a lane (x = centre); it drives forward, so it
//            comes down slower than the road scrolls
//   Coin     pickup at x
// HazardField is a fixed pool (no heap allocation during play).
//
// Positions are not integrated: each obstacle knows when it reaches the car
// (arrivalMs) and y is recomputed from the distance driven every frame
// (difficulty::obstacleY), so it stays exact while the road accelerates.

#include <stdint.h>

enum class HazardType : uint8_t { Barrier, Oil, Traffic, Coin };

struct Hazard {
    bool active = false;
    HazardType type = HazardType::Barrier;
    float y = 0.0f;           // screen y of the obstacle's centre
    float x = 0.0f;           // gap centre (Barrier) or object centre (Traffic, Coin)
    float w = 0.0f;           // gap width (Barrier only)
    uint32_t arrivalMs = 0;   // game time when its centre reaches CAR_Y
    float arrivalDist = 0.0f; // distance driven at arrivalMs
    uint8_t variant = 0;      // cosmetic: traffic car colour
    const char* pattern = ""; // pattern it belongs to (for the crash log)
    bool armed = false;       // false = telegraph (drawn faint), true = solid / dangerous
    bool passed = false;      // already went past the car's line
};

class HazardField {
public:
    static constexpr int MAX = 16;

    void clear();
    // Adds an obstacle; y is set for the current time. nullptr if the pool is full.
    Hazard* spawn(const Hazard& h, uint32_t tMs, float dist);
    // Recomputes every y for game time tMs / distance dist. Returns how many
    // obstacles (not coins) passed the car's line this frame.
    int update(uint32_t tMs, float dist);

    const Hazard& operator[](int i) const { return pool_[i]; }
    Hazard& operator[](int i) { return pool_[i]; }

private:
    Hazard pool_[MAX];
};

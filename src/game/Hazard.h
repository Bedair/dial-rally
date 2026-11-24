#pragma once
// Obstacles come down the road toward the car.
//   Barrier  roadblock across the road with one gap (x = gap centre, w = gap width)
//   Oil      oil spill across the whole road: press to jump over it
//   Traffic  slower car in a lane (x = centre); it drives forward, so it
//            comes down slower than the road scrolls
//   Coin     pickup at x
// HazardField is a fixed pool (no heap allocation during play).

#include <stdint.h>

enum class HazardType : uint8_t { Barrier, Oil, Traffic, Coin };

struct Hazard {
    bool active = false;
    HazardType type = HazardType::Barrier;
    float y = 0.0f;          // screen y of the obstacle's centre
    float x = 0.0f;          // gap centre (Barrier) or object centre (Traffic, Coin)
    float w = 0.0f;          // gap width (Barrier only)
    uint8_t variant = 0;     // cosmetic: traffic car colour
    bool armed = false;      // false = telegraph (drawn faint), true = solid / dangerous
    bool passed = false;     // already went past the car's line
};

class HazardField {
public:
    static constexpr int MAX = 16;

    void clear();
    // Starts an obstacle just above the screen. Returns nullptr if the pool is full.
    Hazard* spawn(HazardType type, float x, float w = 0.0f);
    // Moves every obstacle down. scrollPxPerS is the road speed; traffic moves
    // slower by trafficPxPerS. Returns how many obstacles (not coins) passed
    // the car's line this frame: +1 score each.
    int update(uint32_t dtMs, float scrollPxPerS, float trafficPxPerS);

    const Hazard& operator[](int i) const { return pool_[i]; }
    Hazard& operator[](int i) { return pool_[i]; }

private:
    Hazard pool_[MAX];
};

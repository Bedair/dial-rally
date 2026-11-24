#pragma once
// Fair obstacle generation from predefined patterns.
//
// A pattern is a short list of steps (roadblock gate, traffic car, traffic
// wall, oil spill, coin row). The spawner plans each step ahead of time as a
// *window*: the time span in which the obstacle overlaps the car's line, and
// the x region the car must be in during it ("safe region").
//
// Fairness rule: before committing a step, the spawner checks that from
// EVERY position the player could be in after the previous window, the new
// safe region can be reached in time, assuming FAIR_REACTION_MS reaction and
// FAIR_STEPS_PER_S knob speed. If not, the obstacle is moved toward the
// player and, if that is not enough, the step is delayed. So the generator
// can never produce an impossible sequence.
//
// Planned obstacles are spawned at the top exactly early enough to reach the
// car's line at their planned time (traffic is slower, so it spawns earlier).

#include <stdint.h>
#include "game/Hazard.h"

class PatternSpawner {
public:
    void reset(uint32_t timeMs);
    // Plans ahead and spawns obstacles whose time has come. timeMs is game
    // time (frozen while paused).
    void update(uint32_t timeMs, HazardField& field);

    struct Span { float l, r; };   // x interval of car-centre positions

private:
    struct Event {
        HazardType type;
        float x, w;
        uint32_t arrivalMs;        // when its centre reaches CAR_Y
        uint8_t variant;
    };

    void planPattern();
    void planGate(int8_t lane, uint16_t afterMs);
    void planTraffic(int8_t lane, bool wall, uint16_t afterMs);
    void planOil(uint16_t afterMs);
    void planCoins(int8_t lane, uint16_t afterMs);

    // Fairness helpers (see Patterns.cpp).
    float shiftAvailable(uint32_t startMs) const;
    uint32_t startForShift(float shiftPx) const;
    float requiredShift(const Span* d, int n) const;
    void commit(const Span* d, int n, uint32_t startMs, uint32_t endMs);

    bool addEvent(HazardType t, float x, float w, uint32_t arrivalMs, uint8_t variant = 0);

    static constexpr int QUEUE = 32;
    Event queue_[QUEUE];
    int queued_ = 0;

    uint32_t planEndMs_ = 0;       // end of the last planned window
    Span safe_ = {0, 0};           // where the player can be at planEndMs_
    float lastGateX_ = 0;
    uint32_t lastStaticArrivalMs_ = 0;   // for traffic spacing (no overtaking)
    uint32_t lastOilArrivalMs_ = 0;
    int lastPattern_ = -1;
};

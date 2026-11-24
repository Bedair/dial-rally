#pragma once
// The game world during a run: car, obstacles, road, score, crashes.
// GameController decides *when* the world runs (states); GameEngine decides
// *what* happens in it. DisplayManager only reads it.
//
// Each frame (while the world runs): game time advances, obstacle positions
// are recomputed from the distance driven (Difficulty.h), the PatternSpawner
// adds new ones, then collisions are checked against the car:
//   Roadblock  crash unless the car is fully inside the gap
//   Oil        crash unless the car is airborne (jump)
//   Traffic    crash on overlap
//   Coin       collected on overlap
// Only solid (armed) obstacles collide; faint ones are a warning.
//
// Score = POINTS_PER_OBSTACLE × obstacles passed + POINTS_PER_COIN × coins
//       + MILESTONE_BONUS × milestones (one every MILESTONE_EVERY obstacles).

#include <stdint.h>
#include "game/Player.h"
#include "game/Hazard.h"
#include "game/Patterns.h"

class GameEngine {
public:
    void reset();
    void steer(int32_t knobSteps);
    void jump() { player_.jump(); }
    // world = false: only the car moves (Ready countdown).
    void update(uint32_t dtMs, bool world);

    const Player& player() const { return player_; }
    const HazardField& hazards() const { return hazards_; }
    uint32_t score() const;
    uint32_t passed() const { return passed_; }
    uint32_t coins() const { return coins_; }
    uint32_t milestones() const { return milestones_; }
    uint32_t timeMs() const { return timeMs_; }
    float roadSpeed() const;
    // Distance driven in px, wrapped; the renderer scrolls road markings with it.
    float scroll() const { return scroll_; }

    bool crashed() const { return crashCause_ != nullptr; }
    const char* crashCause() const { return crashCause_; }       // for the log
    const char* crashPattern() const { return crashPattern_; }   // pattern of the obstacle hit

private:
    void checkCollisions();

    Player player_;
    HazardField hazards_;
    PatternSpawner spawner_;
    uint32_t timeMs_ = 0;          // game time, runs only while the world runs
    uint32_t passed_ = 0;          // obstacles that went past the car
    uint32_t coins_ = 0;
    uint32_t milestones_ = 0;
    float scroll_ = 0.0f;
    const char* crashCause_ = nullptr;
    const char* crashPattern_ = "";
};

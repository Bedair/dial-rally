#pragma once
// The game world during a run: car, obstacles, road scroll, score, crashes.
// GameController decides *when* the world runs (states); GameEngine decides
// *what* happens in it. DisplayManager only reads it.
//
// Each frame (while the world runs): obstacles move down, the PatternSpawner
// adds new ones, then collisions are checked against the car:
//   Roadblock  crash unless the car is fully inside the gap
//   Oil        crash unless the car is airborne (jump)
//   Traffic    crash on overlap
//   Coin       collected on overlap (scored in M4)
// Only solid (armed) obstacles collide; faint ones are a warning.

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
    uint32_t score() const { return score_; }
    uint32_t coins() const { return coins_; }
    // Distance driven in px, wrapped; the renderer scrolls road markings with it.
    float scroll() const { return scroll_; }

    bool crashed() const { return crashCause_ != nullptr; }
    const char* crashCause() const { return crashCause_; }   // for the log

private:
    void checkCollisions();

    Player player_;
    HazardField hazards_;
    PatternSpawner spawner_;
    uint32_t timeMs_ = 0;          // game time, runs only while the world runs
    uint32_t score_ = 0;
    uint32_t coins_ = 0;
    float scroll_ = 0.0f;
    const char* crashCause_ = nullptr;
};

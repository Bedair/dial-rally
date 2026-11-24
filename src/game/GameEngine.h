#pragma once
// The game world during a run: car, obstacles, road scroll and score.
// GameController decides *when* the world runs (states); GameEngine decides
// *what* happens in it. DisplayManager only reads it.
//
// M2: a demo spawner cycles roadblock, coin, oil spill, traffic at random
// positions so the renderer can be checked. No collisions yet (M3), and the
// score is a placeholder: +1 per obstacle that passes the car.

#include <stdint.h>
#include "game/Player.h"
#include "game/Hazard.h"

class GameEngine {
public:
    void reset();
    void steer(int32_t knobSteps);
    // world = false: only the car moves (Ready countdown).
    void update(uint32_t dtMs, bool world);

    const Player& player() const { return player_; }
    const HazardField& hazards() const { return hazards_; }
    uint32_t score() const { return score_; }
    // Distance driven in px, wrapped; the renderer scrolls road markings with it.
    float scroll() const { return scroll_; }

private:
    void demoSpawn();

    Player player_;
    HazardField hazards_;
    uint32_t score_ = 0;
    float scroll_ = 0.0f;
    uint32_t spawnTimerMs_ = 0;
    uint8_t demoIndex_ = 0;
};

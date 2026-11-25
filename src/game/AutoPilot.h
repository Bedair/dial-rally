#pragma once
// A simple driver that plays the game, used for validation only:
//   - the soak test firmware (input/AutoPilotSource) plays unattended for hours
//   - the host simulation (test/sim) checks fairness over many random runs
//
// It looks at the obstacle that reaches the car next and steers toward its
// safe position (roadblock gap centre, nearest free spot beside traffic),
// and jumps once for each oil spill shortly before it arrives.
// It never turns the knob faster than FAIR_STEPS_PER_S: if it survives,
// the patterns were reachable at the speed the fairness rule promises.
// Hardware-free: it only reads the GameEngine.

#include <stdint.h>
#include "game/GameEngine.h"

class AutoPilot {
public:
    struct Decision {
        int32_t steps = 0;     // knob steps to turn this frame
        bool jump = false;
    };

    void reset();
    Decision drive(const GameEngine& g, uint32_t dtMs);

private:
    float budget_ = 0;                      // knob steps allowed right now (rate limit)
    bool jumpedFor_[HazardField::MAX] = {}; // one jump per oil spill (by pool slot)
};

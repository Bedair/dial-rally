#pragma once
// Soak-test input: the AutoPilot plays the real game on the Dial, hours on end,
// through the same IInputSource interface as the knob. Used only in the
// "soak" build (MQ_SOAK=1, see platformio.ini and docs/TESTING.md).
//
//   Menu / GameOver  "press" every SOAK_PRESS_EVERY_MS (Play / Play again)
//   Playing          AutoPilot steers and jumps; after a random time per run
//                    (SOAK_GIVE_UP_*) it lets go, so the run ends in a crash
//                    and game over, restart and NVS get exercised too
//   Paused           "press" to resume (should not happen)

#include "input/Actions.h"
#include "game/AutoPilot.h"

class GameController;

class AutoPilotSource : public IInputSource {
public:
    // The pilot needs to see the game; call before begin().
    void attach(const GameController* game) { game_ = game; }

    bool begin() override;
    void update(uint32_t nowMs) override;
    bool poll(ActionEvent& out) override;
    const char* name() const override { return "AutoPilot"; }

private:
    const GameController* game_ = nullptr;
    AutoPilot pilot_;
    uint32_t lastMs_ = 0;
    uint32_t lastPressMs_ = 0;
    uint32_t giveUpMs_ = 0;      // game time at which this run's pilot lets go
    bool inRun_ = false;

    int32_t pendingSteps_ = 0;
    bool pendingPress_ = false;
    uint32_t stampMs_ = 0;
};

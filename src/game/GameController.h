#pragma once
// Game state machine: Menu -> Ready (3-2-1) -> Playing <-> Paused -> GameOver.
//
// Input (from IInputSource only, never hardware):
//   Menu      knob = choose, press = confirm ("Play" / "High score")
//   Ready     knob steers the car, countdown runs on its own
//   Playing   knob steers the car, press = jump, hold = pause; a crash ends the run
//   Paused    knob = choose, press = confirm ("Resume" / "End run")
//   GameOver  knob = choose, press = confirm ("Play again" / "Menu"),
//             presses ignored for GAMEOVER_INPUT_DELAY_MS after the crash
//
// A run ends on a crash or via Paused -> "End run".
// High score is kept in RAM until NVS storage (M4).

#include <stdint.h>
#include "input/Actions.h"
#include "display/DisplayManager.h"
#include "game/GameEngine.h"

enum class GameState : uint8_t { Menu, Ready, Playing, Paused, GameOver };

const char* stateName(GameState s);

class GameController {
public:
    GameController(IInputSource& input, DisplayManager& display)
        : input_(input), display_(display) {}

    void begin(uint32_t nowMs);
    void update(uint32_t nowMs, uint32_t dtMs);   // input + game logic
    void render(uint32_t nowMs);                  // draw the current state (no push)

    GameState state() const { return state_; }

private:
    void handle(const ActionEvent& ev, uint32_t nowMs);
    void enter(GameState s, uint32_t nowMs);
    void endRun(uint32_t nowMs);
    // Knob steps -> menu selection, with MENU_STEPS_PER_ITEM steps per item.
    int scroll(int32_t steps, int selected, int count);

    IInputSource& input_;
    DisplayManager& display_;
    GameEngine engine_;

    GameState state_ = GameState::Menu;
    uint32_t stateSinceMs_ = 0;

    int selected_ = 0;            // highlighted item in Menu / Paused / GameOver
    int32_t scrollAccum_ = 0;     // knob steps not yet worth a menu move
    bool showBest_ = false;       // Menu sub-screen "High score"

    uint32_t best_ = 0;
    bool newBest_ = false;
};

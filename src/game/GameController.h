#pragma once
// Game state machine: Menu -> Ready (3-2-1) -> Playing <-> Paused -> GameOver.
//
// Input (from IInputSource only, never hardware):
//   Menu      knob = choose, press = confirm ("Play" / "High score")
//   Ready     knob steers the car, countdown runs on its own
//   Playing   knob steers the car, press = jump, hold = pause; a crash ends the run
//   Paused    knob = choose, press = confirm ("Resume" / "End run")
//   GameOver  after a crash, the explosion plays first (CRASH_ANIM_MS); then
//             knob = choose, press = confirm ("Play again" / "Menu"), with
//             presses ignored for GAMEOVER_INPUT_DELAY_MS
//
// A run ends on a crash or via Paused -> "End run".
// The high score lives in NVS (ScoreManager) and survives power-off.
// Sounds (AudioManager) and visual effects (DisplayManager::fx*) are
// triggered here from the engine's events, so the engine stays hardware-free.

#include <stdint.h>
#include "input/Actions.h"
#include "display/DisplayManager.h"
#include "game/GameEngine.h"
#include "storage/ScoreManager.h"
#include "audio/AudioManager.h"

enum class GameState : uint8_t { Menu, Ready, Playing, Paused, GameOver };

const char* stateName(GameState s);

class GameController {
public:
    GameController(IInputSource& input, DisplayManager& display, ScoreManager& scores,
                   AudioManager& audio)
        : input_(input), display_(display), scores_(scores), audio_(audio) {}

    void begin(uint32_t nowMs);
    void update(uint32_t nowMs, uint32_t dtMs);   // input + game logic + sound
    void render(uint32_t nowMs);                  // draw the current state (no push)

    GameState state() const { return state_; }
    const GameEngine& engine() const { return engine_; }   // read-only (autopilot, stats)
    uint32_t runs() const { return runs_; }                 // runs finished since boot

private:
    void handle(const ActionEvent& ev, uint32_t nowMs);
    void enter(GameState s, uint32_t nowMs);
    void endRun(uint32_t nowMs);
    void feedback(uint8_t events, uint32_t nowMs);
    uint32_t panelDelayMs() const;
    // Knob steps -> menu selection, with MENU_STEPS_PER_ITEM steps per item.
    int scroll(int32_t steps, int selected, int count, uint32_t nowMs);

    IInputSource& input_;
    DisplayManager& display_;
    ScoreManager& scores_;
    AudioManager& audio_;
    GameEngine engine_;

    GameState state_ = GameState::Menu;
    uint32_t stateSinceMs_ = 0;

    int selected_ = 0;            // highlighted item in Menu / Paused / GameOver
    int32_t scrollAccum_ = 0;     // knob steps not yet worth a menu move
    bool showBest_ = false;       // Menu sub-screen "High score"
    int readyCount_ = 3;          // countdown digit shown (and beeped)

    bool newBest_ = false;        // last run set a new high score
    bool fanfareDone_ = false;
    uint32_t runs_ = 0;
};

#include "input/AutoPilotSource.h"
#include "game/GameController.h"
#include "config/GameConfig.h"
#include <Arduino.h>

bool AutoPilotSource::begin() {
    Serial.println("[MQ] SOAK TEST: autopilot is playing (knob ignored)");
    return game_ != nullptr;
}

void AutoPilotSource::update(uint32_t nowMs) {
    const uint32_t dt = lastMs_ ? nowMs - lastMs_ : 0;
    lastMs_ = stampMs_ = nowMs;
    if (!game_) return;

    switch (game_->state()) {
        case GameState::Menu:
        case GameState::GameOver:
        case GameState::Paused:
            inRun_ = false;
            if (nowMs - lastPressMs_ >= cfg::SOAK_PRESS_EVERY_MS) {
                pendingPress_ = true;
                lastPressMs_ = nowMs;
            }
            break;

        case GameState::Ready:
            if (!inRun_) {               // new run: fresh pilot, new give-up time
                inRun_ = true;
                pilot_.reset();
                giveUpMs_ = random(cfg::SOAK_GIVE_UP_MIN_MS, cfg::SOAK_GIVE_UP_MAX_MS);
            }
            break;

        case GameState::Playing: {
            const GameEngine& g = game_->engine();
            if (g.timeMs() >= giveUpMs_) break;          // hands off: crash soon
            const AutoPilot::Decision d = pilot_.drive(g, dt);
            pendingSteps_ += d.steps;
            if (d.jump) pendingPress_ = true;
            break;
        }
    }
}

bool AutoPilotSource::poll(ActionEvent& out) {
    out = ActionEvent{};
    out.timestampMs = stampMs_;
    if (pendingPress_) {
        pendingPress_ = false;
        out.action = Action::Press;
        out.origin = Origin::Button;
        return true;
    }
    if (pendingSteps_ != 0) {
        out.action = Action::Turn;
        out.origin = Origin::Knob;
        out.steps = pendingSteps_;
        pendingSteps_ = 0;
        return true;
    }
    return false;
}

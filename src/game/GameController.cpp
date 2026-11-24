#include "game/GameController.h"
#include "config/GameConfig.h"
#include <Arduino.h>

const char* stateName(GameState s) {
    switch (s) {
        case GameState::Menu:     return "Menu";
        case GameState::Ready:    return "Ready";
        case GameState::Playing:  return "Playing";
        case GameState::Paused:   return "Paused";
        case GameState::GameOver: return "GameOver";
    }
    return "?";
}

void GameController::begin(uint32_t nowMs) {
    enter(GameState::Menu, nowMs);
}

// ---------------------------------------------------------------------------
// Transitions
// ---------------------------------------------------------------------------
void GameController::enter(GameState s, uint32_t nowMs) {
    if (cfg::DEBUG_LOG) Serial.printf("[MQ] state %s -> %s\n", stateName(state_), stateName(s));
    state_ = s;
    stateSinceMs_ = nowMs;
    selected_ = 0;
    scrollAccum_ = 0;

    switch (s) {
        case GameState::Menu:
            showBest_ = false;
            break;
        case GameState::Ready:            // new run
            player_.reset(0.0f);
            playMs_ = 0;
            score_ = 0;
            newBest_ = false;
            break;
        default:
            break;
    }
}

void GameController::endRun(uint32_t nowMs) {
    newBest_ = score_ > best_;
    if (newBest_) best_ = score_;
    Serial.printf("[MQ] run over: score %lu, best %lu%s\n", (unsigned long)score_,
                  (unsigned long)best_, newBest_ ? " (new)" : "");
    enter(GameState::GameOver, nowMs);
}

int GameController::scroll(int32_t steps, int selected, int count) {
    scrollAccum_ += steps;
    while (scrollAccum_ >= cfg::MENU_STEPS_PER_ITEM) {
        scrollAccum_ -= cfg::MENU_STEPS_PER_ITEM;
        selected++;
    }
    while (scrollAccum_ <= -cfg::MENU_STEPS_PER_ITEM) {
        scrollAccum_ += cfg::MENU_STEPS_PER_ITEM;
        selected--;
    }
    // Clamp (no wrap-around) and drop leftover turning at the ends, so turning
    // back from an end responds immediately.
    if (selected < 0)          { selected = 0;         scrollAccum_ = 0; }
    if (selected > count - 1)  { selected = count - 1; scrollAccum_ = 0; }
    return selected;
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void GameController::handle(const ActionEvent& ev, uint32_t nowMs) {
    switch (state_) {
        case GameState::Menu:
            if (showBest_) {
                if (ev.action == Action::Press) showBest_ = false;
            } else if (ev.action == Action::Turn) {
                selected_ = scroll(ev.steps, selected_, 2);
            } else if (ev.action == Action::Press) {
                if (selected_ == 0) enter(GameState::Ready, nowMs);
                else                showBest_ = true;
            }
            break;

        case GameState::Ready:
            if (ev.action == Action::Turn) player_.turn(ev.angleDeg);
            break;

        case GameState::Playing:
            if (ev.action == Action::Turn) {
                player_.turn(ev.angleDeg);
            } else if (ev.action == Action::Press) {
                if (cfg::DEBUG_LOG) Serial.printf("[MQ] jump (%s) - mechanics in M3\n",
                                                  originName(ev.origin));
            } else if (ev.action == Action::LongPress) {
                enter(GameState::Paused, nowMs);
            }
            break;

        case GameState::Paused:
            if (ev.action == Action::Turn) {
                selected_ = scroll(ev.steps, selected_, 2);
            } else if (ev.action == Action::Press) {
                if (selected_ == 0) enter(GameState::Playing, nowMs);
                else                endRun(nowMs);
            }
            break;

        case GameState::GameOver:
            if (ev.action == Action::Turn) {
                selected_ = scroll(ev.steps, selected_, 2);
            } else if (ev.action == Action::Press &&
                       nowMs - stateSinceMs_ >= cfg::GAMEOVER_INPUT_DELAY_MS) {
                enter(selected_ == 0 ? GameState::Ready : GameState::Menu, nowMs);
            }
            break;
    }
}

// ---------------------------------------------------------------------------
// Per frame
// ---------------------------------------------------------------------------
void GameController::update(uint32_t nowMs, uint32_t dtMs) {
    input_.update(nowMs);
    ActionEvent ev;
    while (input_.poll(ev)) handle(ev, nowMs);

    switch (state_) {
        case GameState::Ready:
            player_.update(dtMs);
            if (nowMs - stateSinceMs_ >= 3 * cfg::READY_STEP_MS) enter(GameState::Playing, nowMs);
            break;
        case GameState::Playing:
            player_.update(dtMs);
            playMs_ += dtMs;
            score_ = playMs_ / 1000;   // M1 placeholder until hazards (M3) score points
            break;
        default:
            break;   // Menu, Paused, GameOver: the world is frozen
    }
}

void GameController::render(uint32_t nowMs) {
    switch (state_) {
        case GameState::Menu:
            if (showBest_) display_.drawBest(best_);
            else           display_.drawMenu(selected_);
            break;
        case GameState::Ready: {
            const int count = 3 - (int)((nowMs - stateSinceMs_) / cfg::READY_STEP_MS);
            display_.drawReady(player_, score_, count < 1 ? 1 : count);
            break;
        }
        case GameState::Playing:
            display_.drawPlaying(player_, score_);
            break;
        case GameState::Paused:
            display_.drawPaused(player_, score_, selected_);
            break;
        case GameState::GameOver:
            display_.drawGameOver(score_, best_, newBest_, selected_);
            break;
    }
}

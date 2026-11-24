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
            engine_.reset();
            newBest_ = false;
            break;
        default:
            break;
    }
}

void GameController::endRun(uint32_t nowMs) {
    const uint32_t score = engine_.score();
    newBest_ = score > best_;
    if (newBest_) best_ = score;
    Serial.printf("[MQ] run over: score %lu, coins %lu, best %lu%s\n", (unsigned long)score,
                  (unsigned long)engine_.coins(), (unsigned long)best_, newBest_ ? " (new)" : "");
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
            if (ev.action == Action::Turn) engine_.steer(ev.steps);
            break;

        case GameState::Playing:
            if (ev.action == Action::Turn) {
                engine_.steer(ev.steps);
            } else if (ev.action == Action::Press) {
                engine_.jump();
                if (cfg::DEBUG_LOG) Serial.printf("[MQ] jump (%s)\n", originName(ev.origin));
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
            engine_.update(dtMs, false);   // car steers, road waits
            if (nowMs - stateSinceMs_ >= 3 * cfg::READY_STEP_MS) enter(GameState::Playing, nowMs);
            break;
        case GameState::Playing:
            engine_.update(dtMs, true);
            if (engine_.crashed()) {
                Serial.printf("[MQ] crash: %s\n", engine_.crashCause());
                endRun(nowMs);
            }
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
            display_.drawReady(engine_, count < 1 ? 1 : count);
            break;
        }
        case GameState::Playing:
            display_.drawPlaying(engine_);
            break;
        case GameState::Paused:
            display_.drawPaused(engine_, selected_);
            break;
        case GameState::GameOver:
            display_.drawGameOver(engine_.score(), best_, newBest_, selected_);
            break;
    }
}

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
    const GameState from = state_;
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
            readyCount_ = 3;
            audio_.play(Sfx::Count, nowMs);
            break;
        case GameState::Playing:
            if (from == GameState::Ready) audio_.play(Sfx::Go, nowMs);
            break;
        case GameState::GameOver:
            fanfareDone_ = false;
            break;
        default:
            break;
    }
}

void GameController::endRun(uint32_t nowMs) {
    const uint32_t score = engine_.score();
    newBest_ = scores_.submit(score);
    runs_++;
    Serial.printf("[MQ] run %lu over: score %lu (passed %lu, coins %lu, milestones %lu) in %.1f s, "
                  "best %lu%s\n",
                  (unsigned long)runs_, (unsigned long)score, (unsigned long)engine_.passed(),
                  (unsigned long)engine_.coins(), (unsigned long)engine_.milestones(),
                  engine_.timeMs() / 1000.0f, (unsigned long)scores_.best(),
                  newBest_ ? " (new)" : "");
    enter(GameState::GameOver, nowMs);
}

// After a crash the game-over panel waits for the explosion (CRASH_ANIM_MS).
uint32_t GameController::panelDelayMs() const {
    return engine_.crashed() ? cfg::CRASH_ANIM_MS : 0;
}

int GameController::scroll(int32_t steps, int selected, int count, uint32_t nowMs) {
    const int before = selected;
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
    if (selected != before) audio_.play(Sfx::Tick, nowMs);
    return selected;
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void GameController::handle(const ActionEvent& ev, uint32_t nowMs) {
    switch (state_) {
        case GameState::Menu:
            if (showBest_) {
                if (ev.action == Action::Press) {
                    showBest_ = false;
                    audio_.play(Sfx::Confirm, nowMs);
                }
            } else if (ev.action == Action::Turn) {
                selected_ = scroll(ev.steps, selected_, 2, nowMs);
            } else if (ev.action == Action::Press) {
                audio_.play(Sfx::Confirm, nowMs);
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
                engine_.jump();       // the sound plays on take-off (EV_JUMP)
                if (cfg::VERBOSE_LOG) Serial.printf("[MQ] jump (%s)\n", originName(ev.origin));
            } else if (ev.action == Action::LongPress) {
                audio_.play(Sfx::Confirm, nowMs);
                enter(GameState::Paused, nowMs);
            }
            break;

        case GameState::Paused:
            if (ev.action == Action::Turn) {
                selected_ = scroll(ev.steps, selected_, 2, nowMs);
            } else if (ev.action == Action::Press) {
                audio_.play(Sfx::Confirm, nowMs);
                if (selected_ == 0) enter(GameState::Playing, nowMs);
                else                endRun(nowMs);
            }
            break;

        case GameState::GameOver: {
            const uint32_t since = nowMs - stateSinceMs_;
            if (since < panelDelayMs()) break;                 // explosion still playing
            if (ev.action == Action::Turn) {
                selected_ = scroll(ev.steps, selected_, 2, nowMs);
            } else if (ev.action == Action::Press &&
                       since >= panelDelayMs() + cfg::GAMEOVER_INPUT_DELAY_MS) {
                audio_.play(Sfx::Confirm, nowMs);
                enter(selected_ == 0 ? GameState::Ready : GameState::Menu, nowMs);
            }
            break;
        }
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
        case GameState::Ready: {
            engine_.update(dtMs, false);   // car steers, road waits
            const uint32_t since = nowMs - stateSinceMs_;
            const int count = 3 - (int)(since / cfg::READY_STEP_MS);
            if (count != readyCount_ && count >= 1) {
                readyCount_ = count;
                audio_.play(Sfx::Count, nowMs);
            }
            if (since >= 3 * cfg::READY_STEP_MS) enter(GameState::Playing, nowMs);
            break;
        }
        case GameState::Playing:
            engine_.update(dtMs, true);
            feedback(engine_.takeEvents(), nowMs);
            if (engine_.crashed()) {
                Serial.printf("[MQ] crash: %s (%s) at %.0f px/s\n", engine_.crashCause(),
                              engine_.crashPattern(), engine_.roadSpeed());
                endRun(nowMs);
            }
            break;
        case GameState::GameOver:
            // Fanfare once the results panel shows a new high score.
            if (newBest_ && !fanfareDone_ && nowMs - stateSinceMs_ >= panelDelayMs()) {
                fanfareDone_ = true;
                audio_.play(Sfx::HighScore, nowMs);
            }
            break;
        default:
            break;   // Menu, Paused: the world is frozen
    }
    audio_.update(nowMs);
}

// Sounds and visual effects for what happened in the engine this frame.
void GameController::feedback(uint8_t events, uint32_t nowMs) {
    if (events & EV_JUMP) audio_.play(Sfx::Jump, nowMs);
    if (events & EV_COIN) {
        audio_.play(Sfx::Coin, nowMs);
        display_.fxCoin(engine_.player().drawX(), nowMs);
    }
    if (events & EV_MILESTONE) {
        audio_.play(Sfx::Milestone, nowMs);
        display_.fxMilestone(nowMs);
    }
    if (events & EV_CRASH) audio_.play(Sfx::Crash, nowMs);
}

void GameController::render(uint32_t nowMs) {
    switch (state_) {
        case GameState::Menu:
            if (showBest_) display_.drawBest(scores_.best());
            else           display_.drawMenu(selected_);
            break;
        case GameState::Ready:
            display_.drawReady(engine_, readyCount_);
            break;
        case GameState::Playing:
            display_.drawPlaying(engine_, nowMs);
            break;
        case GameState::Paused:
            display_.drawPaused(engine_, selected_);
            break;
        case GameState::GameOver: {
            const uint32_t since = nowMs - stateSinceMs_;
            if (since < panelDelayMs()) display_.drawCrash(engine_, since);
            else display_.drawGameOver(engine_, scores_.best(), newBest_, selected_, nowMs);
            break;
        }
    }
}

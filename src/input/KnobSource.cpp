#include "input/KnobSource.h"
#include "config/GameConfig.h"
#include <M5Dial.h>

bool KnobSource::begin() {
    // M5Dial.begin(cfg, true, ...) has already started the encoder interrupts.
    M5Dial.BtnA.setHoldThresh(cfg::KNOB_HOLD_MS);
    lastRaw_ = M5Dial.Encoder.read();
    return true;
}

void KnobSource::update(uint32_t nowMs) {
    stampMs_ = nowMs;

    // --- Encoder -----------------------------------------------------------
    const int32_t raw = M5Dial.Encoder.read();
    int32_t delta = raw - lastRaw_;
    lastRaw_ = raw;
    if (cfg::KNOB_REVERSE) delta = -delta;

    if (delta != 0) {
        const int8_t dir = delta > 0 ? 1 : -1;
        const bool recent = nowMs - lastMoveMs_ < cfg::KNOB_GLITCH_WINDOW_MS;
        if ((delta == 2 || delta == -2) && recent && lastDir_ != 0 && dir != lastDir_) {
            delta = -delta;   // driver guessed the direction wrong
            glitches_++;
            if (cfg::DEBUG_LOG) Serial.printf("[MQ] knob: corrected %+ld glitch (#%lu)\n",
                                              (long)-delta, (unsigned long)glitches_);
        } else {
            lastDir_ = dir;
        }
        lastMoveMs_ = nowMs;
    }
    rawCount_ += delta;

    carry_ += delta;
    const int32_t steps = carry_ / cfg::ENCODER_COUNTS_PER_STEP;   // truncates toward 0
    carry_ -= steps * cfg::ENCODER_COUNTS_PER_STEP;
    pendingSteps_ += steps;

    // --- Button / touch ----------------------------------------------------
    if (M5Dial.BtnA.wasPressed()) {
        pendingPress_ = true;
        pressOrigin_ = Origin::Button;
    } else if (M5Dial.Touch.getCount() > 0 && M5Dial.Touch.getDetail().wasPressed()) {
        pendingPress_ = true;
        pressOrigin_ = Origin::Touch;
    }
    if (M5Dial.BtnA.wasHold()) pendingHold_ = true;
}

bool KnobSource::poll(ActionEvent& out) {
    out = ActionEvent{};
    out.timestampMs = stampMs_;

    // Press first: it is the time-critical one (jump).
    if (pendingPress_) {
        pendingPress_ = false;
        out.action = Action::Press;
        out.origin = pressOrigin_;
        return true;
    }
    if (pendingSteps_ != 0) {
        out.action = Action::Turn;
        out.origin = Origin::Knob;
        out.steps = pendingSteps_;
        pendingSteps_ = 0;
        return true;
    }
    if (pendingHold_) {
        pendingHold_ = false;
        out.action = Action::LongPress;
        out.origin = Origin::Button;
        return true;
    }
    return false;
}

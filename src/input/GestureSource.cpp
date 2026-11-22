#include "input/GestureSource.h"
#include "config/GameConfig.h"
#include <Arduino.h>
#include <Wire.h>

bool GestureSource::begin() {
    // Wire is started on PORT.A pins by main.cpp before this call.
    // The PAJ7620 sleeps after power-up and may miss the first transfers,
    // so retry a few times before giving up. Library returns 1 on success.
    bool ok = false;
    for (int attempt = 1; attempt <= 5 && !ok; attempt++) {
        ok = sensor_.begin(&Wire) != 0;
        if (!ok) {
            Serial.printf("[MQ] gesture init attempt %d failed\n", attempt);
            delay(100);
        }
    }
    if (!ok) return false;
    sensor_.setGestureEntryTime(0);                 // no extra delay before reporting
    if (cfg::GESTURE_GAME_SPEED) sensor_.setGameSpeed();
    else                         sensor_.setNormalSpeed();
    return true;
}

bool GestureSource::poll(ActionEvent& out) {
    const uint32_t now = millis();
    if (now - lastPollMs_ < cfg::GESTURE_POLL_MS) return false;
    lastPollMs_ = now;

    lastRaw_ = sensor_.readGesture();
    if (lastRaw_ == GES_NONE) return false;

    Gesture g = lastRaw_;
    if (cfg::GESTURE_SWAP_UP_DOWN) {
        if (g == GES_UP)        g = GES_DOWN;
        else if (g == GES_DOWN) g = GES_UP;
    }

    switch (g) {
        case GES_UP:   out.action = Action::Jump; break;
        case GES_DOWN: out.action = Action::Duck; break;
        default:       out.action = Action::None; break;   // other gestures ignored by the game
    }
    out.timestampMs = now;
    return true;   // also true for ignored gestures, so the test can log them
}

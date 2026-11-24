#include "storage/ScoreManager.h"
#include <Arduino.h>
#include <Preferences.h>

namespace {
constexpr const char* NVS_NAMESPACE = "dialrally";
constexpr const char* KEY_BEST = "best";
}

void ScoreManager::begin() {
    Preferences prefs;
    if (prefs.begin(NVS_NAMESPACE, true)) {      // read-only
        best_ = prefs.getUInt(KEY_BEST, 0);
        prefs.end();
    }
    Serial.printf("[MQ] high score loaded: %lu\n", (unsigned long)best_);
}

bool ScoreManager::submit(uint32_t score) {
    if (score <= best_) return false;
    best_ = score;
    Preferences prefs;
    if (prefs.begin(NVS_NAMESPACE, false)) {
        prefs.putUInt(KEY_BEST, best_);
        prefs.end();
        Serial.printf("[MQ] high score saved: %lu\n", (unsigned long)best_);
    } else {
        Serial.println("[MQ] high score NOT saved: NVS unavailable");
    }
    return true;
}

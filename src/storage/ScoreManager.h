#pragma once
// High score in NVS (the ESP32's key-value flash storage) via Preferences.
// Survives power-off and re-flashing (NVS is not erased by a normal upload).
// Written only when a run beats the stored best, so flash wear is negligible.

#include <stdint.h>

class ScoreManager {
public:
    void begin();                       // load the stored best (0 if none)
    uint32_t best() const { return best_; }
    // Saves score if it beats the best. Returns true if it was a new best.
    bool submit(uint32_t score);

private:
    uint32_t best_ = 0;
};

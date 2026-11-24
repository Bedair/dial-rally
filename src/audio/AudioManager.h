#pragma once
// Buzzer sound effects, non-blocking.
//
// Each effect is a short sequence of notes. play() starts it; update(), called
// once per frame, starts the next note when the previous one is due. Nothing
// waits: the speaker plays a note in the background while the game runs.
//
// One effect at a time. A new effect replaces the current one only if its
// priority is at least as high, so a crash is never cut off by a coin.

#include <stdint.h>

enum class Sfx : uint8_t {
    Tick,        // menu selection moved
    Confirm,     // menu press
    Count,       // 3-2-1
    Go,          // race starts
    Jump,
    Coin,
    Milestone,   // +50 bonus
    Crash,
    HighScore,   // new best on the game-over screen
};

class AudioManager {
public:
    void begin();
    void play(Sfx s, uint32_t nowMs);
    void update(uint32_t nowMs);

    struct Note { uint16_t hz; uint16_t ms; };   // hz = 0: rest

private:
    const Note* seq_ = nullptr;
    uint8_t len_ = 0, idx_ = 0;
    uint8_t prio_ = 0;
    uint32_t nextMs_ = 0;
};

#pragma once
// Abstract player actions. The game only ever sees these, never raw hardware,
// so the input source can be swapped (or faked in tests) without touching the game.

#include <stdint.h>

enum class Action : uint8_t {
    None,
    Turn,        // knob moved: see ActionEvent::steps
    Press,       // BtnA or touch tap: Jump while playing, Confirm in menus
    LongPress,   // BtnA held (diagnostics / later: reset high score)
};

// Where an action came from (for logs; the game should not care).
enum class Origin : uint8_t { Knob, Button, Touch };

struct ActionEvent {
    Action action = Action::None;
    Origin origin = Origin::Knob;
    uint32_t timestampMs = 0;   // millis() when the source reported it
    int32_t steps = 0;          // Turn only: signed encoder steps since the last Turn
                                // (+ = clockwise); the game decides what a step means
};

inline const char* actionName(Action a) {
    switch (a) {
        case Action::Turn:      return "TURN";
        case Action::Press:     return "PRESS";
        case Action::LongPress: return "HOLD";
        default:                return "NONE";
    }
}

inline const char* originName(Origin o) {
    switch (o) {
        case Origin::Button: return "button";
        case Origin::Touch:  return "touch";
        default:             return "knob";
    }
}

// Interface every input source implements.
class IInputSource {
public:
    virtual ~IInputSource() = default;
    virtual bool begin() = 0;
    // Called once per frame, after M5Dial.update(): sample the hardware.
    virtual void update(uint32_t nowMs) = 0;
    // Drain events gathered by update(). Returns true and fills `out` while
    // events are pending; call in a loop until it returns false.
    virtual bool poll(ActionEvent& out) = 0;
    virtual const char* name() const = 0;
};

#pragma once
// Abstract player actions. The game only ever sees these, never raw sensor data,
// so the input source (gesture sensor, IMU, buttons) can be swapped freely.

#include <stdint.h>

enum class Action : uint8_t {
    None,
    Jump,
    Duck,
    MenuNext,
    MenuPrev,
    Confirm,
};

struct ActionEvent {
    Action action = Action::None;
    uint32_t timestampMs = 0;   // millis() when the source reported it
};

inline const char* actionName(Action a) {
    switch (a) {
        case Action::Jump:     return "JUMP";
        case Action::Duck:     return "DUCK";
        case Action::MenuNext: return "NEXT";
        case Action::MenuPrev: return "PREV";
        case Action::Confirm:  return "OK";
        default:               return "NONE";
    }
}

// Interface every input source implements.
class IInputSource {
public:
    virtual ~IInputSource() = default;
    virtual bool begin() = 0;
    // Returns true and fills `out` when a new action is available.
    virtual bool poll(ActionEvent& out) = 0;
    virtual const char* name() const = 0;
};

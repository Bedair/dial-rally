#pragma once
// PAJ7620U2 gesture sensor (M5Stack Gesture Unit) as an input source.
// Polled over I2C: the Grove connector does not expose the INT line.

#include "input/Actions.h"
#include <RevEng_PAJ7620.h>

class GestureSource : public IInputSource {
public:
    bool begin() override;
    bool poll(ActionEvent& out) override;
    const char* name() const override { return "PAJ7620"; }

    // Raw gesture of the last poll, for diagnostics (Day 0 test).
    Gesture lastRaw() const { return lastRaw_; }

private:
    RevEng_PAJ7620 sensor_;
    uint32_t lastPollMs_ = 0;
    Gesture lastRaw_ = GES_NONE;
};

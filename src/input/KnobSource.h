#pragma once
// The Dial itself as an input source:
//   rotary encoder -> Turn (signed steps)
//   BtnA press     -> Press (origin Button), fires on press-down for low latency
//   touch tap      -> Press (origin Touch), alternative jump
//   BtnA hold      -> LongPress (after KNOB_HOLD_MS; a Press was already sent)
//
// Encoder counts are grouped into "steps" (ENCODER_COUNTS_PER_STEP); leftover
// counts are carried, so slow turning never loses movement. A wrong-direction
// ±2 from the driver is corrected first (see KNOB_GLITCH_WINDOW_MS).

#include "input/Actions.h"

class KnobSource : public IInputSource {
public:
    bool begin() override;
    void update(uint32_t nowMs) override;
    bool poll(ActionEvent& out) override;
    const char* name() const override { return "Knob"; }

    // Diagnostics: total encoder counts since begin() (raw, before grouping).
    int32_t rawCount() const { return rawCount_; }
    // Diagnostics: how many ±2 glitches were corrected since begin().
    uint32_t glitchesFixed() const { return glitches_; }

private:
    int32_t lastRaw_ = 0;      // encoder reading at the previous update()
    int32_t rawCount_ = 0;
    int32_t carry_ = 0;        // counts not yet worth a full step
    int8_t lastDir_ = 0;       // sign of the last real movement
    uint32_t lastMoveMs_ = 0;
    uint32_t glitches_ = 0;

    // Pending events for poll(). At most one of each kind per frame is enough:
    // turns are summed, presses within one frame collapse to one.
    int32_t pendingSteps_ = 0;
    bool pendingPress_ = false;
    Origin pressOrigin_ = Origin::Button;
    bool pendingHold_ = false;
    uint32_t stampMs_ = 0;
};

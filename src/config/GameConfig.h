#pragma once
// The Lost Temple – central configuration.
// Every tunable number lives here so hardware tests only need one file edited.

#include <stdint.h>

namespace cfg {

// ---------------------------------------------------------------------------
// Knob (rotary encoder) and button
// ---------------------------------------------------------------------------
// Raw quadrature counts grouped into one "step". Set after the M0 measurement:
// if one detent (click) gives N counts, set this to N so 1 step = 1 click.
constexpr int32_t ENCODER_COUNTS_PER_STEP = 1;

// Measured in M0 (hold BtnA, turn one full turn, hold again). Placeholder.
constexpr int32_t ENCODER_STEPS_PER_TURN = 64;

// How far the explorer moves around the rim per step, in degrees.
// 360 / ENCODER_STEPS_PER_TURN = one knob turn moves the explorer one lap.
constexpr float KNOB_DEG_PER_STEP = 360.0f / ENCODER_STEPS_PER_TURN;

// true flips the direction if clockwise knob turns move the explorer
// counter-clockwise on screen.
constexpr bool KNOB_REVERSE = false;

constexpr uint32_t KNOB_HOLD_MS = 1000;   // BtnA held this long = LongPress

// ---------------------------------------------------------------------------
// Explorer movement
// ---------------------------------------------------------------------------
// The drawn position eases toward the target angle. Time constant in ms:
// after EASE_TAU_MS the drawn angle has covered ~63 % of the remaining gap.
// Smaller = snappier, larger = smoother. Frame-rate independent.
constexpr float EASE_TAU_MS = 35.0f;

// ---------------------------------------------------------------------------
// Screen geometry (240×240 round display)
// ---------------------------------------------------------------------------
constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 240;
constexpr int CENTER_X = 120;
constexpr int CENTER_Y = 120;
constexpr int RIM_RADIUS = 106;      // centre line of the rim track
constexpr int RIM_WIDTH = 14;        // track thickness
constexpr int ALTAR_RADIUS = 38;     // centre circle
constexpr int EXPLORER_RADIUS = 7;   // M0 test dot

// ---------------------------------------------------------------------------
// Frame loop
// ---------------------------------------------------------------------------
constexpr uint32_t FRAME_MS = 16;    // ~60 fps cap; game target is >= 30 fps

// ---------------------------------------------------------------------------
// M0 test feedback
// ---------------------------------------------------------------------------
constexpr uint32_t FLASH_MS = 150;
constexpr uint16_t BEEP_PRESS_HZ = 2400;
constexpr uint16_t BEEP_TOUCH_HZ = 1800;
constexpr uint16_t BEEP_HOLD_HZ = 900;
constexpr uint32_t BEEP_MS = 40;
constexpr uint8_t SPEAKER_VOLUME = 128;   // 0..255

}  // namespace cfg

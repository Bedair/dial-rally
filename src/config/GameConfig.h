#pragma once
// The Lost Temple – central configuration.
// Every tunable number lives here so hardware tests only need one file edited.

#include <stdint.h>

namespace cfg {

// ---------------------------------------------------------------------------
// Debug
// ---------------------------------------------------------------------------
constexpr bool DEBUG_LOG = true;             // frame-time stats + state changes on serial
constexpr uint32_t FRAME_STATS_MS = 5000;    // frame-time log interval

// ---------------------------------------------------------------------------
// Knob (rotary encoder) and button
// ---------------------------------------------------------------------------
// Raw quadrature counts grouped into one "step". M0 logs: 1 count per detent.
constexpr int32_t ENCODER_COUNTS_PER_STEP = 1;

// Steps per full knob turn. M0 logs are consistent with 64, but the hold
// measurement is still open (see M0 notes).
constexpr int32_t ENCODER_STEPS_PER_TURN = 64;

// How far the explorer moves around the rim per step, in degrees.
// 360 / ENCODER_STEPS_PER_TURN = one knob turn moves the explorer one lap.
constexpr float KNOB_DEG_PER_STEP = 360.0f / ENCODER_STEPS_PER_TURN;

// true flips the direction if clockwise knob turns move the explorer
// counter-clockwise on screen.
constexpr bool KNOB_REVERSE = false;

// BtnA held this long = LongPress = pause while playing.
// (The press-down before it still counts as a Press / jump.)
constexpr uint32_t KNOB_HOLD_MS = 600;

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
constexpr int EXPLORER_RADIUS = 7;   // placeholder dot until the sprite (M2/M6)

// ---------------------------------------------------------------------------
// Frame loop
// ---------------------------------------------------------------------------
constexpr uint32_t FRAME_MS = 16;    // ~60 fps cap; game target is >= 30 fps

// ---------------------------------------------------------------------------
// Game flow
// ---------------------------------------------------------------------------
constexpr int32_t MENU_STEPS_PER_ITEM = 3;          // knob steps to move one menu item
constexpr uint32_t READY_STEP_MS = 700;             // each of 3-2-1
constexpr uint32_t GAMEOVER_INPUT_DELAY_MS = 600;   // ignore presses right after a crash

}  // namespace cfg

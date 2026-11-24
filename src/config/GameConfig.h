#pragma once
// Dial Rally – central configuration.
// Every tunable number lives here so hardware tests only need one file edited.
//
// Coordinates: screen pixels, x to the right, y down, (0,0) top-left of the
// 240×240 round display. The road runs top to bottom; obstacles move down.

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

// true flips the direction if turning the knob clockwise steers left.
constexpr bool KNOB_REVERSE = false;

// BtnA held this long = LongPress = pause while playing.
// (The press-down before it still counts as a Press / jump.)
constexpr uint32_t KNOB_HOLD_MS = 600;

// ---------------------------------------------------------------------------
// Screen
// ---------------------------------------------------------------------------
constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 240;
constexpr int CENTER_X = 120;
constexpr int CENTER_Y = 120;

// ---------------------------------------------------------------------------
// Road
// ---------------------------------------------------------------------------
constexpr int ROAD_LEFT = 60;                // asphalt from x = ROAD_LEFT ...
constexpr int ROAD_RIGHT = 180;              // ... to x = ROAD_RIGHT
constexpr int ROAD_LANES = 3;
constexpr int KERB_W = 5;                    // red/white kerb on both edges
constexpr int KERB_SEGMENT = 12;             // kerb stripe length
constexpr int DASH_LEN = 16, DASH_GAP = 14;  // lane markings

// ---------------------------------------------------------------------------
// Player car
// ---------------------------------------------------------------------------
constexpr int CAR_W = 18;
constexpr int CAR_H = 30;
constexpr int CAR_Y = 180;                   // centre of the car, fixed on screen

// How far one knob step moves the car sideways. With 4 px the car crosses
// the whole road (road width - kerbs - CAR_W = 92 px) in 23 steps, about a
// third of a turn.
constexpr float KNOB_PX_PER_STEP = 4.0f;

// The drawn position eases toward the target position. Time constant in ms:
// after EASE_TAU_MS the car has covered ~63 % of the remaining distance.
// Smaller = snappier, larger = smoother. Frame-rate independent.
constexpr float EASE_TAU_MS = 35.0f;

// ---------------------------------------------------------------------------
// Obstacles (geometry; collisions follow in M3, difficulty in M4)
// ---------------------------------------------------------------------------
constexpr float OBSTACLE_SPAWN_Y = -24.0f;   // start just above the screen
constexpr float OBSTACLE_ARM_Y = 70.0f;      // faint above, solid below (telegraph)
constexpr float OBSTACLE_REMOVE_Y = SCREEN_H + 30.0f;

constexpr int BARRIER_H = 10;                // roadblock thickness
constexpr int OIL_H = 18;                    // oil spill depth (jump over it)
constexpr int TRAFFIC_W = 18, TRAFFIC_H = 30;
constexpr int COIN_R = 5;

// M2 demo spawner only (replaced by patterns in M3).
constexpr uint32_t DEMO_SPAWN_MS = 1000;
constexpr float DEMO_SPEED_PX_S = 120.0f;    // road scroll speed
constexpr float DEMO_TRAFFIC_PX_S = 50.0f;   // traffic drives forward this fast
constexpr float DEMO_GAP_PX = 44.0f;         // roadblock gap width

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

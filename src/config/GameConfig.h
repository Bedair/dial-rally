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

// Encoder glitch fix. When both encoder pins change between two interrupts,
// the M5Dial driver counts ±2 and has to guess the direction; M0 logs showed
// it guessing wrong (-2 while turning steadily clockwise). A ±2 against the
// direction of a move less than this long ago is corrected to the same
// direction. Real reversals of 2 clicks within one frame that fast are rare.
constexpr uint32_t KNOB_GLITCH_WINDOW_MS = 100;

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

// Jump (press / tap): clears oil spills only; roadblocks and traffic still hit.
constexpr uint32_t JUMP_MS = 600;
constexpr float JUMP_LIFT_PX = 7.0f;         // how far the car sprite rises at the top

// Collision forgiveness: hit boxes are this many px smaller on every side
// than the drawn shapes, so a graze that looks clean is not a crash.
constexpr float HITBOX_MARGIN = 2.0f;

// ---------------------------------------------------------------------------
// Obstacles (geometry)
// ---------------------------------------------------------------------------
constexpr float OBSTACLE_SPAWN_Y = -24.0f;   // start just above the screen
constexpr float OBSTACLE_ARM_Y = 70.0f;      // faint above, solid below (telegraph)
constexpr float OBSTACLE_REMOVE_Y = SCREEN_H + 30.0f;

constexpr int BARRIER_H = 10;                // roadblock thickness
constexpr int OIL_H = 18;                    // oil spill depth (jump over it)
constexpr int TRAFFIC_W = 18, TRAFFIC_H = 30;
constexpr int COIN_R = 5;

constexpr float TRAFFIC_PX_S = 50.0f;        // traffic drives forward this fast

// ---------------------------------------------------------------------------
// Difficulty curve (Difficulty.h): linear from START to the cap over the ramp
// ---------------------------------------------------------------------------
constexpr uint32_t DIFF_RAMP_MS = 150000;    // 2.5 min to full difficulty
constexpr float ROAD_SPEED_START = 120.0f;   // road speed, px/s
constexpr float ROAD_SPEED_MAX = 210.0f;
constexpr float GAP_START_PX = 46.0f;        // roadblock gap width
constexpr float GAP_MIN_PX = 34.0f;          // car hit box is 14 px wide: 10 px slack each side
constexpr float SPACING_MIN_SCALE = 0.55f;   // time between pattern steps shrinks to 55 %

// ---------------------------------------------------------------------------
// Scoring
// ---------------------------------------------------------------------------
constexpr uint32_t POINTS_PER_OBSTACLE = 1;  // each obstacle that passes the car
constexpr uint32_t POINTS_PER_COIN = 10;
constexpr uint32_t MILESTONE_EVERY = 25;     // obstacles
constexpr uint32_t MILESTONE_BONUS = 50;

// ---------------------------------------------------------------------------
// Patterns and fairness
// ---------------------------------------------------------------------------
// Obstacles come from predefined patterns (Patterns.cpp). Every obstacle is
// placed so the car can reach a safe position in time, assuming the player
// reacts within FAIR_REACTION_MS and then turns the knob at FAIR_STEPS_PER_S.
constexpr float FAIR_STEPS_PER_S = 32.0f;    // half a knob turn per second
constexpr uint32_t FAIR_REACTION_MS = 150;
constexpr uint32_t PATTERN_GAP_MS = 700;     // breathing time between patterns (× spacingScale)
constexpr uint32_t OIL_MIN_SPACING_MS = JUMP_MS + 300;   // so every oil can be jumped

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

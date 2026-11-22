#pragma once
// Motion Quest – central configuration.

#include <stdint.h>

namespace cfg {

// PORT.A (Grove, red) on the M5Stack Dial
constexpr int PORTA_SDA = 13;
constexpr int PORTA_SCL = 15;
constexpr uint32_t I2C_FREQ_HZ = 100000;   // 100 kHz: safest for the Grove cable

constexpr uint8_t GESTURE_I2C_ADDR = 0x73;   // PAJ7620U2

// Sensor mounting: set to true if the sensor is rotated 180° in the enclosure,
// so a physical upward swipe is reported as "down".
constexpr bool GESTURE_SWAP_UP_DOWN = false;

// Sensor sampling: false = normal 120 fps, true = "game" 240 fps.
// Day 0: test both and compare latency.
constexpr bool GESTURE_GAME_SPEED = false;

// Gesture timing (starting values, tune after the Day 0 test)
constexpr uint32_t GESTURE_POLL_MS     = 20;
constexpr uint32_t GESTURE_COOLDOWN_MS = 350;

}  // namespace cfg

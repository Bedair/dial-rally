#pragma once
// Motion Quest – central configuration.
// Every tunable number lives here so hardware tests only need one file edited.

#include <stdint.h>

namespace cfg {

// ---------------------------------------------------------------------------
// PORT.A (Grove, red) on the M5Stack Dial
// ---------------------------------------------------------------------------
constexpr int PORTA_SDA = 13;
constexpr int PORTA_SCL = 15;
constexpr uint32_t I2C_FREQ_HZ = 100000;   // 100 kHz: safest for the Grove cable

// ---------------------------------------------------------------------------
// IMU: M5Stack Mini 6-axis IMU Unit (MPU6886) on PORT.A, worn on the wrist
// ---------------------------------------------------------------------------
constexpr uint8_t IMU_I2C_ADDR          = 0x68;
constexpr uint8_t IMU_WHO_AM_I_EXPECTED = 0x19;   // MPU6886 WHO_AM_I (reg 0x75)
constexpr uint32_t IMU_SAMPLE_MS        = 5;      // 200 Hz polling

// Axis mapping. Index into the sensor axes: 0 = X, 1 = Y, 2 = Z.
// FLICK_GYRO_AXIS is the axis the wrist rotates around when flicking up/down
// (it runs across the wrist when the unit lies flat on the forearm).
// FLICK_UP_SIGN is +1 if a flick UP gives a positive rate on that axis, else -1.
// Day 0: flick and read which gyro column in the CSV is largest, then set these.
constexpr uint8_t AXIS_X = 0, AXIS_Y = 1, AXIS_Z = 2;
constexpr uint8_t FLICK_GYRO_AXIS = AXIS_Y;   // placeholder until the bracer test
constexpr int8_t  FLICK_UP_SIGN   = +1;

// Flick detection thresholds (starting values, tune with the CSV log).
// A capture window opens when ANY gyro axis exceeds FLICK_LOG_DPS, so near
// misses and wrong-axis motion are logged too.
constexpr float FLICK_LOG_DPS     = 120.0f;   // open a capture window / log it
constexpr float FLICK_TRIGGER_DPS = 250.0f;   // flick axis must exceed this to fire
constexpr float FLICK_DOMINANCE   = 1.2f;     // flick axis >= this × other axes (0 = off)
constexpr uint32_t FLICK_WINDOW_MS   = 150;   // peak capture window per flick
constexpr uint32_t FLICK_COOLDOWN_MS = 300;   // ignore the wrist's return swing

// ---------------------------------------------------------------------------
// Day 0 test feedback
// ---------------------------------------------------------------------------
constexpr uint32_t FLASH_MS     = 300;
constexpr uint16_t BEEP_JUMP_HZ = 2400;
constexpr uint16_t BEEP_DUCK_HZ = 1200;
constexpr uint32_t BEEP_MS      = 40;

// ---------------------------------------------------------------------------
// PAJ7620 gesture sensor (unused since Day 0: the unit was dead, kept for the record)
// ---------------------------------------------------------------------------
constexpr uint8_t GESTURE_I2C_ADDR = 0x73;   // PAJ7620U2

// Sensor mounting: set to true if the sensor is rotated 180° in the enclosure,
// so a physical upward swipe is reported as "down".
constexpr bool GESTURE_SWAP_UP_DOWN = false;

// Sensor sampling: false = normal 120 fps, true = "game" 240 fps.
constexpr bool GESTURE_GAME_SPEED = false;

constexpr uint32_t GESTURE_POLL_MS     = 20;
constexpr uint32_t GESTURE_COOLDOWN_MS = 350;

}  // namespace cfg

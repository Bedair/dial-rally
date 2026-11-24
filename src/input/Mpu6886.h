#pragma once
// Minimal MPU6886 driver (accelerometer + gyroscope over I2C).
// Only what the game needs: identify the chip, configure ranges, read one sample.

#include <Wire.h>
#include <stdint.h>

struct ImuSample {
    float a[3];          // acceleration X/Y/Z in g (includes gravity)
    float g[3];          // angular rate X/Y/Z in degrees per second
    uint32_t tMs;        // millis() when it was read
};

class Mpu6886 {
public:
    static constexpr uint8_t REG_WHO_AM_I = 0x75;

    // Reads WHO_AM_I (0x75) and, for diagnostics, register 0x00 (chip ID on
    // Bosch BMI160/BMI270, which also live at 0x68). Returns false if the
    // device does not answer at all.
    static bool identify(TwoWire& wire, uint8_t addr, uint8_t& whoAmI, uint8_t& reg00);

    // Human-readable guess for a WHO_AM_I / reg 0x00 pair, for the Day 0 report.
    static const char* chipName(uint8_t whoAmI, uint8_t reg00);

    // Resets and configures the chip: ±8 g, ±2000 dps, 1 kHz internal rate.
    bool begin(TwoWire& wire, uint8_t addr);

    // Reads accel + gyro in one 14-byte burst. Returns false on I2C error.
    bool read(ImuSample& out);

private:
    bool writeReg(uint8_t reg, uint8_t val);
    static bool readRegs(TwoWire& wire, uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t len);

    TwoWire* wire_ = nullptr;
    uint8_t addr_ = 0;
};

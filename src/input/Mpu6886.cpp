#include "input/Mpu6886.h"
#include <Arduino.h>

namespace {
// Register map (MPU6886 datasheet)
constexpr uint8_t REG_SMPLRT_DIV   = 0x19;
constexpr uint8_t REG_CONFIG       = 0x1A;
constexpr uint8_t REG_GYRO_CONFIG  = 0x1B;
constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
constexpr uint8_t REG_ACCEL_CONF2  = 0x1D;
constexpr uint8_t REG_INT_ENABLE   = 0x38;
constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;   // 14 bytes: accel XYZ, temp, gyro XYZ
constexpr uint8_t REG_USER_CTRL    = 0x6A;
constexpr uint8_t REG_PWR_MGMT_1   = 0x6B;
constexpr uint8_t REG_PWR_MGMT_2   = 0x6C;

constexpr float ACCEL_LSB_PER_G   = 4096.0f;   // ±8 g
constexpr float GYRO_LSB_PER_DPS  = 16.4f;     // ±2000 dps

inline int16_t be16(const uint8_t* p) { return (int16_t)((p[0] << 8) | p[1]); }
}  // namespace

bool Mpu6886::readRegs(TwoWire& wire, uint8_t addr, uint8_t reg, uint8_t* buf, uint8_t len) {
    wire.beginTransmission(addr);
    wire.write(reg);
    if (wire.endTransmission(false) != 0) return false;   // repeated start
    if (wire.requestFrom(addr, len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = wire.read();
    return true;
}

bool Mpu6886::writeReg(uint8_t reg, uint8_t val) {
    wire_->beginTransmission(addr_);
    wire_->write(reg);
    wire_->write(val);
    return wire_->endTransmission() == 0;
}

bool Mpu6886::identify(TwoWire& wire, uint8_t addr, uint8_t& whoAmI, uint8_t& reg00) {
    whoAmI = reg00 = 0;
    if (!readRegs(wire, addr, REG_WHO_AM_I, &whoAmI, 1)) return false;
    readRegs(wire, addr, 0x00, &reg00, 1);   // diagnostics only, may fail
    return true;
}

const char* Mpu6886::chipName(uint8_t whoAmI, uint8_t reg00) {
    switch (whoAmI) {
        case 0x19: return "MPU6886";
        case 0x68: return "MPU6050";
        case 0x70: return "MPU6500";
        case 0x71: return "MPU9250";
        case 0x73: return "MPU9255";
        case 0x12: return "ICM-20602";
        default: break;
    }
    if (reg00 == 0x24) return "BMI270";
    if (reg00 == 0xD1) return "BMI160";
    return "unknown";
}

bool Mpu6886::begin(TwoWire& wire, uint8_t addr) {
    wire_ = &wire;
    addr_ = addr;
    // Setup only: short blocking delays are fine here, never in the game loop.
    if (!writeReg(REG_PWR_MGMT_1, 0x80)) return false;   // device reset
    delay(20);
    bool ok = true;
    ok &= writeReg(REG_PWR_MGMT_1, 0x01);     // wake, auto-select best clock
    delay(10);
    ok &= writeReg(REG_PWR_MGMT_2, 0x00);     // all accel + gyro axes on
    ok &= writeReg(REG_ACCEL_CONFIG, 0x10);   // ±8 g: flicks can exceed 4 g
    ok &= writeReg(REG_GYRO_CONFIG, 0x18);    // ±2000 dps: flicks exceed 1000 dps
    ok &= writeReg(REG_CONFIG, 0x01);         // gyro DLPF 176 Hz
    ok &= writeReg(REG_ACCEL_CONF2, 0x00);    // accel DLPF 218 Hz
    ok &= writeReg(REG_SMPLRT_DIV, 0x00);     // 1 kHz internal rate
    ok &= writeReg(REG_USER_CTRL, 0x00);      // FIFO off
    ok &= writeReg(REG_INT_ENABLE, 0x00);     // no interrupts (INT not on Grove)
    delay(10);

    // Read back one config register to make sure the writes landed.
    uint8_t gyroCfg = 0;
    ok &= readRegs(wire, addr, REG_GYRO_CONFIG, &gyroCfg, 1) && gyroCfg == 0x18;
    return ok;
}

bool Mpu6886::read(ImuSample& out) {
    uint8_t buf[14];
    if (!wire_ || !readRegs(*wire_, addr_, REG_ACCEL_XOUT_H, buf, sizeof(buf))) return false;
    for (int i = 0; i < 3; i++) {
        out.a[i] = be16(&buf[i * 2]) / ACCEL_LSB_PER_G;
        out.g[i] = be16(&buf[8 + i * 2]) / GYRO_LSB_PER_DPS;   // skip 2 temp bytes
    }
    out.tMs = millis();
    return true;
}

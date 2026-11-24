#include "input/ImuSource.h"
#include "config/GameConfig.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>

namespace {
// Keeps the value with the larger magnitude, preserving its sign.
inline void keepPeak(float& peak, float v) {
    if (fabsf(v) > fabsf(peak)) peak = v;
}
}  // namespace

bool ImuSource::begin() {
    // Wire is started on PORT.A pins by main.cpp before this call.
    present_ = Mpu6886::identify(Wire, cfg::IMU_I2C_ADDR, whoAmI_, reg00_);
    Serial.printf("[MQ] IMU @0x%02X: %s, WHO_AM_I(0x75)=0x%02X reg0x00=0x%02X -> %s (expect 0x%02X)\n",
                  cfg::IMU_I2C_ADDR, present_ ? "answered" : "NO ANSWER", whoAmI_, reg00_,
                  Mpu6886::chipName(whoAmI_, reg00_), cfg::IMU_WHO_AM_I_EXPECTED);

    // Do not drive an unconfirmed chip with MPU6886 register writes.
    if (!present_ || whoAmI_ != cfg::IMU_WHO_AM_I_EXPECTED) return false;

    const bool ok = imu_.begin(Wire, cfg::IMU_I2C_ADDR);
    Serial.printf("[MQ] MPU6886 config %s\n", ok ? "OK" : "FAILED");
    capturing_ = false;
    recordReady_ = false;
    return ok;
}

void ImuSource::openWindow(const ImuSample& s) {
    current_ = FlickRecord{};
    current_.tMs = s.tMs;
    capturing_ = true;
    firedInWindow_ = false;
}

void ImuSource::trackPeaks(const ImuSample& s) {
    if (current_.samples > 0) {
        const uint32_t gap = s.tMs - lastWindowSampleMs_;
        if (gap > current_.maxGapMs) current_.maxGapMs = gap;
    }
    lastWindowSampleMs_ = s.tMs;
    current_.samples++;
    const float rate = s.g[cfg::FLICK_GYRO_AXIS] * cfg::FLICK_UP_SIGN;   // + = up
    current_.upPeak   = fmaxf(current_.upPeak, rate);
    current_.downPeak = fminf(current_.downPeak, rate);
    for (int i = 0; i < 3; i++) {
        keepPeak(current_.peakA[i], s.a[i]);
        keepPeak(current_.peakG[i], s.g[i]);
    }
}

bool ImuSource::tryFire(const ImuSample& s, ActionEvent& out) {
    const float rate = s.g[cfg::FLICK_GYRO_AXIS] * cfg::FLICK_UP_SIGN;   // + = up
    const float mag = fabsf(rate);
    if (mag < cfg::FLICK_TRIGGER_DPS) return false;

    // Reject motion that is mostly around another axis (e.g. twisting the forearm).
    // Compare against the other axes' peaks so far in this window, not just the
    // current sample: a twist often peaks first and the flick axis catches up later.
    // (trackPeaks() has already folded the current sample into the peaks.)
    float other = 0;
    for (int i = 0; i < 3; i++) {
        if (i != cfg::FLICK_GYRO_AXIS) other = fmaxf(other, fabsf(current_.peakG[i]));
    }
    if (cfg::FLICK_DOMINANCE > 0 && mag < cfg::FLICK_DOMINANCE * other) return false;

    out.action = rate > 0 ? Action::Jump : Action::Duck;
    out.timestampMs = s.tMs;
    firedInWindow_ = true;
    current_.action = out.action;
    current_.fireMs = (int32_t)(s.tMs - current_.tMs);
    cooldownUntilMs_ = s.tMs + cfg::FLICK_COOLDOWN_MS;
    return true;
}

bool ImuSource::poll(ActionEvent& out) {
    const uint32_t now = millis();
    if (now - lastSampleMs_ < cfg::IMU_SAMPLE_MS) return false;
    lastSampleMs_ = now;

    ImuSample s;
    if (!imu_.read(s)) {
        i2cErrors_++;
        return false;
    }

    // All window/cooldown timing uses the sample's own timestamp (taken after
    // the I2C read). Mixing it with `now` (taken before the read) made
    // `now - tMs` wrap to a huge unsigned value and closed every window instantly.
    if (!capturing_) {
        const bool coolingDown = (int32_t)(s.tMs - cooldownUntilMs_) < 0;   // wrap-safe
        float maxRate = 0;
        for (float g : s.g) maxRate = fmaxf(maxRate, fabsf(g));
        if (coolingDown || maxRate < cfg::FLICK_LOG_DPS) return false;
        openWindow(s);
    }

    trackPeaks(s);
    const bool fired = !firedInWindow_ && tryFire(s, out);

    if (s.tMs - current_.tMs >= cfg::FLICK_WINDOW_MS) {
        capturing_ = false;
        ready_ = current_;
        recordReady_ = true;
    }
    return fired;
}

bool ImuSource::takeRecord(FlickRecord& out) {
    if (!recordReady_) return false;
    out = ready_;
    recordReady_ = false;
    return true;
}

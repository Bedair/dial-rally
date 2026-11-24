#pragma once
// Wrist IMU as an input source: turns wrist flicks into Jump / Duck actions.
//
// Detection (all numbers in GameConfig.h):
//  1. A capture window opens when any gyro axis exceeds FLICK_LOG_DPS.
//  2. Inside the window, the action fires as soon as the flick axis exceeds
//     FLICK_TRIGGER_DPS and dominates the other axes. The sign of the rate
//     at that moment gives the direction (up = Jump, down = Duck).
//     Firing immediately (not at window end) keeps latency low.
//  3. Peaks keep being tracked until the window closes, then one FlickRecord
//     is published for logging (also for near misses that never fired).
//  4. After firing, a cooldown ignores the wrist's return swing.

#include "input/Actions.h"
#include "input/Mpu6886.h"

// Summary of one capture window, for the threshold-tuning log.
struct FlickRecord {
    uint32_t tMs = 0;                 // millis() when the window opened
    float peakA[3] = {0, 0, 0};       // signed peak acceleration per axis, g
    float peakG[3] = {0, 0, 0};       // signed peak angular rate per axis, dps
    Action action = Action::None;     // Jump / Duck, or None for a near miss
    int32_t fireMs = -1;              // ms from window open to firing, -1 if none
    uint16_t samples = 0;             // IMU samples taken inside the window
    uint32_t maxGapMs = 0;            // longest gap between samples (shows render stalls)
    float upPeak = 0;                 // flick axis, strongest "up" rate (>= 0), dps
    float downPeak = 0;               // flick axis, strongest "down" rate (<= 0), dps
};

class ImuSource : public IInputSource {
public:
    // Checks WHO_AM_I first; the driver is only started if it matches.
    bool begin() override;
    bool poll(ActionEvent& out) override;
    const char* name() const override { return "MPU6886"; }

    // Returns true once per finished capture window.
    bool takeRecord(FlickRecord& out);

    bool present() const { return present_; }       // answered on I2C
    uint8_t whoAmI() const { return whoAmI_; }
    uint8_t reg00() const { return reg00_; }
    uint32_t i2cErrors() const { return i2cErrors_; }
    // True while a capture window is open: the caller should avoid slow
    // screen pushes then, because they stall IMU sampling.
    bool capturing() const { return capturing_; }

private:
    void openWindow(const ImuSample& s);
    void trackPeaks(const ImuSample& s);
    bool tryFire(const ImuSample& s, ActionEvent& out);

    Mpu6886 imu_;
    bool present_ = false;
    uint8_t whoAmI_ = 0, reg00_ = 0;
    uint32_t i2cErrors_ = 0;

    uint32_t lastSampleMs_ = 0;
    uint32_t lastWindowSampleMs_ = 0;
    bool capturing_ = false;
    bool firedInWindow_ = false;
    uint32_t cooldownUntilMs_ = 0;

    FlickRecord current_;
    FlickRecord ready_;
    bool recordReady_ = false;
};

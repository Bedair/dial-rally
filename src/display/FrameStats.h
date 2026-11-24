#pragma once
// Frame-time statistics for debug builds: average / max work time per frame
// (input + logic + render + push) and the achieved frame rate.

#include <Arduino.h>
#include "config/GameConfig.h"

class FrameStats {
public:
    void begin(uint32_t nowMs) { reset(nowMs); }

    // Call once per frame with the frame's start time and its work time.
    void add(uint32_t frameStartMs, uint32_t workMs) {
        if (!cfg::DEBUG_LOG) return;
        frames_++;
        sumMs_ += workMs;
        if (workMs > maxMs_) maxMs_ = workMs;
        const uint32_t span = frameStartMs - startMs_;
        if (span >= cfg::FRAME_STATS_MS) {
            Serial.printf("[MQ] frame: %.1f fps, work avg %.1f ms, max %lu ms, heap %u B\n",
                          frames_ * 1000.0f / span, (float)sumMs_ / frames_,
                          (unsigned long)maxMs_, (unsigned)ESP.getFreeHeap());
            reset(frameStartMs);
        }
    }

private:
    void reset(uint32_t nowMs) {
        startMs_ = nowMs;
        frames_ = sumMs_ = maxMs_ = 0;
    }

    uint32_t startMs_ = 0, frames_ = 0, sumMs_ = 0, maxMs_ = 0;
};

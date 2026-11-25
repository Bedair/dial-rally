#pragma once
// Frame-time statistics (M7 validation).
//
// Work time = input + logic + render + push for one frame.
//   Every FRAME_STATS_MS:    "[MQ] frame: ..."   fps, average and max work time
//   Every SESSION_REPORT_MS: "[MQ] session ..."  totals since boot: uptime,
//       frames, work-time percentiles (p50/p95/p99) and max, frames over the
//       30 fps budget (FRAME_BUDGET_MS), heap now / lowest ever / largest
//       free block, and runs played. This is the line to watch in the
//       long-run soak test (docs/TESTING.md).
// Percentiles come from a 1-ms histogram, so they cost no memory per frame.

#include <Arduino.h>
#include "config/GameConfig.h"

class FrameStats {
public:
    void begin(uint32_t nowMs) {
        window(nowMs);
        sessionStartMs_ = lastReportMs_ = nowMs;
    }

    // Call once per frame with the frame's start time and its work time.
    void add(uint32_t frameStartMs, uint32_t workMs) {
        if (!cfg::DEBUG_LOG) return;
        frames_++;
        sumMs_ += workMs;
        if (workMs > maxMs_) maxMs_ = workMs;

        totalFrames_++;
        hist_[workMs < BUCKETS - 1 ? workMs : BUCKETS - 1]++;
        if (workMs > maxEverMs_) maxEverMs_ = workMs;
        if (workMs > cfg::FRAME_BUDGET_MS) overBudget_++;

        const uint32_t span = frameStartMs - startMs_;
        if (span >= cfg::FRAME_STATS_MS) {
            Serial.printf("[MQ] frame: %.1f fps, work avg %.1f ms, max %lu ms, heap %u B\n",
                          frames_ * 1000.0f / span, (float)sumMs_ / frames_,
                          (unsigned long)maxMs_, (unsigned)ESP.getFreeHeap());
            window(frameStartMs);
        }
    }

    // Prints the session summary when SESSION_REPORT_MS has passed.
    void maybeReport(uint32_t nowMs, uint32_t runs) {
        if (!cfg::DEBUG_LOG || nowMs - lastReportMs_ < cfg::SESSION_REPORT_MS) return;
        lastReportMs_ = nowMs;
        const uint32_t s = (nowMs - sessionStartMs_) / 1000;
        Serial.printf("[MQ] session %02lu:%02lu:%02lu, %lu frames, work p50 %lu p95 %lu p99 %lu "
                      "max %lu ms, over %lu ms: %lu, heap %u B (min %u, largest block %u), "
                      "runs %lu\n",
                      (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60),
                      (unsigned long)(s % 60), (unsigned long)totalFrames_,
                      (unsigned long)percentile(50), (unsigned long)percentile(95),
                      (unsigned long)percentile(99), (unsigned long)maxEverMs_,
                      (unsigned long)cfg::FRAME_BUDGET_MS, (unsigned long)overBudget_,
                      (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
                      (unsigned)ESP.getMaxAllocHeap(), (unsigned long)runs);
    }

private:
    static constexpr uint32_t BUCKETS = 64;   // 0..62 ms, last bucket = 63 ms or more

    void window(uint32_t nowMs) {
        startMs_ = nowMs;
        frames_ = sumMs_ = maxMs_ = 0;
    }

    // Smallest work time (ms) that p % of all frames did not exceed.
    uint32_t percentile(uint32_t p) const {
        if (!totalFrames_) return 0;
        const uint64_t want = ((uint64_t)totalFrames_ * p + 99) / 100;
        uint64_t seen = 0;
        for (uint32_t ms = 0; ms < BUCKETS; ms++) {
            seen += hist_[ms];
            if (seen >= want) return ms;
        }
        return BUCKETS - 1;
    }

    // Short window (FRAME_STATS_MS).
    uint32_t startMs_ = 0, frames_ = 0, sumMs_ = 0, maxMs_ = 0;
    // Whole session since boot.
    uint32_t sessionStartMs_ = 0, lastReportMs_ = 0;
    uint32_t totalFrames_ = 0, maxEverMs_ = 0, overBudget_ = 0;
    uint32_t hist_[BUCKETS] = {};
};

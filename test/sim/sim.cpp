// Dial Rally – host simulation of the real game logic (M7 validation).
//
// Compiles src/game/*.cpp unchanged for the PC and checks, over many random
// seeds, the claims the game makes:
//
//   1. Fairness: the AutoPilot, which never turns the knob faster than
//      FAIR_STEPS_PER_S, survives every run for the whole duration, through
//      the full difficulty ramp. If a pattern were impossible, it would crash.
//   2. Danger: a driver that does nothing crashes quickly in every run, so
//      collisions really are checked (and all three causes occur).
//   3. Spawning: obstacles never appear in the middle of the screen, and the
//      obstacle pool never fills up.
//
// Build and run: see test/sim/README.md. Exit code 0 = all checks passed.

#include <Arduino.h>
#include <cstring>
#include <set>
#include <string>
#include "config/GameConfig.h"
#include "game/AutoPilot.h"
#include "game/GameEngine.h"

SerialStub Serial;

namespace {

constexpr uint32_t FRAME_MS = 16;

struct RunResult {
    bool crashed = false;
    std::string cause, pattern;
    uint32_t timeMs = 0, score = 0;
    int maxActive = 0, midSpawns = 0;
};

// Plays one run. driver: true = AutoPilot, false = hands off.
RunResult play(unsigned seed, uint32_t limitMs, bool driver) {
    std::srand(seed);
    GameEngine g;
    g.reset();
    AutoPilot pilot;
    pilot.reset();
    bool wasActive[HazardField::MAX] = {};
    RunResult r;

    while (g.timeMs() < limitMs && !g.crashed()) {
        if (driver) {
            const AutoPilot::Decision d = pilot.drive(g, FRAME_MS);
            if (d.steps) g.steer(d.steps);
            if (d.jump) g.jump();
        }
        g.update(FRAME_MS, true);
        (void)g.takeEvents();

        int active = 0;
        for (int i = 0; i < HazardField::MAX; i++) {
            const Hazard& h = g.hazards()[i];
            if (h.active) active++;
            // A new obstacle must start above the screen (allow one frame of travel).
            if (h.active && !wasActive[i] && h.y > cfg::OBSTACLE_SPAWN_Y + 6) r.midSpawns++;
            wasActive[i] = h.active;
        }
        if (active > r.maxActive) r.maxActive = active;
    }
    r.crashed = g.crashed();
    if (r.crashed) { r.cause = g.crashCause(); r.pattern = g.crashPattern(); }
    r.timeMs = g.timeMs();
    r.score = g.score();
    return r;
}

}  // namespace

int main(int argc, char** argv) {
    int seeds = 20;
    uint32_t minutes = 30;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--seeds") && i + 1 < argc) seeds = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--minutes") && i + 1 < argc) minutes = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--verbose")) Serial.verbose = true;
        else { std::printf("usage: sim [--seeds N] [--minutes M] [--verbose]\n"); return 2; }
    }
    const uint32_t limitMs = minutes * 60u * 1000u;
    bool ok = true;
    int worstActive = 0, midSpawns = 0;

    std::printf("== 1. fairness: AutoPilot at %.0f knob steps/s, %d seeds x %u min ==\n",
                cfg::FAIR_STEPS_PER_S, seeds, minutes);
    for (int s = 1; s <= seeds; s++) {
        const RunResult r = play(s, limitMs, true);
        worstActive = r.maxActive > worstActive ? r.maxActive : worstActive;
        midSpawns += r.midSpawns;
        if (r.crashed) {
            ok = false;
            std::printf("  seed %2d: FAIL crash %s (%s) after %.1f s\n", s, r.cause.c_str(),
                        r.pattern.c_str(), r.timeMs / 1000.0f);
        } else {
            std::printf("  seed %2d: survived, score %u\n", s, r.score);
        }
    }

    std::printf("== 2. danger: no driver, every run must crash within 15 s ==\n");
    std::set<std::string> causes;
    for (int s = 1; s <= seeds; s++) {
        const RunResult r = play(1000 + s, 15000, false);
        midSpawns += r.midSpawns;
        if (!r.crashed) {
            ok = false;
            std::printf("  seed %2d: FAIL no crash\n", 1000 + s);
        } else {
            causes.insert(r.cause);
        }
    }
    std::printf("  crash causes seen: %zu of 3 (", causes.size());
    for (const auto& c : causes) std::printf(" %s", c.c_str());
    std::printf(" )\n");
    if (seeds >= 10 && causes.size() < 3) { ok = false; std::printf("  FAIL: not all causes seen\n"); }

    std::printf("== 3. spawning ==\n  max obstacles on screen %d of %d, mid-screen spawns %d\n",
                worstActive, HazardField::MAX, midSpawns);
    if (midSpawns > 0 || worstActive >= HazardField::MAX) ok = false;

    std::printf("== %s ==\n", ok ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    return ok ? 0 : 1;
}

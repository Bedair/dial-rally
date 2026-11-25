# Host simulation

Runs the real game logic (`src/game/*.cpp`, compiled unchanged for the PC) for many random runs and checks:

1. **Fairness**: an autopilot that never turns the knob faster than `FAIR_STEPS_PER_S` survives every run, for 30 minutes each, through the whole difficulty ramp. An impossible obstacle sequence would make it crash.
2. **Danger**: a driver that does nothing crashes within 15 s in every run, and all three crash causes (roadblock, oil, traffic) occur.
3. **Spawning**: obstacles never appear mid-screen and the obstacle pool never fills.

```bash
test/sim/run.sh                        # 20 seeds x 30 min of game time, ~1-3 s
test/sim/run.sh --seeds 5 --minutes 5  # quicker
test/sim/run.sh --verbose              # also print the game's [MQ] log lines
```

Needs `g++` (or set `CXX=clang++`) with C++17. Exit code 0 means all checks passed.

`stubs/Arduino.h` replaces the few Arduino functions the game logic uses (`random`, `Serial.printf`). The autopilot is `src/game/AutoPilot.*`, the same code the soak-test firmware uses on the Dial.

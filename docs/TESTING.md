# Dial Rally – test and validation (M7)

Three layers:

| | What | Where | Time |
|---|---|---|---|
| A | Host simulation: fairness, collisions, spawning | PC, `test/sim/run.sh` | seconds |
| B | Hardware checklist: every feature by hand | Dial, normal firmware | ~20 min |
| C | Soak test: hours of unattended play, frame time and memory | Dial, `soak` firmware | 4–8 h |

Record results in the table at the end; they go into the Hackster write-up.

---

## A. Host simulation

```bash
test/sim/run.sh
```

Pass: the last line is `== ALL CHECKS PASSED ==` (exit code 0). Details in `test/sim/README.md`.

Run it after any change to `src/game/` or the gameplay numbers in `GameConfig.h`.

---

## B. Hardware checklist

Normal firmware: `pio run -t upload`, then `pio device monitor`. Press the Dial's reset button (or replug) with the monitor open to see the boot lines.

| # | Area | Do | Expected |
|---|---|---|---|
| B1 | Boot | Power on | `[MQ] boot, reset reason: power-on` (or `usb`), `canvas ok 240x240 @ 16 bit`, `high score loaded: N`, `ready (Knob input)`. Menu shows DIAL RALLY. |
| B2 | Knob | Turn clockwise in the menu / in play | Selection moves down; car moves right. One click = one step, ~23 clicks cross the road. |
| B3 | Knob glitch fix | Spin fast in one direction during play | Car never jumps backwards; any `knob: corrected` line is followed by smooth movement. |
| B4 | Button / touch | Press BtnA, tap the screen during play | Each gives one jump (`jump (button)` / `jump (touch)`). |
| B5 | Menu | Turn and press | Tick on every selection change, confirm sound on press. High score screen shows the stored best, press returns. |
| B6 | Countdown | Play | 3-2-1 with a beep each, long GO note; car can already steer. |
| B7 | Pause | Hold BtnA ~0.6 s in play | One jump, then PAUSED; score and road freeze. Resume continues the same run; End run goes to results with cause "run ended". |
| B8 | Roadblock | Drive through a gap / clip a barrier | Pass = +1. Clip = crash, `crash: roadblock (<pattern>)`. |
| B9 | Oil | Jump over oil / drive into it | Jump: car lifts with shadow, chirp, survives. No jump: `crash: oil`. |
| B10 | Jump buffer | Press twice quickly | Two jumps back to back, two chirps. |
| B11 | Traffic | Steer around / hit a slow car; a "wall" of two | Hit = `crash: traffic`. The free lane of a wall is always reachable. |
| B12 | Coins | Drive over coins | Spinning coin disappears, "bling", "+10" rises, score +10. |
| B13 | Telegraph | Watch obstacles appear | Faint (dim) at the top, solid from about a third of the way down. Only solid obstacles can hit. |
| B14 | Milestone | Pass 25 obstacles | `milestone 1: +50`, banner "+50 BONUS", yellow score box, rising jingle. |
| B15 | Difficulty | Play 2–3 min | Road speeds up, gaps narrow, patterns come closer; milestone lines show speed rising toward 210 px/s and gap shrinking toward 34 px. |
| B16 | Crash | Crash into anything | White flash, explosion with debris, "CRASH!", screen shake, crunch. Presses ignored until the results panel is up. |
| B17 | Results | After the crash | Cause, score, best or blinking NEW HIGH SCORE (fanfare), stats line; Play again / Menu work. |
| B18 | High score | Beat the best, then replug | `high score saved: N`; after replug `high score loaded: N`. A lower score does not overwrite it. |
| B19 | Art | Look closely | No magenta pixels; cars, coins, oil, roadblocks, trees correct; road edges not cut off in a way that hides the car. |
| B20 | Frame time | Watch `[MQ] frame:` during play | ~62 fps, work avg ≤ 16 ms, max < 33 ms. |
| B21 | Fairness feel | Play 10+ runs | Every crash felt like your mistake. If not, note the pattern from the crash line. |

---

## C. Long-run soak test

The `soak` build replaces the knob with the autopilot (`src/game/AutoPilot.*`), which plays the real game on the Dial. Each run it drives fairly and then lets go after 45–240 s of game time, so it also crashes, shows results, restarts and occasionally writes a new high score, over and over. Sound is muted (the sound code still runs) and it keeps its own high score (`dialrally_sk`), so the player's best is untouched.

```bash
pio run -e soak -t upload
pio device monitor -e soak --filter log2file     # keeps a log file in the project folder
```

Keep the monitor open the whole time (USB serial can stall the game if nothing reads it). Leave it running for at least 4 hours, ideally overnight. Then reflash the normal firmware with `pio run -t upload`.

Every minute it prints one summary line, for example:

```
[MQ] session 04:00:00, 899000 frames, work p50 12 p95 14 p99 15 max 21 ms, over 33 ms: 0, heap 206784 B (min 205900, largest block 110580), runs 112
```

### Pass criteria

| # | Check | Pass |
|---|---|---|
| C1 | No reset | Exactly one `[MQ] boot` line in the whole log, and the session clock keeps counting up. Any `PANIC`, `WATCHDOG` or `BROWN-OUT` is a fail. |
| C2 | Game keeps cycling | `runs` keeps increasing (roughly one run every 1–4 minutes); `run N over` lines show all three crash causes. |
| C3 | Frame time | `p99` ≤ 20 ms and `max` < 33 ms; `over 33 ms` stays 0 (a handful in hours is acceptable, note it). |
| C4 | No memory leak | After the first 10 minutes, `heap` and `min` stop going down: the value at the end is within 1 KB of the value at 10 minutes. |
| C5 | No fragmentation | `largest block` stays the same (within 1 KB) from 10 minutes to the end. |

To pull the summary lines out of the log file: `grep "\[MQ\] session" platformio-device-monitor-*.log`. The first and the last line are enough for C3–C5.

---

## Results

| Date | Firmware (commit) | Test | Result | Notes |
|---|---|---|---|---|
| | | A host simulation | | |
| | | B hardware checklist | | |
| | | C soak, duration: | | first / last session line: |

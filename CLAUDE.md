# CLAUDE.md – The Lost Temple (repo: motion-quest)

Context for Claude Code working in this repository.

## Git rules (strict)

- NEVER run `git commit`, `git push`, `git tag`, `git rebase`, `git reset`, `git merge` or any command that changes git history or the remote.
- The developer tests each milestone on hardware and commits it himself.
- You may run read-only git commands (`git status`, `git diff`, `git log`).
- At the end of each milestone: stop, summarise what changed (files + why), list the hardware test steps the developer should run, and suggest a commit message. Do not commit.

## Project

The Lost Temple: an arcade game played entirely on one M5Stack Dial – round screen + knob, no external sensors or cables.
Contest: Hackster.io Junk Drawer Competition 2026, category Game & Fame. Deadline Jan 10, 2027 11:59 PM PT; target submission ~Jan 5.
Hook: "A whole arcade game in a single knob."

## Frozen decisions (v1.3) – do not change without asking

### Hardware
- M5Stack Dial only (M5StampS3, ESP32-S3, 240×240 round display, rotary encoder, front button BtnA, touchscreen, buzzer). No external units on PORT.A / PORT.B.
- Housing: 3D-printed stone-tablet enclosure (no electronics). Not part of the firmware.
- History for the write-up: the PAJ7620 Gesture Unit failed on Day 0 (no I2C answer on PORT.A with two cables, while other units answered on the same port). An IMU wrist controller was considered, then dropped to keep the project to a single device.

### Software platform
- PlatformIO, Arduino framework. Libraries: M5Dial, M5Unified, M5GFX only. No LVGL, no game framework, no PAJ7620 library.

### Gameplay
- The explorer stands on the rim of the round screen at angle `playerAngle`. Turning the knob moves the explorer around the rim (angle per encoder step in GameConfig.h; render position eases toward the target angle).
- Centre "altar" circle (radius ~38 px) shows the score. Hazards emerge from the altar edge and expand outward toward the rim.
- Hazard types:
  - Collapse ring: a stone ring with one gap (arc width configurable). The explorer survives only if inside the gap when the ring reaches the rim.
  - Rubble ring: a full, low ring with no gap. Press the Dial (BtnA) to jump over it. Touch-tap anywhere = jump as an alternative.
  - Coin: travels outward at an angle; collected if the explorer is at that angle when it reaches the rim.
- Jump lasts JUMP_MS (config). Jump is ignored while already jumping, but one press during a jump is buffered.
- Telegraphing: a hazard is drawn faint when it appears and solid before it becomes dangerous.
- Fairness: hazards come from predefined patterns. The angle change between consecutive gaps is limited to what the knob can reach at the current speed. Never generate an impossible sequence.
- Difficulty: ring speed increases, spawn interval shrinks and gap width narrows over time, with caps in GameConfig.h.
- Scoring: +1 per hazard survived, +10 per coin, milestone bonus every 25 hazards. High score stored in NVS via Preferences.
- States: Menu (title, Play, High score; knob navigates, press confirms) → Ready (3-2-1) → Playing → Paused (hold BtnA during play, KNOB_HOLD_MS; press = jump) → GameOver (score, high score, press to restart).

### Rendering and audio
- Draw each frame into one 240×240 M5Canvas and push once per frame. Allocate the canvas once at startup; log free heap. If 16-bit allocation fails, fall back to 8-bit colour.
- Target ≥ 30 fps; log frame time over serial in debug builds.
- Buzzer sound effects: coin, jump, crash, milestone. Short, non-blocking.
- All art original (simple pixel art / primitives). No third-party sprites, names or trademarks (contest rule).

### Stretch (not MVP, ask first)
- NFC "relic cards" via the Dial's built-in reader for a starting power-up.

## Input abstraction
- src/input/Actions.h defines abstract actions and IInputSource. Add a KnobSource that reports encoder steps (as an angle delta) and BtnA / touch as Jump / Confirm. The game never reads hardware directly.

## Milestones (one at a time, stop after each)

- M0 Cleanup + knob input: remove GestureSource and the RevEng_PAJ7620 dependency; implement KnobSource; test screen with an explorer dot moving around the rim with the knob and a flash on press. Measure and log encoder steps per full turn.
- M1 Game skeleton: state machine (Menu, Ready, Playing, Paused, GameOver), M5Canvas frame loop, frame-time logging.
- M2 Ring renderer: rim track, altar with score, explorer sprite, hazards expanding outward.
- M3 Mechanics: gap rings, rubble rings, jump + buffer, collisions, fair patterns, telegraphing.
- M4 Scoring: coins, difficulty curve, milestone bonus, high score in NVS.
- M5 Feedback: buzzer effects, crash flash / screen shake, game-over screen polish.
- M6 Art pass: original pixel-art sprites, png-to-RGB565 converter in tools/.
- M7 Validation: test checklist, frame-time stats, long-run stability test.

## Code layout

```
src/
  main.cpp
  config/GameConfig.h
  input/      Actions.h, KnobSource
  game/       GameController, GameEngine, Player, Hazard
  display/    DisplayManager
  storage/    ScoreManager
  audio/      AudioManager
assets/sprites/   source PNGs
tools/            png -> RGB565 header converter
```

## Conventions

- millis()-based timing, no blocking delay() in the game loop.
- Serial logs prefixed with [MQ], at 115200 baud.
- Build with `pio run` and fix errors. Upload with `pio run -t upload` only when the developer asks.
- Keep modules small and commented; the code is scored in the contest (BOM, schematics and code = 25 points).

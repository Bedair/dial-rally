# CLAUDE.md – Motion Quest

Context for Claude Code working in this repository.

## Git rules (strict)

- NEVER run `git commit`, `git push`, `git tag`, `git rebase`, `git reset` or any command that changes git history or the remote.
- The developer tests each milestone on hardware and commits it himself.
- You may run read-only git commands (`git status`, `git diff`, `git log`).
- At the end of each milestone: stop, summarise what changed (files + why), and list the hardware test steps the developer should run before committing. Suggest a commit message, but do not commit.

## Project

Motion Quest – The Lost Temple: a ring-runner game on the M5Stack Dial, controlled by flicking your wrist in the air.
Contest: Hackster.io Junk Drawer Competition 2026, category Game & Fame. Deadline Jan 10, 2027 11:59 PM PT; target submission ~Jan 5.

## Frozen decisions (v1.2) – do not change without asking

- Platform: M5Stack Dial (M5StampS3, ESP32-S3), PlatformIO, Arduino framework.
- Libraries: M5Dial, M5Unified, M5GFX. No LVGL, no game framework.
- Layout: ring runner. Explorer fixed at 6 o'clock; obstacles and coins move along the ring by angle. HUD (score, high score) in the centre.
- Input (changed in v1.2): M5Stack Mini 6-axis IMU Unit on PORT.A, worn on the wrist in a 3D-printed "explorer's bracer", Grove cable to the Dial.
  - PORT.A: SDA = G13, SCL = G15 (verified on hardware). IMU found at I2C 0x68.
  - Expected chip MPU6886 (WHO_AM_I register 0x75 should read 0x19). Verify on hardware; if it differs, report before writing the driver.
  - Wrist flick up = Jump, wrist flick down = Duck. Thresholds and axis mapping live in GameConfig.h.
  - Encoder = menu navigation, BtnA = confirm / pause / restart.
- Input abstraction: InputManager emits abstract actions (Jump, Duck, MenuNext, MenuPrev, Confirm) defined in src/input/Actions.h. Each sensor is an IInputSource. ImuSource is the active source; GestureSource stays in the repo but is unused.
- History: the PAJ7620 Gesture Unit failed on Day 0 (no I2C response on PORT.A with two cables, while the ultrasonic unit (0x57) and IMU (0x68) answered on the same port). Keep this for the write-up.
- One buffered action: a flick during jump/duck fires when the current action ends.
- Obstacles: rock (jump), spikes (jump), low pillar (duck). Telegraph before dangerous. Predefined patterns, never impossible sequences.
- Haptics: Vibrator Hat on PORT.B (G1/G2), pulse on collision.
- Rendering: draw each frame into a 240×240 M5Canvas and push once per frame. Draw ring background once; move objects only.
- High score in NVS via Preferences.
- All art original. No third-party sprites, names or trademarks (contest rule).

## Milestones (one at a time, stop after each)

- M0 IMU Day 0 test: MPU6886 driver, flick detection, JUMP/DUCK flash, CSV serial log (t, ax, ay, az, gx, gy, gz peaks, action), counters, tune thresholds.
- M1 Game skeleton: GameController state machine (Menu, Ready, Playing, Paused, GameOver), encoder + button navigation, M5Canvas frame loop.
- M2 Ring renderer: ring background, explorer sprite at 6 o'clock, objects moving by angle, HUD in the centre.
- M3 Mechanics: jump/duck states, one-action buffer, obstacle patterns, telegraphing, collisions.
- M4 Scoring: coins, score, difficulty curve, high score in NVS.
- M5 Feedback: buzzer sound effects, Vibrator Hat on PORT.B.
- M6 Art pass: original pixel-art sprites, png-to-RGB565 converter in tools/.
- M7 Validation: test checklist, flick detection rate, frame-time logging.

## Code layout

```
src/
  main.cpp
  config/GameConfig.h
  input/      Actions.h, InputManager, ImuSource, GestureSource (unused)
  game/       GameController, GameEngine, Player, Obstacle
  display/    DisplayManager
  storage/    ScoreManager
  audio/      AudioManager, Haptics
assets/sprites/   source PNGs
tools/            png -> RGB565 header converter
```

## Conventions

- millis()-based timing, no blocking delay() in the game loop.
- Serial logs prefixed with [MQ], at 115200 baud.
- Build with `pio run`; upload with `pio run -t upload` only when the developer asks.
- Keep modules small and commented; the code is scored in the contest (BOM, schematics and code = 25 points).

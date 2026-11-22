# CLAUDE.md – Motion Quest

Context for Claude Code working in this repository.

## Project

Motion Quest – The Lost Temple: a gesture-controlled ring-runner game on the M5Stack Dial.
Contest: Hackster.io Junk Drawer Competition 2026, category Game & Fame. Deadline Jan 10, 2027 11:59 PM PT; target submission ~Jan 5.

## Frozen decisions (v1.1) – do not change without asking

- Platform: M5Stack Dial (M5StampS3, ESP32-S3), PlatformIO, Arduino framework.
- Libraries: M5Dial, M5Unified, M5GFX. No LVGL, no game framework.
- Layout: ring runner. Explorer fixed at 6 o'clock; obstacles and coins move along the ring by angle. HUD (score, high score) in the centre.
- Input: PAJ7620U2 Gesture Unit on PORT.A (G13 SDA, G15 SCL, addr 0x73), polled (no interrupt on Grove).
  - Swipe up = jump (toward ring centre), swipe down = duck.
  - Encoder = menu navigation, BtnA = confirm / pause / restart.
  - Up/down mapping lives in GameConfig.h (sensor may be rotated in the enclosure).
- Input abstraction: InputManager emits abstract actions (JUMP, DUCK, MENU_NEXT, MENU_PREV, CONFIRM). Gesture and IMU sources sit behind one interface. IMU (Mini 6-axis unit) is fallback only.
- One buffered action: a swipe during jump/duck fires when the current action ends.
- Obstacles: rock (jump), spikes (jump), low pillar (duck). Telegraph before dangerous. Predefined patterns, never impossible sequences.
- Haptics: Vibrator Hat on PORT.B, pulse on collision.
- Rendering: draw each frame into a 240×240 M5Canvas and push once per frame. Draw ring background once; move objects only.
- High score in NVS via Preferences.
- All art original. No third-party sprites, names or trademarks (contest rule).

## Code layout

```
src/
  main.cpp
  config/GameConfig.h
  input/      InputManager, GestureSource, (ImuSource)
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
- Keep modules small and commented; the code is scored in the contest (BOM, schematics and code = 25 points).

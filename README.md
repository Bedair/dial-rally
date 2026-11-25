# Dial Rally

A whole arcade game in a single knob.

A top-down car race on the M5Stack Dial's round display. The road rushes toward you: turn the knob to steer through gaps in roadblocks and around slower traffic, press the Dial to jump over oil spills, and grab coins on the way. No extra sensors, no cables: just the Dial.

Entry for the Hackster.io **Junk Drawer Competition 2026** (Game & Fame).

## Hardware

| Part | Role |
|---|---|
| M5Stack Dial (ESP32-S3) | The whole console: round 240×240 display, rotary encoder (steer), front button (jump / pause), touchscreen (jump), buzzer |
| 3D-printed enclosure | Housing (no electronics) |

## Controls

| Input | Action |
|---|---|
| Turn the knob | Steer left / right, choose in menus |
| Press the Dial or tap the screen | Jump over oil / confirm |
| Hold the Dial ~0.6 s | Pause |

## Build

1. Install VS Code and the PlatformIO extension.
2. Open this folder in VS Code.
3. Connect the Dial over USB-C and run **PlatformIO: Upload**, or from a terminal:

```bash
pio run -t upload
pio device monitor
```

## Art

All sprites are original pixel art. The PNGs in `assets/sprites/` are the source; the firmware uses C++ headers generated from them (`src/display/sprites/`, committed so a plain `pio run` works).

```bash
pip install pillow
python tools/make_sprites.py   # (re)draw the original sprites -> assets/sprites/*.png
python tools/png2rgb565.py     # PNGs -> src/display/sprites/*.h (RGB565 + faint copy)
```

To change the art, edit a PNG in any pixel editor (keep its size; transparent pixels stay transparent) and run only `png2rgb565.py`. Running `make_sprites.py` again overwrites the PNGs with the drawn-in-code originals.

## Testing

Full checklist and the long-run soak test: [docs/TESTING.md](docs/TESTING.md).

```bash
test/sim/run.sh                              # host simulation: fairness + collisions, seconds
pio run -e soak -t upload                    # soak firmware: the game plays itself for hours
pio device monitor -e soak --filter log2file
```

Every obstacle sequence is generated to be reachable at half a knob turn per second (see `src/game/Patterns.cpp`); the simulation checks this with an autopilot limited to exactly that speed.

## Status

- [x] M0 Knob input (encoder, button, touch)
- [x] M1 Game skeleton (state machine, frame loop, frame-time logging)
- [x] M2 Road renderer
- [x] M3 Mechanics (collisions, jump, fair patterns)
- [x] M4 Scoring, difficulty curve, high score in NVS
- [x] M5 Sound and feedback
- [x] M6 Art pass (original sprites, PNG converter)
- [ ] M7 Validation (tools done; hardware checklist and soak run pending)
- [ ] Hackster write-up

## License

MIT

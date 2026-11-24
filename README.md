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

## Status

- [x] M0 Knob input (encoder, button, touch)
- [x] M1 Game skeleton (state machine, frame loop, frame-time logging)
- [ ] M2 Road renderer (in test)
- [ ] M3 Mechanics, M4 scoring, M5 sound and feedback, M6 art pass, M7 validation
- [ ] Hackster write-up

## License

MIT

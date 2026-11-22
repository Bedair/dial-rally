# Motion Quest – The Lost Temple

A temple-runner game built for a round screen and played without touching anything.

An explorer runs around a circular temple corridor on the M5Stack Dial's round display. Swipe up over the gesture sensor to jump, swipe down to duck. Entry for the Hackster.io **Junk Drawer Competition 2026** (Game & Fame).

## Hardware

| Part | Role |
|---|---|
| M5Stack Dial (ESP32-S3) | Game console: round 240×240 display, encoder, button, buzzer |
| M5Stack Gesture Unit (PAJ7620U2) | Jump / duck input, on PORT.A (I2C, 0x73) |
| M5Stack Vibrator Hat | Crash haptics, on PORT.B |
| 3D-printed stone-tablet enclosure | Housing, gesture sensor mounted as the "temple eye" |

## Build

1. Install VS Code and the PlatformIO extension.
2. Open this folder in VS Code.
3. Connect the Dial over USB-C and run **PlatformIO: Upload**, or from a terminal:

```bash
pio run -t upload
pio device monitor
```

## Status

- [x] Repository scaffold and bring-up sketch (display, encoder, button)
- [ ] Day 0: gesture sensor test with latency logging
- [ ] Ring-runner game engine
- [ ] Enclosure, haptics, sound
- [ ] Hackster write-up

## License

MIT

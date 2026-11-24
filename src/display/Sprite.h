#pragma once
// A sprite converted from assets/sprites/*.png by tools/png2rgb565.py.
// Pixels are RGB565 (native byte order: the canvas uses setSwapBytes(true)).
// SPRITE_KEY marks transparent pixels. `dim` is a half-brightness copy for
// drawing obstacles faint while they are only a warning (telegraph).

#include <stdint.h>

constexpr uint16_t SPRITE_KEY = 0xF81F;   // magenta

struct Sprite {
    const uint16_t* px;
    const uint16_t* dim;
    uint16_t w, h;
};

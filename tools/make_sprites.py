#!/usr/bin/env python3
"""Draw Dial Rally's original pixel-art sprites and save them as PNGs.

All art in this file is original (contest rule: no third-party sprites).
Symmetric sprites (cars) are written as their LEFT half in text, one
character per pixel, and mirrored. Round things (coins, oil, trees) are
drawn from simple shapes so they stay crisp at this tiny size.

Output: assets/sprites/*.png (transparent background). The PNGs are the
source the firmware is built from: edit them in any pixel editor, or change
this script and run it again, then run tools/png2rgb565.py.

    python tools/make_sprites.py
"""
from pathlib import Path
import math

from PIL import Image

OUT = Path(__file__).resolve().parent.parent / "assets" / "sprites"

# ---------------------------------------------------------------------------
# Text pixel art
# ---------------------------------------------------------------------------
# Shared palette letters. '.' is transparent. Per-sprite letters (body colour)
# are added by the caller.
BASE = {
    "k": (16, 16, 20),     # outline
    "w": (245, 245, 245),  # white
    "g": (28, 42, 78),     # glass
    "G": (110, 150, 210),  # glass highlight
    "t": (22, 22, 22),     # tyre
    "y": (255, 240, 150),  # headlight
    "o": (255, 70, 40),    # tail light
    "s": (130, 130, 140),  # grey (spoiler)
}


def from_text(rows, palette, mirror=True):
    """Rows of text -> RGBA image. With mirror=True each row is the left half."""
    if mirror:
        rows = [r + r[::-1] for r in rows]
    w = len(rows[0])
    for i, r in enumerate(rows):
        assert len(r) == w, f"row {i} is {len(r)} px wide, expected {w}: {r!r}"
    img = Image.new("RGBA", (w, len(rows)), (0, 0, 0, 0))
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch != ".":
                img.putpixel((x, y), palette[ch] + (255,))
    return img


# Player: low sports car, red with a white racing stripe and a rear spoiler.
# 20 x 30, nose up. Wheels stick out 2 px on each side (the hit box is the
# 18 px body width minus HITBOX_MARGIN, see GameConfig.h).
PLAYER_LEFT = [
    "....kkkkkk",
    "...kyprrww",
    "...kprrrww",
    "..kprrrrww",
    "..krrrrrww",
    "ttkrrrrrww",
    "ttkRgggggg",
    "ttkRgGgggg",
    "ttkRgGgggg",
    "..kRrggggg",
    "..krrrrrww",
    "..krprrrww",
    "..krprrrww",
    "..krprrrww",
    "..krprrrww",
    "..krrrrrww",
    "..kRgggggg",
    "..kRgGgggg",
    "..krrrrrww",
    "ttkrrrrrww",
    "ttkrrrrrww",
    "ttkrrrrrww",
    "ttkrrrrrww",
    "..krrrrrww",
    "..kRRRRRww",
    ".kkkkkkkkk",
    ".kssssssss",
    ".kkkkkkkkk",
    "...koRRRRR",
    "....kkkkkk",
]
PLAYER_COLOURS = {
    "r": (232, 30, 36),    # body
    "R": (150, 16, 22),    # body shade
    "p": (255, 120, 105),  # body highlight
}

# Traffic: boxier saloon with a long roof, no stripes. 20 x 30.
TRAFFIC_LEFT = [
    "...kkkkkkk",
    "..kybbbbbb",
    "..kbbbbbbb",
    "..kbbbbbbb",
    "ttkbbbbbbb",
    "ttkbBBBBBB",
    "ttkbgggggg",
    "ttkbgGgggg",
    "..kbgGgggg",
    "..kbbggggg",
    "..kbbbbbbb",
    "..kbHbbbbb",
    "..kbHbbbbb",
    "..kbHbbbbb",
    "..kbHbbbbb",
    "..kbHbbbbb",
    "..kbbbbbbb",
    "..kbgggggg",
    "..kbgGgggg",
    "..kbbbbbbb",
    "ttkbbbbbbb",
    "ttkbbbbbbb",
    "ttkbbbbbbb",
    "ttkbbbbbbb",
    "..kbbbbbbb",
    "..kBBBBBBB",
    "..kobbbbbb",
    "..kobbbbbb",
    "...kkkkkkk",
    "..........",
]
TRAFFIC_COLOURS = {
    "blue":   {"b": (45, 115, 250), "B": (25, 70, 170), "H": (140, 185, 255)},
    "yellow": {"b": (250, 200, 40), "B": (180, 135, 20), "H": (255, 235, 150)},
    "green":  {"b": (60, 190, 85),  "B": (32, 118, 52), "H": (150, 235, 160)},
}

# ---------------------------------------------------------------------------
# Shape-drawn sprites
# ---------------------------------------------------------------------------
def coin_frame(squash):
    """12 x 12 gold coin seen at an angle: squash 1 = facing, ~0 = edge-on."""
    size = 12
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    c = (size - 1) / 2
    rx, ry = max(0.8, 5.5 * squash), 5.5
    for y in range(size):
        for x in range(size):
            dx, dy = (x - c) / rx, (y - c) / ry
            d = dx * dx + dy * dy
            if d > 1.0:
                continue
            if d > 0.62:
                col = (180, 120, 10)            # rim
            elif dx < -0.15 and dy < -0.15:
                col = (255, 250, 200)           # highlight
            else:
                col = (255, 205, 40)            # face
            img.putpixel((x, y), col + (255,))
    return img


def oil_tile(seed):
    """22 x 18 oil blob made of overlapping ellipses, with a purple sheen."""
    w, h = 22, 18
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    # (cx, cy, rx, ry) blobs; two seeds give two tiles that alternate.
    blobs = [(6, 9, 6, 6), (13, 8, 8, 7), (17, 11, 5, 5)] if seed == 0 else \
            [(5, 8, 5, 6), (11, 10, 8, 6), (17, 8, 5, 6)]
    for y in range(h):
        for x in range(w):
            inside = any(((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1 for cx, cy, rx, ry in blobs)
            if inside:
                img.putpixel((x, y), (18, 16, 26, 255))
    sheen = [(8, 6), (9, 6), (10, 7), (14, 11), (15, 11)] if seed == 0 else \
            [(6, 6), (7, 6), (12, 8), (13, 8), (14, 9)]
    for x, y in sheen:
        img.putpixel((x, y), (110, 80, 170, 255))
    return img


def barrier_tile():
    """12 x 10 roadblock segment: red/white diagonal stripes, dark border.
    The stripe period (6 px) divides the width, so tiles join seamlessly."""
    w, h = 12, 10
    img = Image.new("RGBA", (w, h), (0, 0, 0, 255))
    for y in range(1, h - 1):
        for x in range(w):
            red = ((x + y) // 3) % 2 == 0
            img.putpixel((x, y), ((235, 30, 30) if red else (250, 250, 250)) + (255,))
    return img


def tree(r, seed):
    """Round tree crown seen from above with a soft shadow, size 2r+3."""
    size = 2 * r + 3
    img = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    c = r
    for y in range(size):
        for x in range(size):
            # shadow, offset down-right
            if (x - c - 2) ** 2 + (y - c - 2) ** 2 <= r * r:
                img.putpixel((x, y), (20, 50, 20, 255))
    for y in range(size):
        for x in range(size):
            d = math.hypot(x - c, y - c)
            if d <= r:
                lit = (x - c) + (y - c) < -r * 0.4
                bump = ((x * 7 + y * 13 + seed) % 5) == 0
                col = (95, 175, 70) if lit else (45, 120, 45)
                if bump:
                    col = (35, 95, 35)
                img.putpixel((x, y), col + (255,))
    return img


# ---------------------------------------------------------------------------
def main():
    OUT.mkdir(parents=True, exist_ok=True)
    sprites = {
        "car_player": from_text(PLAYER_LEFT, {**BASE, **PLAYER_COLOURS}),
        "oil_a": oil_tile(0),
        "oil_b": oil_tile(1),
        "barrier": barrier_tile(),
        "tree": tree(7, 1),
        "bush": tree(4, 3),
    }
    for name, colours in TRAFFIC_COLOURS.items():
        sprites[f"car_{name}"] = from_text(TRAFFIC_LEFT, {**BASE, **colours})
    for i, squash in enumerate([1.0, 0.6, 0.15, 0.6]):
        sprites[f"coin_{i}"] = coin_frame(squash)

    for name, img in sprites.items():
        path = OUT / f"{name}.png"
        img.save(path)
        print(f"{path.relative_to(OUT.parent.parent)}  {img.width}x{img.height}")


if __name__ == "__main__":
    main()

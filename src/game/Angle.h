#pragma once
// Angle helpers. Convention for the whole game: degrees, 0° = 12 o'clock,
// clockwise positive, stored in 0..360.

#include <math.h>

inline float wrap360(float a) {
    a = fmodf(a, 360.0f);
    return a < 0 ? a + 360.0f : a;
}

// Signed shortest difference b - a, in -180..180.
inline float angleDiff(float a, float b) {
    float d = wrap360(b - a);
    return d > 180.0f ? d - 360.0f : d;
}

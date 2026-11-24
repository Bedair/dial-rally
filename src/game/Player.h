#pragma once
// The explorer on the rim. The knob moves the target angle; the drawn angle
// eases toward it so movement looks smooth even though the knob has detents.
// Collisions (M3) use targetAngle(), drawing uses drawAngle().

#include <stdint.h>

class Player {
public:
    void reset(float angleDeg = 0.0f);
    void turn(float deltaDeg);          // from Action::Turn
    void update(uint32_t dtMs);         // easing

    float targetAngle() const { return target_; }
    float drawAngle() const { return draw_; }

private:
    float target_ = 0.0f;
    float draw_ = 0.0f;
};

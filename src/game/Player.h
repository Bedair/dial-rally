#pragma once
// The player's car. The knob moves the target x position; the drawn x eases
// toward it so steering looks smooth even though the knob has detents.
// Collisions (M3) use targetX(), drawing uses drawX(). The car's y is fixed
// (CAR_Y): the road moves, not the car.

#include <stdint.h>

class Player {
public:
    void reset();                      // centre of the road
    void steer(float deltaPx);         // from Action::Turn, clamped to the road
    void update(uint32_t dtMs);        // easing

    float targetX() const { return target_; }
    float drawX() const { return draw_; }

private:
    float target_ = 0.0f;
    float draw_ = 0.0f;
};

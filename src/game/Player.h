#pragma once
// The player's car. The knob moves the target x position; the drawn x eases
// toward it so steering looks smooth even though the knob has detents.
// Collisions use targetX() (what the knob says), drawing uses drawX(); the
// difference is a few px for a few frames. The car's y is fixed (CAR_Y):
// the road moves, not the car.
//
// Jump: lasts JUMP_MS. A press while airborne is not lost: one press is
// buffered and starts the next jump the moment the current one lands.

#include <stdint.h>

class Player {
public:
    void reset();                      // centre of the road, on the ground
    void steer(float deltaPx);         // from Action::Turn, clamped to the road
    void jump();                       // from Action::Press while playing
    void update(uint32_t dtMs);        // easing + jump timer

    float targetX() const { return target_; }
    float drawX() const { return draw_; }
    bool airborne() const { return jumping_; }
    // 0 on the ground, rising to 1 at the top of the jump and back to 0.
    float jumpHeight() const;

private:
    float target_ = 0.0f;
    float draw_ = 0.0f;
    bool jumping_ = false;
    bool jumpBuffered_ = false;
    uint32_t jumpMs_ = 0;              // time since the current jump started
};

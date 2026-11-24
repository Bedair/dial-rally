#include "game/Player.h"
#include "config/GameConfig.h"
#include <math.h>

namespace {
constexpr float MIN_X = cfg::ROAD_LEFT + cfg::KERB_W + cfg::CAR_W / 2.0f;
constexpr float MAX_X = cfg::ROAD_RIGHT - cfg::KERB_W - cfg::CAR_W / 2.0f;
constexpr float PI_F = 3.14159265f;
}

void Player::reset() {
    target_ = draw_ = (cfg::ROAD_LEFT + cfg::ROAD_RIGHT) / 2.0f;
    jumping_ = jumpBuffered_ = false;
    jumpMs_ = 0;
}

// Clamping the target (not just the drawing) means turning back from the
// edge responds immediately, with no "dead" turning to undo.
void Player::steer(float deltaPx) {
    target_ += deltaPx;
    if (target_ < MIN_X) target_ = MIN_X;
    if (target_ > MAX_X) target_ = MAX_X;
}

void Player::jump() {
    if (!jumping_) {
        jumping_ = true;
        jumpMs_ = 0;
    } else {
        jumpBuffered_ = true;          // at most one, extra presses are ignored
    }
}

void Player::update(uint32_t dtMs) {
    // 1 - e^(-dt/tau) makes the easing independent of the frame rate.
    const float k = 1.0f - expf(-(float)dtMs / cfg::EASE_TAU_MS);
    const float d = target_ - draw_;
    draw_ = fabsf(d) < 0.05f ? target_ : draw_ + d * k;

    if (jumping_) {
        jumpMs_ += dtMs;
        if (jumpMs_ >= cfg::JUMP_MS) {
            jumping_ = jumpBuffered_;  // buffered press: take off again at once
            jumpBuffered_ = false;
            jumpMs_ = 0;
        }
    }
}

float Player::jumpHeight() const {
    if (!jumping_) return 0.0f;
    return sinf(PI_F * jumpMs_ / cfg::JUMP_MS);
}

#include "game/Player.h"
#include "game/Angle.h"
#include "config/GameConfig.h"

void Player::reset(float angleDeg) {
    target_ = draw_ = wrap360(angleDeg);
}

void Player::turn(float deltaDeg) {
    target_ = wrap360(target_ + deltaDeg);
}

// 1 - e^(-dt/tau) makes the easing independent of the frame rate.
void Player::update(uint32_t dtMs) {
    const float k = 1.0f - expf(-(float)dtMs / cfg::EASE_TAU_MS);
    const float d = angleDiff(draw_, target_);
    draw_ = fabsf(d) < 0.05f ? target_ : wrap360(draw_ + d * k);
}

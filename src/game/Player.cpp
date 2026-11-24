#include "game/Player.h"
#include "config/GameConfig.h"
#include <math.h>

namespace {
constexpr float MIN_X = cfg::ROAD_LEFT + cfg::KERB_W + cfg::CAR_W / 2.0f;
constexpr float MAX_X = cfg::ROAD_RIGHT - cfg::KERB_W - cfg::CAR_W / 2.0f;
}

void Player::reset() {
    target_ = draw_ = (cfg::ROAD_LEFT + cfg::ROAD_RIGHT) / 2.0f;
}

// Clamping the target (not just the drawing) means turning back from the
// edge responds immediately, with no "dead" turning to undo.
void Player::steer(float deltaPx) {
    target_ += deltaPx;
    if (target_ < MIN_X) target_ = MIN_X;
    if (target_ > MAX_X) target_ = MAX_X;
}

// 1 - e^(-dt/tau) makes the easing independent of the frame rate.
void Player::update(uint32_t dtMs) {
    const float k = 1.0f - expf(-(float)dtMs / cfg::EASE_TAU_MS);
    const float d = target_ - draw_;
    draw_ = fabsf(d) < 0.05f ? target_ : draw_ + d * k;
}

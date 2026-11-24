#include "game/Hazard.h"
#include "config/GameConfig.h"

void HazardField::clear() {
    for (Hazard& h : pool_) h.active = false;
}

Hazard* HazardField::spawn(HazardType type, float x, float w) {
    for (Hazard& h : pool_) {
        if (h.active) continue;
        h = Hazard{};
        h.active = true;
        h.type = type;
        h.y = cfg::OBSTACLE_SPAWN_Y;
        h.x = x;
        h.w = w;
        return &h;
    }
    return nullptr;
}

int HazardField::update(uint32_t dtMs, float scrollPxPerS, float trafficPxPerS) {
    int passedCar = 0;
    const float dt = dtMs / 1000.0f;
    for (Hazard& h : pool_) {
        if (!h.active) continue;
        const float speed = h.type == HazardType::Traffic ? scrollPxPerS - trafficPxPerS
                                                          : scrollPxPerS;
        h.y += speed * dt;
        h.armed = h.y >= cfg::OBSTACLE_ARM_Y;
        if (!h.passed && h.y >= cfg::CAR_Y) {
            h.passed = true;
            if (h.type != HazardType::Coin) passedCar++;
        }
        if (h.y > cfg::OBSTACLE_REMOVE_Y) h.active = false;
    }
    return passedCar;
}

#include "game/Hazard.h"
#include "game/Difficulty.h"
#include "config/GameConfig.h"

void HazardField::clear() {
    for (Hazard& h : pool_) h.active = false;
}

Hazard* HazardField::spawn(const Hazard& proto, uint32_t tMs, float dist) {
    for (Hazard& h : pool_) {
        if (h.active) continue;
        h = proto;
        h.active = true;
        h.passed = false;
        h.y = difficulty::obstacleY(h.type, h.arrivalMs, h.arrivalDist, tMs, dist);
        h.armed = h.y >= cfg::OBSTACLE_ARM_Y;
        return &h;
    }
    return nullptr;
}

int HazardField::update(uint32_t tMs, float dist) {
    int passedCar = 0;
    for (Hazard& h : pool_) {
        if (!h.active) continue;
        h.y = difficulty::obstacleY(h.type, h.arrivalMs, h.arrivalDist, tMs, dist);
        h.armed = h.y >= cfg::OBSTACLE_ARM_Y;
        if (!h.passed && h.y >= cfg::CAR_Y) {
            h.passed = true;
            if (h.type != HazardType::Coin) passedCar++;
        }
        if (h.y > cfg::OBSTACLE_REMOVE_Y) h.active = false;
    }
    return passedCar;
}

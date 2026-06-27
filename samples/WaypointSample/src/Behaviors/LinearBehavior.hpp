#pragma once
#include "Behavior.hpp"
#include "../Tank.hpp"

namespace WaypointSample {

class LinearBehavior : public Behavior {
public:
    explicit LinearBehavior(Tank& tank) : Behavior(tank) {}

    void Update(const GameTime& /*gameTime*/) override {
        Vector2 dir = Vector2(
            tank_.Waypoints().Peek().X - tank_.Location().X,
            tank_.Waypoints().Peek().Y - tank_.Location().Y);
        dir.Normalize();
        tank_.SetDirection(dir);
    }
};

} // namespace WaypointSample

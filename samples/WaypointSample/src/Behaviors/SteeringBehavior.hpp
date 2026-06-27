#pragma once
#include <algorithm>
#include <cmath>
#include "Behavior.hpp"
#include "../Tank.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

namespace WaypointSample {

using namespace Microsoft::Xna::Framework;

class SteeringBehavior : public Behavior {
public:
    explicit SteeringBehavior(Tank& tank) : Behavior(tank) {}

    void Update(const GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        float prev    = tank_.MoveSpeed();
        float desired = FindMaxMoveSpeed(tank_.Waypoints().Peek());
        tank_.SetMoveSpeed(MathHelper::Clamp(desired,
            prev - Tank::MaxMoveSpeedDelta * elapsed,
            prev + Tank::MaxMoveSpeedDelta * elapsed));

        float facing = std::atan2(tank_.Direction().Y, tank_.Direction().X);
        facing = TurnToFace(tank_.Location(), tank_.Waypoints().Peek(),
                            facing, Tank::MaxAngularVelocity * elapsed);
        tank_.SetDirection(Vector2(std::cos(facing), std::sin(facing)));
    }

private:
    float FindMaxMoveSpeed(Vector2 waypoint) {
        float turningRadius = Tank::MaxMoveSpeed / Tank::MaxAngularVelocity;
        Vector2 orth(tank_.Direction().Y, -tank_.Direction().X);
        float closest = std::min(
            Vector2::Distance(waypoint, Vector2(
                tank_.Location().X + orth.X * turningRadius,
                tank_.Location().Y + orth.Y * turningRadius)),
            Vector2::Distance(waypoint, Vector2(
                tank_.Location().X - orth.X * turningRadius,
                tank_.Location().Y - orth.Y * turningRadius)));

        if (closest < turningRadius) {
            float radius = Vector2::Distance(tank_.Location(), waypoint) / 2.0f;
            return Tank::MaxAngularVelocity * radius;
        }
        return Tank::MaxMoveSpeed;
    }

    static float TurnToFace(Vector2 pos, Vector2 target,
                             float currentAngle, float turnSpeed) {
        float desired = std::atan2(target.Y - pos.Y, target.X - pos.X);
        float diff    = WrapAngle(desired - currentAngle);
        diff = MathHelper::Clamp(diff, -turnSpeed, turnSpeed);
        return WrapAngle(currentAngle + diff);
    }

    static float WrapAngle(float r) {
        while (r < -MathHelper::Pi) r += MathHelper::TwoPi;
        while (r >  MathHelper::Pi) r -= MathHelper::TwoPi;
        return r;
    }
};

} // namespace WaypointSample

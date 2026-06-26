#pragma once
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

namespace Platformer {

using namespace Microsoft::Xna::Framework;

struct Circle {
    Vector2 Center;
    float Radius;

    Circle(Vector2 position, float radius) : Center(position), Radius(radius) {}

    bool Intersects(Rectangle rectangle) const {
        Vector2 v(
            MathHelper::Clamp(Center.X, (float)rectangle.getLeftProperty(), (float)rectangle.getRightProperty()),
            MathHelper::Clamp(Center.Y, (float)rectangle.getTopProperty(), (float)rectangle.getBottomProperty())
        );
        Vector2 direction = Center - v;
        float distanceSquared = direction.LengthSquared();
        return (distanceSquared > 0) && (distanceSquared < Radius * Radius);
    }
};

} // namespace Platformer

#pragma once
#include <cmath>
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

namespace Platformer {

using namespace Microsoft::Xna::Framework;

struct RectangleExtensions {
    static Vector2 GetIntersectionDepth(Rectangle rectA, Rectangle rectB) {
        float halfWidthA  = rectA.Width  / 2.0f;
        float halfHeightA = rectA.Height / 2.0f;
        float halfWidthB  = rectB.Width  / 2.0f;
        float halfHeightB = rectB.Height / 2.0f;

        Vector2 centerA((float)rectA.getLeftProperty() + halfWidthA,
                        (float)rectA.getTopProperty()  + halfHeightA);
        Vector2 centerB((float)rectB.getLeftProperty() + halfWidthB,
                        (float)rectB.getTopProperty()  + halfHeightB);

        float distanceX    = centerA.X - centerB.X;
        float distanceY    = centerA.Y - centerB.Y;
        float minDistanceX = halfWidthA  + halfWidthB;
        float minDistanceY = halfHeightA + halfHeightB;

        if (std::abs(distanceX) >= minDistanceX || std::abs(distanceY) >= minDistanceY)
            return Vector2::Zero;

        float depthX = distanceX > 0 ? minDistanceX - distanceX : -minDistanceX - distanceX;
        float depthY = distanceY > 0 ? minDistanceY - distanceY : -minDistanceY - distanceY;
        return Vector2(depthX, depthY);
    }

    static Vector2 GetBottomCenter(Rectangle rect) {
        return Vector2(rect.X + rect.Width / 2.0f, (float)rect.getBottomProperty());
    }
};

} // namespace Platformer

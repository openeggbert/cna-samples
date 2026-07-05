#pragma once

// MathUtility.hpp -- C++ port of Utils/MathUtility.cs (XNA 4.0 CardsStarterKit sample).

#include <cmath>

#include "Microsoft/Xna/Framework/Vector2.hpp"

namespace CardsFramework {

using Microsoft::Xna::Framework::Vector2;

class MathUtility {
public:
    // Rotates a point around another specified point (rotation in radians).
    static Vector2 RotateAboutOrigin(Vector2 point, Vector2 origin, float rotation) {
        Vector2 u = point - origin;

        if (u == Vector2::Zero)
            return point;

        float a = std::atan2(u.Y, u.X);
        a += rotation;

        float len = u.Length();
        u = Vector2(len * std::cos(a), len * std::sin(a));
        return u + origin;
    }
};

} // namespace CardsFramework

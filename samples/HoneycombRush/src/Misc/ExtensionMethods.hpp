#pragma once

// ExtensionMethods.hpp — C++ port of Misc/ExtensionMethods.cs (XNA 4.0
// HoneycombRush sample). C++ has no extension methods, so these become plain
// free functions; call sites read `GetVector(rect)` instead of `rect.GetVector()`.

#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;

inline Vector2 GetVector(const Rectangle& rect) {
    return Vector2((float)rect.X, (float)rect.Y);
}

inline Vector2 GetVector(const Point& point) {
    return Vector2((float)point.X, (float)point.Y);
}

inline bool HasCollision(const Rectangle& first, const Rectangle& second) {
    return first.Intersects(second);
}

} // namespace HoneycombRush

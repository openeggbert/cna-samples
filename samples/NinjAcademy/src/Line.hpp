#pragma once

// Line.hpp — C++ port of Utility/Line.cs (XNA 4.0 NinjAcademy sample).
// A finite line segment between two points, used for sword-slash/bamboo
// intersection checks.

#include <optional>

#include "Microsoft/Xna/Framework/Vector2.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Vector2;

// A finite line between two points. Port of Utility/Line.cs.
struct Line {
    Vector2 Start;
    Vector2 End;

    Line() = default;
    Line(Vector2 start, Vector2 end) : Start(start), End(end) {}

    // Returns the intersection point with another line segment, or nullopt if
    // there is none.
    std::optional<Vector2> GetIntersection(const Line& otherLine) const {
        float numeratorA = (otherLine.End.X - otherLine.Start.X) * (Start.Y - otherLine.Start.Y) -
                            (otherLine.End.Y - otherLine.Start.Y) * (Start.X - otherLine.Start.X);
        float numeratorB = (End.X - Start.X) * (Start.Y - otherLine.Start.Y) -
                            (End.Y - Start.Y) * (Start.X - otherLine.Start.X);
        float denominator = (otherLine.End.Y - otherLine.Start.Y) * (End.X - Start.X) -
                             (otherLine.End.X - otherLine.Start.X) * (End.Y - Start.Y);

        if (denominator == 0.0f) {
            // Lines are parallel (and possibly coincide)
            return std::nullopt;
        }

        float uA = numeratorA / denominator;
        float uB = numeratorB / denominator;

        if (uA < 0.0f || uA > 1.0f || uB < 0.0f || uB > 1.0f) {
            // The intersection is outside one of the line segments
            return std::nullopt;
        }

        return Vector2(Start.X + uA * (End.X - Start.X), Start.Y + uA * (End.Y - Start.Y));
    }
};

} // namespace NinjAcademy

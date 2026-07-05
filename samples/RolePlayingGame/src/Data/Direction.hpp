#pragma once

// Direction.hpp -- C++ port of RolePlayingGameData/Direction.cs.

#include <string>

namespace RolePlayingGameData {

enum class Direction {
    North,
    NorthEast,
    East,
    SouthEast,
    South,
    SouthWest,
    West,
    NorthWest,
};

inline const char* ToString(Direction d) {
    switch (d) {
        case Direction::North: return "North";
        case Direction::NorthEast: return "NorthEast";
        case Direction::East: return "East";
        case Direction::SouthEast: return "SouthEast";
        case Direction::South: return "South";
        case Direction::SouthWest: return "SouthWest";
        case Direction::West: return "West";
        case Direction::NorthWest: return "NorthWest";
    }
    return "South";
}

inline Direction DirectionFromString(const std::string& s) {
    if (s == "North") return Direction::North;
    if (s == "NorthEast") return Direction::NorthEast;
    if (s == "East") return Direction::East;
    if (s == "SouthEast") return Direction::SouthEast;
    if (s == "South") return Direction::South;
    if (s == "SouthWest") return Direction::SouthWest;
    if (s == "West") return Direction::West;
    if (s == "NorthWest") return Direction::NorthWest;
    return Direction::South;
}

} // namespace RolePlayingGameData

#pragma once

// PlayerPosition.hpp -- C++ port of TileEngine/PlayerPosition.cs.

#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "../Data/Direction.hpp"

namespace RolePlaying {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Vector2;

class TileEngine; // fwd decl -- see TileEngine.hpp

// The position of a player in the tile engine. Players are the only objects
// that move between tiles.
class PlayerPosition {
public:
    Point TilePosition;
    Vector2 TileOffset;

    // Defined out-of-line in TileEngine.hpp (needs TileEngine::Map/GetScreenPosition).
    Vector2 ScreenPosition() const;

    RolePlayingGameData::Direction PositionDirection = RolePlayingGameData::Direction::South;

    bool IsMoving() const { return isMoving_; }

    void Move(Vector2 movement) {
        isMoving_ = (movement != Vector2::Zero);
        CalculateMovement(movement, TilePosition, TileOffset);
        if (isMoving_) PositionDirection = CalculateDirection(movement);
    }

    // Defined out-of-line in TileEngine.hpp (needs TileEngine::Map for tile size).
    static void CalculateMovement(Vector2 movement, Point& tilePosition, Vector2& tileOffset);

    static RolePlayingGameData::Direction CalculateDirection(Vector2 vector) {
        using RolePlayingGameData::Direction;
        if (vector.X > 0) {
            if (vector.Y > 0) return Direction::SouthEast;
            if (vector.Y < 0) return Direction::NorthEast;
            return Direction::East;
        } else if (vector.X < 0) {
            if (vector.Y > 0) return Direction::SouthWest;
            if (vector.Y < 0) return Direction::NorthWest;
            return Direction::West;
        } else {
            if (vector.Y > 0) return Direction::South;
            if (vector.Y < 0) return Direction::North;
        }
        return Direction::South;
    }

private:
    bool isMoving_ = false;
};

} // namespace RolePlaying

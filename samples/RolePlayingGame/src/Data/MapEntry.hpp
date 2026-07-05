#pragma once

// MapEntry.hpp -- C++ port of RolePlayingGameData/MapEntry.cs.

#include "ContentEntry.hpp"
#include "Direction.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"

namespace RolePlayingGameData {

class AnimatingSprite;

// The description of where an instance of a world object is in the world.
template <typename T>
class MapEntry : public ContentEntry<T> {
public:
    Microsoft::Xna::Framework::Point MapPosition;
    Direction EntryDirection = Direction::South;

    // Only used when there might be several of the same WorldObject in the scene at once.
    std::shared_ptr<AnimatingSprite> MapSprite;
};

} // namespace RolePlayingGameData

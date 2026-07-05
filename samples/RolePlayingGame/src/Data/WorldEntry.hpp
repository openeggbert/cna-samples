#pragma once

// WorldEntry.hpp -- C++ port of RolePlayingGameData/WorldEntry.cs.

#include <string>

#include "MapEntry.hpp"

namespace RolePlayingGameData {

// A description of a piece of content, including the name of the map it's on.
template <typename T>
class WorldEntry : public MapEntry<T> {
public:
    std::string MapContentName;
};

} // namespace RolePlayingGameData

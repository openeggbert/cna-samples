#pragma once

// FixedCombat.hpp -- C++ port of RolePlayingGameData/Map/FixedCombat.cs.

#include <memory>
#include <vector>

#include "../ContentEntry.hpp"
#include "../Characters/Monster.hpp"
#include "../WorldObject.hpp"

namespace RolePlayingGameData {

// The description of a fixed combat encounter in the world.
class FixedCombat : public WorldObject {
public:
    std::vector<std::shared_ptr<ContentEntry<Monster>>> Entries;
};

} // namespace RolePlayingGameData

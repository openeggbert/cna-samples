#pragma once

// StoreCategory.hpp -- C++ port of RolePlayingGameData/Map/StoreCategory.cs.

#include <memory>
#include <string>
#include <vector>

#include "../Gear/Gear.hpp"

namespace RolePlayingGameData {

// A category of gear for sale in a store.
class StoreCategory {
public:
    std::string Name;
    std::vector<std::string> AvailableContentNames;
    std::vector<std::shared_ptr<Gear>> AvailableGear;
};

} // namespace RolePlayingGameData

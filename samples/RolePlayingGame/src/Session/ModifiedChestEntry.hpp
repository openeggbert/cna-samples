#pragma once

// ModifiedChestEntry.hpp -- C++ port of Session/ModifiedChestEntry.cs.
// SaveGame serialization is dropped (see missing.md); this is now purely an
// in-session (in-memory) record of a chest's modified-but-not-emptied contents.

#include <memory>
#include <vector>

#include "../Data/ContentEntry.hpp"
#include "../Data/Gear/Gear.hpp"
#include "../Data/Map/Chest.hpp"
#include "../Data/WorldEntry.hpp"

namespace RolePlaying {

class ModifiedChestEntry {
public:
    RolePlayingGameData::WorldEntry<RolePlayingGameData::Chest> WorldEntry;
    std::vector<std::shared_ptr<RolePlayingGameData::ContentEntry<RolePlayingGameData::Gear>>> ChestEntries;
    int Gold = 0;
};

} // namespace RolePlaying

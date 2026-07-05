#pragma once

// GameStartDescription.hpp -- C++ port of RolePlayingGameData/GameStartDescription.cs.

#include <string>
#include <vector>

namespace RolePlayingGameData {

// The data needed to start a new game.
class GameStartDescription {
public:
    std::string MapContentName;
    std::vector<std::string> PlayerContentNames;
    // The first quest will be started before the world is shown.
    std::string QuestLineContentName;
};

} // namespace RolePlayingGameData

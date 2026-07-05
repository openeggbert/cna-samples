#pragma once

// QuestNpc.hpp -- C++ port of RolePlayingGameData/Characters/QuestNpc.cs.

#include <string>

#include "Character.hpp"

namespace RolePlayingGameData {

class QuestNpc : public Character {
public:
    std::string IntroductionDialogue;
};

} // namespace RolePlayingGameData

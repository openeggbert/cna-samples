#pragma once

// CharacterLevelDescription.hpp -- C++ port of
// RolePlayingGameData/Characters/CharacterLevelDescription.cs.

#include <memory>
#include <string>
#include <vector>

#include "../Spell.hpp"

namespace RolePlayingGameData {

class CharacterLevelDescription {
public:
    int ExperiencePoints = 0;
    std::vector<std::string> SpellContentNames;
    std::vector<std::shared_ptr<Spell>> Spells;
};

} // namespace RolePlayingGameData

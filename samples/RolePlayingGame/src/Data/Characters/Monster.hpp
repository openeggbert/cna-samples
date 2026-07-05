#pragma once

// Monster.hpp -- C++ port of RolePlayingGameData/Characters/Monster.cs.

#include <vector>

#include "../Gear/GearDrop.hpp"
#include "FightingCharacter.hpp"

namespace RolePlayingGameData {

class Monster : public FightingCharacter {
public:
    int DefendPercentage = 0;
    std::vector<GearDrop> GearDrops;

    int CalculateGoldReward(System::Random& random) const {
        return GetCharacterClass()->BaseGoldValue * CharacterLevel();
    }

    int CalculateExperienceReward(System::Random& random) const {
        return GetCharacterClass()->BaseExperienceValue * CharacterLevel();
    }

    std::vector<std::string> CalculateGearDrop(System::Random& random) const {
        std::vector<std::string> gearNames;
        for (auto& drop : GearDrops) {
            if (random.Next(0, 100) < drop.DropPercentage()) gearNames.push_back(drop.GearName);
        }
        return gearNames;
    }
};

} // namespace RolePlayingGameData

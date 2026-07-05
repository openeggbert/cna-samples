#pragma once

// CharacterLevelingStatistics.hpp -- C++ port of
// RolePlayingGameData/Characters/CharacterLevelingStatistics.cs.

namespace RolePlayingGameData {

struct CharacterLevelingStatistics {
    int HealthPointsIncrease = 0;
    int LevelsPerHealthPointsIncrease = 0;
    int MagicPointsIncrease = 0;
    int LevelsPerMagicPointsIncrease = 0;
    int PhysicalOffenseIncrease = 0;
    int LevelsPerPhysicalOffenseIncrease = 0;
    int PhysicalDefenseIncrease = 0;
    int LevelsPerPhysicalDefenseIncrease = 0;
    int MagicalOffenseIncrease = 0;
    int LevelsPerMagicalOffenseIncrease = 0;
    int MagicalDefenseIncrease = 0;
    int LevelsPerMagicalDefenseIncrease = 0;
};

} // namespace RolePlayingGameData

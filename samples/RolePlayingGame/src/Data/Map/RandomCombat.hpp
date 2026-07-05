#pragma once

// RandomCombat.hpp -- C++ port of RolePlayingGameData/Map/RandomCombat.cs.

#include <memory>
#include <vector>

#include "../Characters/Monster.hpp"
#include "../Int32Range.hpp"
#include "../WeightedContentEntry.hpp"

namespace RolePlayingGameData {

// Description of possible random combats in a particular map.
class RandomCombat {
public:
    // The chance of a random combat starting with each step, from 1 to 100.
    int CombatProbability = 0;
    // The chance of a successful escape from a random combat, from 1 to 100.
    int FleeProbability = 0;
    Int32Range MonsterCountRange;
    std::vector<std::shared_ptr<WeightedContentEntry<Monster>>> Entries;
};

} // namespace RolePlayingGameData

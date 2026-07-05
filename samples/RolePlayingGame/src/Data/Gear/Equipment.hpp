#pragma once

// Equipment.hpp -- C++ port of RolePlayingGameData/Gear/Equipment.cs.

#include "../StatisticsValue.hpp"
#include "Gear.hpp"

namespace RolePlayingGameData {

// Gear that may be equipped onto a FightingCharacter.
class Equipment : public Gear {
public:
    ~Equipment() override = default;

    // Buff values are positive, and will be added.
    StatisticsValue OwnerBuffStatistics;
};

} // namespace RolePlayingGameData

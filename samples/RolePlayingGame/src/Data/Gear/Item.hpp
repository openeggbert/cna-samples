#pragma once

// Item.hpp -- C++ port of RolePlayingGameData/Gear/Item.cs.

#include <memory>
#include <string>

#include "../AnimatingSprite.hpp"
#include "../StatisticsRange.hpp"
#include "Gear.hpp"

namespace RolePlayingGameData {

// A usable piece of gear that has a spell-like effect.
class Item : public Gear {
public:
    ~Item() override = default;

    // Flags that specify when an item may be used.
    enum ItemUsage { Combat = 1, NonCombat = 2 };

    // Defaults to "either", with both values.
    int Usage = Combat | NonCombat;

    // If true, the statistics change are used as a debuff (subtracted).
    // Otherwise, the statistics change is used as a buff (added).
    bool IsOffensive = false;

    // If the duration is zero, then the effects last for the rest of the battle.
    int TargetDuration = 0;

    // This is a debuff if IsOffensive is true, otherwise it's a buff.
    StatisticsRange TargetEffectRange;

    int AdjacentTargets = 0;

    std::string UsingCueName;
    std::string TravelingCueName;
    std::string ImpactCueName;
    std::string BlockCueName;

    // Optional. If null, then a Using or Creating animation in SpellSprite is used.
    std::shared_ptr<AnimatingSprite> CreationSprite;
    std::shared_ptr<AnimatingSprite> SpellSprite;
    std::shared_ptr<AnimatingSprite> Overlay;

    std::string GetPowerText() const override { return TargetEffectRange.GetModifierString(); }
};

} // namespace RolePlayingGameData

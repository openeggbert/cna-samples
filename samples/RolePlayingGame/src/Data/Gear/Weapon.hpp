#pragma once

// Weapon.hpp -- C++ port of RolePlayingGameData/Gear/Weapon.cs.

#include <memory>
#include <string>

#include "../AnimatingSprite.hpp"
#include "../Int32Range.hpp"
#include "Equipment.hpp"

namespace RolePlayingGameData {

// Equipment that can be equipped on a FightingCharacter to improve their physical damage.
class Weapon : public Equipment {
public:
    ~Weapon() override = default;

    // Damage range values are positive, and will be subtracted.
    Int32Range TargetDamageRange;

    std::string SwingCueName;
    std::string HitCueName;
    std::string BlockCueName;

    std::shared_ptr<AnimatingSprite> Overlay;

    std::string GetPowerText() const override {
        return "Weapon Attack: " + TargetDamageRange.ToString();
    }
};

} // namespace RolePlayingGameData

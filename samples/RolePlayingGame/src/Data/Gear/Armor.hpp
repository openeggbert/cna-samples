#pragma once

// Armor.hpp -- C++ port of RolePlayingGameData/Gear/Armor.cs.

#include "../Int32Range.hpp"
#include "Equipment.hpp"

namespace RolePlayingGameData {

// Equipment that can be equipped on a FightingCharacter to improve their defense.
class Armor : public Equipment {
public:
    ~Armor() override = default;

    // Only one piece may fill a slot at the same time.
    enum class ArmorSlot { Helmet, Shield, Torso, Boots };

    ArmorSlot Slot = ArmorSlot::Helmet;

    Int32Range OwnerHealthDefenseRange;
    Int32Range OwnerMagicDefenseRange;

    std::string GetPowerText() const override {
        return "Weapon Defense: " + OwnerHealthDefenseRange.ToString() +
               "\nMagic Defense: " + OwnerMagicDefenseRange.ToString();
    }
};

inline Armor::ArmorSlot ArmorSlotFromString(const std::string& s) {
    if (s == "Helmet") return Armor::ArmorSlot::Helmet;
    if (s == "Shield") return Armor::ArmorSlot::Shield;
    if (s == "Torso") return Armor::ArmorSlot::Torso;
    if (s == "Boots") return Armor::ArmorSlot::Boots;
    return Armor::ArmorSlot::Helmet;
}

} // namespace RolePlayingGameData

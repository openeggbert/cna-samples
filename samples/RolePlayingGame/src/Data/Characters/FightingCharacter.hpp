#pragma once

// FightingCharacter.hpp -- C++ port of RolePlayingGameData/Characters/FightingCharacter.cs.

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "../ContentEntry.hpp"
#include "../Gear/Armor.hpp"
#include "../Gear/Equipment.hpp"
#include "../Gear/Gear.hpp"
#include "../Gear/Weapon.hpp"
#include "../Int32Range.hpp"
#include "../StatisticsValue.hpp"
#include "Character.hpp"
#include "CharacterClass.hpp"

namespace RolePlayingGameData {

// A character that engages in combat.
class FightingCharacter : public Character {
public:
    ~FightingCharacter() override = default;

    std::string CharacterClassContentName;

    std::shared_ptr<CharacterClass> GetCharacterClass() const { return characterClass_; }
    void SetCharacterClass(std::shared_ptr<CharacterClass> v) {
        characterClass_ = std::move(v);
        ResetBaseStatistics();
    }

    int CharacterLevel() const { return characterLevel_; }
    void SetCharacterLevel(int v) {
        characterLevel_ = v;
        ResetBaseStatistics();
        spellsCached_ = false;
        spells_.clear();
    }

    bool IsMaximumCharacterLevel() const {
        return characterLevel_ >= (int)characterClass_->LevelEntries.size();
    }

    const std::vector<std::shared_ptr<Spell>>& Spells() {
        if (!spellsCached_ && characterClass_) {
            spells_ = characterClass_->GetAllSpellsForLevel(characterLevel_);
            spellsCached_ = true;
        }
        return spells_;
    }

    int Experience() const { return experience_; }
    void SetExperience(int v) {
        experience_ = v;
        while (experience_ >= ExperienceForNextLevel()) {
            if (IsMaximumCharacterLevel()) break;
            experience_ -= ExperienceForNextLevel();
            SetCharacterLevel(characterLevel_ + 1);
        }
    }

    int ExperienceForNextLevel() const {
        int checkIndex = std::min(characterLevel_, (int)characterClass_->LevelEntries.size()) - 1;
        return characterClass_->LevelEntries[checkIndex].ExperiencePoints;
    }

    const StatisticsValue& BaseStatistics() const { return baseStatistics_; }
    void SetBaseStatistics(const StatisticsValue& v) { baseStatistics_ = v; }

    void ResetBaseStatistics() {
        baseStatistics_ = characterClass_ ? characterClass_->GetStatisticsForLevel(characterLevel_) : StatisticsValue();
    }

    StatisticsValue CharacterStatistics() const { return baseStatistics_ + equipmentBuffStatistics_; }

    std::vector<std::shared_ptr<Equipment>>& EquippedEquipment() { return equippedEquipment_; }
    const std::vector<std::shared_ptr<Equipment>>& EquippedEquipment() const { return equippedEquipment_; }
    std::vector<std::string> InitialEquipmentContentNames;

    std::shared_ptr<Weapon> GetEquippedWeapon() const {
        for (auto& e : equippedEquipment_) {
            if (auto w = std::dynamic_pointer_cast<Weapon>(e)) return w;
        }
        return nullptr;
    }

    bool EquipWeapon(const std::shared_ptr<Weapon>& weapon, std::shared_ptr<Equipment>& oldEquipment) {
        if (!weapon) throw std::invalid_argument("weapon");
        if (!weapon->CheckRestrictions(*this)) { oldEquipment = nullptr; return false; }

        auto existingWeapon = GetEquippedWeapon();
        if (existingWeapon) {
            oldEquipment = existingWeapon;
            equippedEquipment_.erase(std::remove(equippedEquipment_.begin(), equippedEquipment_.end(),
                                                  std::static_pointer_cast<Equipment>(existingWeapon)),
                                     equippedEquipment_.end());
        } else {
            oldEquipment = nullptr;
        }

        equippedEquipment_.push_back(weapon);
        RecalculateEquipmentStatistics();
        RecalculateTotalTargetDamageRange();
        return true;
    }

    void UnequipWeapon() {
        equippedEquipment_.erase(
            std::remove_if(equippedEquipment_.begin(), equippedEquipment_.end(),
                            [](const std::shared_ptr<Equipment>& e) { return std::dynamic_pointer_cast<Weapon>(e) != nullptr; }),
            equippedEquipment_.end());
        RecalculateEquipmentStatistics();
    }

    std::shared_ptr<Armor> GetEquippedArmor(Armor::ArmorSlot slot) const {
        for (auto& e : equippedEquipment_) {
            if (auto a = std::dynamic_pointer_cast<Armor>(e); a && a->Slot == slot) return a;
        }
        return nullptr;
    }

    bool EquipArmor(const std::shared_ptr<Armor>& armor, std::shared_ptr<Equipment>& oldEquipment) {
        if (!armor) throw std::invalid_argument("armor");
        if (!armor->CheckRestrictions(*this)) { oldEquipment = nullptr; return false; }

        auto equippedArmor = GetEquippedArmor(armor->Slot);
        if (equippedArmor) {
            oldEquipment = equippedArmor;
            equippedEquipment_.erase(std::remove(equippedEquipment_.begin(), equippedEquipment_.end(),
                                                  std::static_pointer_cast<Equipment>(equippedArmor)),
                                     equippedEquipment_.end());
        } else {
            oldEquipment = nullptr;
        }

        equippedEquipment_.push_back(armor);
        RecalculateTotalDefenseRanges();
        RecalculateEquipmentStatistics();
        return true;
    }

    void UnequipArmor(Armor::ArmorSlot slot) {
        equippedEquipment_.erase(
            std::remove_if(equippedEquipment_.begin(), equippedEquipment_.end(),
                            [slot](const std::shared_ptr<Equipment>& e) {
                                auto a = std::dynamic_pointer_cast<Armor>(e);
                                return a && a->Slot == slot;
                            }),
            equippedEquipment_.end());
        RecalculateEquipmentStatistics();
        RecalculateTotalDefenseRanges();
    }

    virtual bool Equip(const std::shared_ptr<Equipment>& equipment) {
        std::shared_ptr<Equipment> oldEquipment;
        return Equip(equipment, oldEquipment);
    }

    virtual bool Equip(const std::shared_ptr<Equipment>& equipment, std::shared_ptr<Equipment>& oldEquipment) {
        if (!equipment) throw std::invalid_argument("equipment");
        if (auto w = std::dynamic_pointer_cast<Weapon>(equipment)) return EquipWeapon(w, oldEquipment);
        if (auto a = std::dynamic_pointer_cast<Armor>(equipment)) return EquipArmor(a, oldEquipment);
        oldEquipment = nullptr;
        return false;
    }

    virtual bool Unequip(const std::shared_ptr<Equipment>& equipment) {
        if (!equipment) throw std::invalid_argument("equipment");
        auto it = std::find(equippedEquipment_.begin(), equippedEquipment_.end(), equipment);
        if (it != equippedEquipment_.end()) {
            equippedEquipment_.erase(it);
            RecalculateEquipmentStatistics();
            RecalculateTotalTargetDamageRange();
            RecalculateTotalDefenseRanges();
            return true;
        }
        return false;
    }

    const StatisticsValue& EquipmentBuffStatistics() const { return equipmentBuffStatistics_; }

    void RecalculateEquipmentStatistics() {
        equipmentBuffStatistics_ = StatisticsValue();
        for (auto& e : equippedEquipment_) equipmentBuffStatistics_ += e->OwnerBuffStatistics;
    }

    const Int32Range& TargetDamageRange() const { return targetDamageRange_; }
    void RecalculateTotalTargetDamageRange() {
        targetDamageRange_ = Int32Range();
        for (auto& e : equippedEquipment_) {
            if (auto w = std::dynamic_pointer_cast<Weapon>(e)) targetDamageRange_ += w->TargetDamageRange;
        }
    }

    const Int32Range& HealthDefenseRange() const { return healthDefenseRange_; }
    const Int32Range& MagicDefenseRange() const { return magicDefenseRange_; }
    void RecalculateTotalDefenseRanges() {
        healthDefenseRange_ = Int32Range();
        magicDefenseRange_ = Int32Range();
        for (auto& e : equippedEquipment_) {
            if (auto a = std::dynamic_pointer_cast<Armor>(e)) {
                healthDefenseRange_ += a->OwnerHealthDefenseRange;
                magicDefenseRange_ += a->OwnerMagicDefenseRange;
            }
        }
    }

    std::vector<std::shared_ptr<ContentEntry<Gear>>> Inventory;

    std::shared_ptr<AnimatingSprite> CombatSprite;

    void ResetAnimation(bool isWalking) override {
        Character::ResetAnimation(isWalking);
        if (CombatSprite) CombatSprite->PlayAnimation("Idle");
    }

    int CombatAnimationInterval = 100;
    void AddStandardCharacterCombatAnimations() {
        if (!CombatSprite) return;
        CombatSprite->AddAnimation(std::make_shared<Animation>("Idle", 37, 42, CombatAnimationInterval, true));
        CombatSprite->AddAnimation(std::make_shared<Animation>("Walk", 25, 30, CombatAnimationInterval, true));
        CombatSprite->AddAnimation(std::make_shared<Animation>("Attack", 1, 6, CombatAnimationInterval, false));
        CombatSprite->AddAnimation(std::make_shared<Animation>("SpellCast", 31, 36, CombatAnimationInterval, false));
        CombatSprite->AddAnimation(std::make_shared<Animation>("Defend", 13, 18, CombatAnimationInterval, false));
        CombatSprite->AddAnimation(std::make_shared<Animation>("Dodge", 13, 18, CombatAnimationInterval, false));
        CombatSprite->AddAnimation(std::make_shared<Animation>("Hit", 19, 24, CombatAnimationInterval, false));
        CombatSprite->AddAnimation(std::make_shared<Animation>("Die", 7, 12, CombatAnimationInterval, false));
    }

private:
    std::shared_ptr<CharacterClass> characterClass_;
    int characterLevel_ = 1;
    std::vector<std::shared_ptr<Spell>> spells_;
    bool spellsCached_ = false;
    int experience_ = 0;
    StatisticsValue baseStatistics_;
    std::vector<std::shared_ptr<Equipment>> equippedEquipment_;
    StatisticsValue equipmentBuffStatistics_;
    Int32Range targetDamageRange_;
    Int32Range healthDefenseRange_;
    Int32Range magicDefenseRange_;
};

// Cross-referencing definition -- Gear::CheckRestrictions needs FightingCharacter
// to be fully defined (see Gear.hpp).
inline bool Gear::CheckRestrictions(const FightingCharacter& fightingCharacter) const {
    if (fightingCharacter.CharacterLevel() < MinimumCharacterLevel) return false;
    if (SupportedClasses.empty()) return true;
    auto& className = fightingCharacter.GetCharacterClass()->Name;
    return std::find(SupportedClasses.begin(), SupportedClasses.end(), className) != SupportedClasses.end();
}

} // namespace RolePlayingGameData

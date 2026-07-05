#pragma once

// Player.hpp -- C++ port of RolePlayingGameData/Characters/Player.cs.

#include <memory>
#include <string>

#include "../StatisticsValue.hpp"
#include "FightingCharacter.hpp"

namespace RolePlayingGameData {

// A member of the player's party, also represented in the world before joining.
// There is only one of a given Player in the game world at a time, and their
// current statistics persist after combat -- tracked here.
class Player : public FightingCharacter {
public:
    // The current set of persistent statistics modifiers -- damage, etc.
    StatisticsValue StatisticsModifiers;

    StatisticsValue CurrentStatistics() const { return CharacterStatistics() + StatisticsModifiers; }

    int Gold = 0;

    std::string IntroductionDialogue;
    std::string JoinAcceptedDialogue;
    std::string JoinRejectedDialogue;

    std::string ActivePortraitTextureName;
    std::shared_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> ActivePortraitTexture;
    std::string InactivePortraitTextureName;
    std::shared_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> InactivePortraitTexture;
    std::string UnselectablePortraitTextureName;
    std::shared_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> UnselectablePortraitTexture;

    std::shared_ptr<Player> Clone() const {
        auto player = std::make_shared<Player>();

        player->ActivePortraitTexture = ActivePortraitTexture;
        player->ActivePortraitTextureName = ActivePortraitTextureName;
        player->SetAssetName(AssetName());
        player->SetCharacterClass(GetCharacterClass());
        player->CharacterClassContentName = CharacterClassContentName;
        player->SetCharacterLevel(CharacterLevel());
        player->CombatAnimationInterval = CombatAnimationInterval;
        player->CombatSprite = CombatSprite ? CombatSprite->Clone() : nullptr;
        player->CharacterDirection = CharacterDirection;
        player->EquippedEquipment() = EquippedEquipment();
        player->SetExperience(Experience());
        player->Gold = Gold;
        player->InactivePortraitTexture = InactivePortraitTexture;
        player->InactivePortraitTextureName = InactivePortraitTextureName;
        player->InitialEquipmentContentNames = InitialEquipmentContentNames;
        player->IntroductionDialogue = IntroductionDialogue;
        player->Inventory = Inventory;
        player->JoinAcceptedDialogue = JoinAcceptedDialogue;
        player->JoinRejectedDialogue = JoinRejectedDialogue;
        player->MapIdleAnimationInterval = MapIdleAnimationInterval;
        player->MapPosition = MapPosition;
        player->MapSprite = MapSprite ? MapSprite->Clone() : nullptr;
        player->MapWalkingAnimationInterval = MapWalkingAnimationInterval;
        player->SetName(Name());
        player->ShadowTexture = ShadowTexture;
        player->State = State;
        player->UnselectablePortraitTexture = UnselectablePortraitTexture;
        player->UnselectablePortraitTextureName = UnselectablePortraitTextureName;
        player->WalkingSprite = WalkingSprite ? WalkingSprite->Clone() : nullptr;

        player->RecalculateEquipmentStatistics();
        player->RecalculateTotalDefenseRanges();
        player->RecalculateTotalTargetDamageRange();
        player->ResetAnimation(false);
        player->ResetBaseStatistics();

        return player;
    }
};

} // namespace RolePlayingGameData

#pragma once

// Spell.hpp -- C++ port of RolePlayingGameData/Spell.cs.

#include <memory>
#include <string>

#include "AnimatingSprite.hpp"
#include "ContentObject.hpp"
#include "StatisticsRange.hpp"
#include "StatisticsValue.hpp"

namespace RolePlayingGameData {

class Spell : public ContentObject {
public:
    std::string Name;
    std::string Description;
    int MagicPointCost = 0;

    virtual std::string GetPowerText() const { return TargetEffectRange().GetModifierString(); }

    // If true, the statistics change are used as a debuff (subtracted).
    // Otherwise, the statistics change is used as a buff (added).
    bool IsOffensive = false;
    // If the duration is zero, then the effects last for the rest of the battle.
    int TargetDuration = 0;

    // This is a debuff if IsOffensive is true, otherwise it's a buff.
    const StatisticsRange& TargetEffectRange() const { return targetEffectRange_; }

    StatisticsRange InitialTargetEffectRange;
    int AdjacentTargets = 0;

    int Level() const { return level_; }
    void SetLevel(int v) {
        level_ = v;
        targetEffectRange_ = InitialTargetEffectRange;
        for (int i = 1; i < level_; i++) targetEffectRange_ += LevelingProgression;
    }

    StatisticsValue LevelingProgression;

    std::string CreatingCueName;
    std::string TravelingCueName;
    std::string ImpactCueName;
    std::string BlockCueName;

    std::string IconTextureName;
    std::shared_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> IconTexture;

    std::shared_ptr<AnimatingSprite> SpellSprite;
    std::shared_ptr<AnimatingSprite> Overlay;

    // Assignable directly by the content loader (mirrors the original Reader's
    // direct field assignment of targetEffectRange before Level is set).
    void SetTargetEffectRangeDirect(const StatisticsRange& r) { targetEffectRange_ = r; }

    std::shared_ptr<Spell> Clone() const {
        auto spell = std::make_shared<Spell>();
        spell->AdjacentTargets = AdjacentTargets;
        spell->SetAssetName(AssetName());
        spell->BlockCueName = BlockCueName;
        spell->CreatingCueName = CreatingCueName;
        spell->Description = Description;
        spell->IconTexture = IconTexture;
        spell->IconTextureName = IconTextureName;
        spell->ImpactCueName = ImpactCueName;
        spell->InitialTargetEffectRange = InitialTargetEffectRange;
        spell->IsOffensive = IsOffensive;
        spell->LevelingProgression = LevelingProgression;
        spell->MagicPointCost = MagicPointCost;
        spell->Name = Name;
        spell->Overlay = Overlay ? Overlay->Clone() : nullptr;
        spell->SpellSprite = SpellSprite ? SpellSprite->Clone() : nullptr;
        spell->TargetDuration = TargetDuration;
        spell->TravelingCueName = TravelingCueName;
        spell->SetLevel(level_);
        return spell;
    }

private:
    StatisticsRange targetEffectRange_;
    int level_ = 1;
};

} // namespace RolePlayingGameData

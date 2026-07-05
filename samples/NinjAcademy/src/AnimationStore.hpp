#pragma once

// AnimationStore.hpp — C++ port of NinjAcademyCommonTypes/AnimationStore.cs
// (XNA 4.0 NinjAcademy sample). Stores a collection of animations, keyed by
// alias.
//
// The original loads this from Textures/Animations.xml via a custom content
// pipeline processor (NinjAcademyPipeline/AnimationProcessor.cs). CNA has no
// general XML content-pipeline deserializer, so -- matching this project's
// established precedent for such data (e.g. DynamicMenu's MenuPage2.xml) --
// the XML is hand-translated once into equivalent C++ construction code
// below; see missing.md.

#include <stdexcept>
#include <string>
#include <unordered_map>

#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "System/TimeSpan.hpp"

#include "Animation.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Content::ContentManager;

// Port of NinjAcademyCommonTypes/AnimationStore.cs.
class AnimationStore {
public:
    std::unordered_map<std::string, Animation> Animations;

    // Returns an animation from the store which has the specified alias.
    Animation& operator[](const std::string& animationAlias) {
        auto it = Animations.find(animationAlias);
        if (it == Animations.end())
            throw std::out_of_range("No animation with alias '" + animationAlias + "'");
        return it->second;
    }

    // Initializes all contained animations by loading their sprite sheets.
    void Initialize(ContentManager& contentManager) {
        for (auto& [alias, animation] : Animations)
            animation.LoadSheet(contentManager);
    }
};

// Hand-translated equivalent of Textures/Animations.xml (see file header).
inline AnimationStore BuildAnimationStore() {
    AnimationStore store;

    auto add = [&](const std::string& alias, int frameWidth, int frameHeight, int visualCenterX, int visualCenterY,
                    int sheetRows, int sheetColumns, const std::string& sheetName, int frameMillisecondInterval,
                    bool cyclic) {
        Animation animation(sheetName, Point(frameWidth, frameHeight), Point(sheetColumns, sheetRows),
                             Vector2((float)visualCenterX, (float)visualCenterY), cyclic);
        animation.SetFrameInterval(System::TimeSpan::FromMilliseconds(frameMillisecondInterval));
        store.Animations.emplace(alias, std::move(animation));
    };

    add("Dynamite", 27, 117, 9, 69, 1, 2, "Textures/Game Elements/dynamite", 250, true);
    add("ThrowingStar", 203, 91, 100, 32, 1, 4, "Textures/Game Elements/throwingStar", 150, true);
    add("GoldTarget", 57, 58, 29, 29, 1, 12, "Textures/Game Elements/gold_target", 500, true);
    add("FallingTarget", 57, 58, 29, 29, 1, 8, "Textures/Game Elements/targetFalling", 750, false);
    add("FallingGoldTarget", 57, 58, 29, 29, 1, 8, "Textures/Game Elements/GoldTargetFalling", 750, false);
    add("Explosion", 40, 40, 20, 21, 1, 6, "Textures/Game Elements/Explosion", 400, false);

    return store;
}

} // namespace NinjAcademy

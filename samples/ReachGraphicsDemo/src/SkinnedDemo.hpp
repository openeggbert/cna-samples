#pragma once

// Placeholder for XnaGraphicsDemo.SkinnedDemo (SkinnedDemo.cs) -- SKIPPED per this
// task's own scope decision. SkinnedDemo demonstrates SkinnedEffect: an animated
// humanoid model ("dude.fbx") driven by SkinningData/AnimationClip/AnimationPlayer
// (borrowed from the XNA "Skinned Model" sample) over a generated skydome background
// (Sky.cs/SkyProcessor.cs, borrowed from the "Generated Geometry" sample).
//
// CNA has no skeletal animation playback system at all (DEFERRED.md item #13 -- no
// AnimationClip/Keyframe/AnimationPlayer types, and no per-vertex bone weight/keyframe
// data in the .model.json format), so this cannot be ported, faithfully or otherwise --
// unlike InverseKinematics' Avatar half (which keeps the *entire* original code
// structure, permanently guarded by a real, always-false CNA runtime condition,
// AvatarRenderer::State() always reporting Unavailable), there is no equivalent
// CNA class or state to guard against here: the underlying types this demo needs
// (SkinningData, AnimationClip, AnimationPlayer, a SkinnedEffect wrapper) simply do not
// exist in CNA yet, so there is nothing to construct even in an inert/guarded form.
//
// Per this task's own scope decision, the menu entry that would open this scene is
// replaced with a clear "not available" message instead -- the surrounding
// MenuComponent framework (menu list, back button, attract-mode cycling, F1 help
// overlay) all still work normally when this screen is selected; only the animated
// model itself is replaced with explanatory text. See missing.md for the full account.

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "DemoGame.hpp"
#include "MenuComponent.hpp"
#include "MenuEntry.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class SkinnedDemo : public MenuComponent {
public:
    explicit SkinnedDemo(DemoGame& game) : MenuComponent(game) {
        auto backEntry = std::make_unique<MenuEntry>();
        backEntry->SetText("back");
        backEntry->Clicked = [&game]() { game.SetActiveMenu(0); };
        Entries.push_back(std::move(backEntry));
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "SkinnedDemo";
        return name;
    }

    // Draws the "not available" placeholder in place of the SkinnedEffect demo.
    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(0, 0, 0, 255));

        DrawTitle("skinned effect", std::optional<Color>(), Color(127, 112, 104, 255));

        GetSpriteBatch().Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend, nullptr, nullptr, nullptr, nullptr,
                                GetGame().GetScaleMatrix());

        constexpr float lineHeight = 32.0f;
        Vector2 pos(48.0f, 300.0f);

        auto drawLine = [&](const std::string& text) {
            GetSpriteBatch().DrawString(GetFont(), text, pos, Color::White);
            pos.Y += lineHeight;
        };

        drawLine("SkinnedEffect / skeletal animation");
        drawLine("is not available in this port.");
        pos.Y += lineHeight;
        drawLine("CNA has no AnimationClip / Keyframe /");
        drawLine("AnimationPlayer implementation yet");
        drawLine("(DEFERRED.md item #13).");
        pos.Y += lineHeight;
        drawLine("See missing.md for details.");

        GetSpriteBatch().End();

        // This will draw the "back" menu entry.
        MenuComponent::Draw(gameTime);
    }
};

} // namespace ReachGraphicsDemoSample

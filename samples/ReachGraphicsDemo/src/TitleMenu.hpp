#pragma once

// Ported from XnaGraphicsDemo.TitleMenu (TitleMenu.cs). The main menu screen allows
// users to choose between the various demo screens.
//
// NOTE (correction to this task's own original brief): TitleMenu.cs does NOT use
// Sky.cs/Tank.cs or any 3D scene at all -- a full read of the C# original found its own
// Draw() is just DrawTitle() (a flat CornflowerBlue background + BigFont title text) and
// a SpriteBatch-only "floating xna text labels" background effect. The task brief's
// premise ("TitleMenu's own 3D scene using Sky.cs/Tank.cs") turned out to be incorrect --
// Sky.cs/Tank.cs are used by SkinnedDemo.cs/BasicDemo.cs+AlphaDemo.cs respectively, not
// TitleMenu.cs. So there was no "simplify the title screen" scope decision to make here;
// TitleMenu is ported fully and faithfully as-is. See missing.md for the full account.

#include <cmath>
#include <random>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "System/Random.hpp"

#include "DemoGame.hpp"
#include "MenuComponent.hpp"
#include "MenuEntry.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using System::TimeSpan;

// We display a set of floating "xna" text labels in the background of the menu.
struct FloatingXna {
    Vector2 Position;
    float Age = 0.0f;
    float Size = 1.0f;
};

class TitleMenu : public MenuComponent {
public:
    explicit TitleMenu(DemoGame& game) : MenuComponent(game) {
        Entries.push_back(MakeEntry("basic effect", [&g = game]() { g.SetActiveMenu(1); }));
        Entries.push_back(MakeEntry("dual texture effect", [&g = game]() { g.SetActiveMenu(2); }));
        Entries.push_back(MakeEntry("alpha test effect", [&g = game]() { g.SetActiveMenu(3); }));
        Entries.push_back(MakeEntry("skinned effect", [&g = game]() { g.SetActiveMenu(4); }));
        Entries.push_back(MakeEntry("environment map effect", [&g = game]() { g.SetActiveMenu(5); }));
        Entries.push_back(MakeEntry("particles", [&g = game]() { g.SetActiveMenu(6); }));
        Entries.push_back(MakeEntry("quit", [&game]() { game.Exit(); }));
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "TitleMenu";
        return name;
    }

    // Resets the menu state.
    void Reset() override {
        floatingXnas_.clear();
        time_ = 0.0f;

        MenuComponent::Reset();
    }

    // Updates the floating "xna" background labels.
    void Update(GameTime& gameTime) override {
        time_ += static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

        // Spawn a new label?
        if (time_ > XnaSpawnRate) {
            FloatingXna xna;

            xna.Size = static_cast<float>(random_.NextDouble()) * 2.0f + 0.5f;

            xna.Position.X = static_cast<float>(random_.NextDouble()) * 320.0f + 80.0f;
            xna.Position.Y = static_cast<float>(random_.NextDouble()) * 700.0f + 50.0f;

            floatingXnas_.push_back(xna);

            time_ -= XnaSpawnRate;
        }

        // Animate the existing labels.
        std::size_t i = 0;

        while (i < floatingXnas_.size()) {
            FloatingXna& xna = floatingXnas_[i];

            xna.Age += static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

            // Different size labels move at different speeds.
            float speed = 1.5f - xna.Size;

            if (std::abs(speed) > 0.01f) {
                xna.Position.Y -= xna.Age * xna.Age / speed / 10.0f;
            }

            // Remove old labels.
            if (xna.Age >= XnaLifespan) {
                floatingXnas_.erase(floatingXnas_.begin() + static_cast<long>(i));
            } else {
                ++i;
            }
        }

        MenuComponent::Update(gameTime);
    }

    // Draws the main menu.
    void Draw(const GameTime& gameTime) override {
        DrawTitle("xna demo", Color::CornflowerBlue, Color::Lerp(Color::Blue, Color::CornflowerBlue, 0.85f));

        // Draw the background "xna" labels.
        GetSpriteBatch().Begin();

        for (auto& blob : floatingXnas_) {
            float alpha = std::min(blob.Age, 1.0f) * std::min((XnaLifespan - blob.Age) / (XnaLifespan - 2.0f), 1.0f);

            alpha *= alpha;
            alpha /= 8.0f;

            GetSpriteBatch().DrawString(GetBigFont(), "xna", blob.Position, Color::Blue * alpha, MathHelper::PiOver2,
                                         Vector2::Zero, blob.Size, SpriteEffects::None, 0.0f);
        }

        GetSpriteBatch().End();

        // This will draw the various menu items.
        MenuComponent::Draw(gameTime);
    }

protected:
    // The main menu wants a shorter attract delay than the other screens.
    TimeSpan AttractDelay() const override { return TimeSpan::FromSeconds(3); }

    // When the attract mode timeout is reached, we cycle through each other screen in turn.
    void OnAttract() override {
        Entries[static_cast<std::size_t>(attractCycle_)]->OnClicked();

        if (++attractCycle_ >= static_cast<int>(Entries.size()) - 1) attractCycle_ = 0;
    }

private:
    static std::unique_ptr<MenuEntry> MakeEntry(const std::string& text, std::function<void()> clicked) {
        auto entry = std::make_unique<MenuEntry>();
        entry->SetText(text);
        entry->Clicked = std::move(clicked);
        return entry;
    }

    int attractCycle_ = 0;
    float time_ = 0.0f;

    System::Random random_;

    std::vector<FloatingXna> floatingXnas_;

    static constexpr float XnaSpawnRate = 1.5f;
    static constexpr float XnaLifespan = 7.0f;
};

} // namespace ReachGraphicsDemoSample

#pragma once

// Ported from XnaGraphicsDemo.ParticleDemo (ParticleDemo.cs). Demo shows how to use
// SpriteBatch: a simple 2D "cat" particle system (up to 5000 particles, a ring buffer of
// position/velocity/rotation/spin/size/color), with a spawn-rate slider and
// drag-to-spawn-particles interaction.
//
// NOXNA note: the C# original's own commented-out `ResolutionMenu` entry (a menu item
// that cycles the back buffer through 480x800/360x600/240x400) is dead code in the
// original itself ("This menu option ... is currently disabled, because the image
// scaler feature is not yet implemented in the CTP release") -- not ported here either,
// for the same reason the original never enabled it.

#include <array>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/Random.hpp"

#include "DemoGame.hpp"
#include "MenuComponent.hpp"
#include "MenuEntry.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class ParticleDemo : public MenuComponent {
public:
    static constexpr int MaxParticles = 5000;

    struct Particle {
        Vector2 Position;
        Vector2 Velocity;
        float Size = 0.0f;
        float Rotation = 0.0f;
        float Spin = 0.0f;
        Color Tint = Color::White;
    };

    explicit ParticleDemo(DemoGame& game) : MenuComponent(game) {
        auto spawnRateEntry = std::make_unique<FloatMenuEntry>();
        spawnRateEntry->SetText("spawn rate");
        spawnRate_ = spawnRateEntry.get();
        Entries.push_back(std::move(spawnRateEntry));

        auto backEntry = std::make_unique<MenuEntry>();
        backEntry->SetText("back");
        backEntry->Clicked = [&game]() {
            // Before we quit back out of this menu, reset back to the default resolution.
            if (game.GetGraphics().getPreferredBackBufferWidthProperty() != 480) {
                game.GetGraphics().setPreferredBackBufferWidthProperty(480);
                game.GetGraphics().setPreferredBackBufferHeightProperty(800);

                game.GetGraphics().ApplyChanges();
            }

            game.SetActiveMenu(0);
        };
        Entries.push_back(std::move(backEntry));
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "ParticleDemo";
        return name;
    }

    // Resets the menu state.
    void Reset() override {
        firstParticle_ = 0;
        particleCount_ = 0;
        spawnRate_->Value = 0.2f;

        MenuComponent::Reset();
    }

    // Loads content for this demo.
    void LoadContent() override { cat_ = GetGame().getContentProperty().Load<Texture2D>("cat"); }

    // Updates the particle system.
    void Update(GameTime& gameTime) override {
        int i = firstParticle_;

        for (int j = particleCount_; j > 0; j--) {
            // Move a particle.
            particles_[static_cast<std::size_t>(i)].Position =
                particles_[static_cast<std::size_t>(i)].Position + particles_[static_cast<std::size_t>(i)].Velocity;
            particles_[static_cast<std::size_t>(i)].Rotation += particles_[static_cast<std::size_t>(i)].Spin;
            particles_[static_cast<std::size_t>(i)].Velocity.Y += 0.1f;

            // Retire old particles?
            constexpr float borderPadding = 96.0f;

            if (i == firstParticle_) {
                const Vector2& pos = particles_[static_cast<std::size_t>(i)].Position;
                if (pos.X < -borderPadding || pos.X > 480.0f + borderPadding || pos.Y < -borderPadding ||
                    pos.Y > 800.0f + borderPadding) {
                    if (++firstParticle_ >= MaxParticles) firstParticle_ = 0;

                    particleCount_--;
                }
            }

            if (++i >= MaxParticles) i = 0;
        }

        // Spawn new particles?
        spawnCounter_ += spawnRate_->Value * 10.0f;

        while (spawnCounter_ > 1.0f) {
            SpawnParticle(std::optional<Vector2>());
            spawnCounter_ -= 1.0f;
        }

        MenuComponent::Update(gameTime);
    }

    // Draws the cat particle system.
    void Draw(const GameTime& gameTime) override {
        DrawTitle("particles", Color::CornflowerBlue, Color::Lerp(Color::Blue, Color::CornflowerBlue, 0.85f));

        GetSpriteBatch().Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend, nullptr, nullptr, nullptr, nullptr,
                                GetGame().GetScaleMatrix());

        Vector2 origin(static_cast<float>(cat_->getWidthProperty()) / 2.0f, static_cast<float>(cat_->getHeightProperty()) / 2.0f);

        int i = firstParticle_ + particleCount_ - 1;

        if (i >= MaxParticles) i -= MaxParticles;

        for (int j = 0; j < particleCount_; j++) {
            const Particle& p = particles_[static_cast<std::size_t>(i)];
            GetSpriteBatch().Draw(*cat_, p.Position, std::optional<Rectangle>(), p.Tint, p.Rotation, origin, p.Size,
                                   SpriteEffects::None, 0.0f);

            if (--i < 0) i = MaxParticles - 1;
        }

        GetSpriteBatch().End();

        MenuComponent::Draw(gameTime);
    }

protected:
    // Dragging on the menu background creates new particles.
    void OnDrag(Vector2 /*delta*/) override { SpawnParticle(LastTouchPoint); }

private:
    // Helper creates a new cat particle.
    void SpawnParticle(std::optional<Vector2> position) {
        if (particleCount_ >= MaxParticles) return;

        int i = firstParticle_ + particleCount_;

        if (i >= MaxParticles) i -= MaxParticles;

        Particle& p = particles_[static_cast<std::size_t>(i)];

        p.Position = position.has_value() ? position.value()
                                           : Vector2(static_cast<float>(random_.NextDouble()) * 480.0f,
                                                     static_cast<float>(random_.NextDouble()) * 800.0f);
        p.Velocity = Vector2(static_cast<float>(random_.NextDouble()) - 0.5f, static_cast<float>(random_.NextDouble()) - 0.5f) * 10.0f;
        p.Size = static_cast<float>(random_.NextDouble()) * 0.5f + 0.5f;
        p.Rotation = 0.0f;
        p.Spin = (static_cast<float>(random_.NextDouble()) - 0.5f) * 0.1f;

        if (position.has_value()) {
            // Explicitly positioned particles have no tint.
            p.Tint = Color::White;
        } else {
            // Randomly positioned particles have random tint colors.
            int r = 128 + static_cast<int>(random_.NextDouble() * 127);
            int g = 128 + static_cast<int>(random_.NextDouble() * 127);
            int b = 128 + static_cast<int>(random_.NextDouble() * 127);

            p.Tint = Color(r, g, b, 255);
        }

        particleCount_++;
    }

    std::array<Particle, MaxParticles> particles_;

    int firstParticle_ = 0;
    int particleCount_ = 0;

    FloatMenuEntry* spawnRate_ = nullptr;
    float spawnCounter_ = 0.0f;

    std::optional<Texture2D> cat_;

    System::Random random_;
};

} // namespace ReachGraphicsDemoSample

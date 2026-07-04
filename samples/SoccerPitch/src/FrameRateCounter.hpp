#pragma once

// FrameRateCounter.hpp — C++ port of FrameRateCounter.cs (XNA 4.0 SoccerPitch
// sample). General timing/frame-rate display component.
//
// Adaptation note: the original constructs its own ContentManager
// (`new ContentManager(game.Services)`) with a blank RootDirectory and loads
// `"content\\Font"` (a literal path prefix, since that ContentManager's root
// was never set to "Content"). This port sets RootDirectory to "Content"
// explicitly and loads plain "Font" instead — same asset, tidier path,
// no behavior difference.

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "System/TimeSpan.hpp"

namespace FrameRateCounterComponent {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;

// General timing and frame-rate display component. Port of FrameRateCounter.cs.
class FrameRateCounter : public DrawableGameComponent {
public:
    explicit FrameRateCounter(Game& game)
        : DrawableGameComponent(game), content_(&game.getServicesProperty()) {
        content_.setRootDirectoryProperty("Content");
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        content_.setGraphicsDevice(getGraphicsDeviceProperty());
        font_.emplace(content_.Load<SpriteFont>("Font"));

        fpsScreenLocation_ = Vector2(320.0f, 32.0f);

        DrawableGameComponent::LoadContent();
    }

    void UnloadContent() override {
        content_.Unload();
        DrawableGameComponent::UnloadContent();
    }

    void Update(GameTime& gameTime) override {
        elapsedTicks_ += gameTime.getElapsedGameTimeProperty().getTicksProperty();
        if (elapsedTicks_ > System::TimeSpan::TicksPerSecond) {
            elapsedTicks_ -= System::TimeSpan::TicksPerSecond;
            frameRate_ = frameCounter_;
            frameCounter_ = 0;
        }
    }

    void Draw(const GameTime&) override {
        frameCounter_++;

        std::string fps = "fps: " + std::to_string(frameRate_);

        spriteBatch_->Begin();
        spriteBatch_->DrawString(*font_, fps, fpsScreenLocation_, Color::White);
        spriteBatch_->End();
    }

private:
    ContentManager content_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> font_;

    Vector2 fpsScreenLocation_;
    int frameRate_ = 0;
    int frameCounter_ = 0;
    int64_t elapsedTicks_ = 0;
};

} // namespace FrameRateCounterComponent

#pragma once

// DebugManager.hpp — C++ port of GameDebugTools/DebugManager.cs (XNA 4.0
// PerformanceMeasuring sample). Holds shared graphics resources (SpriteBatch,
// white texture, debug font) used by the other GameDebugTools components.

#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace PerformanceMeasuring::GameDebugTools {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// DebugManager class that holds graphics resources for debug drawing. Port of
// GameDebugTools/DebugManager.cs.
class DebugManager : public DrawableGameComponent {
public:
    DebugManager(Game& game, std::string debugFontName)
        : DrawableGameComponent(game), debugFontName_(std::move(debugFontName)) {
        game.getServicesProperty().AddService<DebugManager>(this);

        // This component doesn't need to be updated nor drawn.
        setEnabledProperty(false);
        setVisibleProperty(false);
    }

    SpriteBatch& getSpriteBatch() { return *spriteBatch_; }
    Texture2D& getWhiteTexture() { return *whiteTexture_; }
    SpriteFont& getDebugFont() { return *debugFont_; }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        debugFont_.emplace(getGameProperty().getContentProperty().Load<SpriteFont>(debugFontName_));

        whiteTexture_ = std::make_unique<Texture2D>(getGraphicsDeviceProperty(), 1, 1);
        Color white = Color::White;
        whiteTexture_->SetData(&white, 1);

        DrawableGameComponent::LoadContent();
    }

private:
    std::string debugFontName_;

    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::unique_ptr<Texture2D> whiteTexture_;
    std::optional<SpriteFont> debugFont_;
};

} // namespace PerformanceMeasuring::GameDebugTools

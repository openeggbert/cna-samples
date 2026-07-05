#pragma once

// BackgroundScreen.hpp — C++ port of Screens/BackgroundScreen.cs (XNA 4.0
// NinjAcademy sample). A screen that just draws a static full-screen
// background texture.

#include <string>

#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// Port of Screens/BackgroundScreen.cs.
class BackgroundScreen : public GameScreen {
public:
    explicit BackgroundScreen(const std::string& backgroundName) : backgroundName_(backgroundName) {
        setTransitionOnTime(TimeSpan::FromSeconds(0.0));
        setTransitionOffTime(TimeSpan::FromSeconds(0.5));
    }

    void LoadContent() override {
        background_.emplace(Load<Texture2D>("Textures/Backgrounds/" + backgroundName_));
        viewport_ = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty().getBoundsProperty();
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        (void)coveredByOtherScreen;
        GameScreen::Update(gameTime, otherScreenHasFocus, false);
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();
        spriteBatch.Draw(*background_, viewport_, mul(Color::White, TransitionAlpha()));
        spriteBatch.End();
    }

private:
    std::string backgroundName_;
    std::optional<Texture2D> background_;
    Rectangle viewport_;
};

} // namespace NinjAcademy

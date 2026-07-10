#pragma once

// BackgroundScreen.hpp — C++ port of Screens/BackgroundScreen.cs (XNA 4.0
// MarbleMaze sample).

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/TimeSpan.hpp"

#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class BackgroundScreen : public GameScreen {
public:
    BackgroundScreen() {
        setTransitionOnTime(System::TimeSpan::FromSeconds(0.0));
        setTransitionOffTime(System::TimeSpan::FromSeconds(0.5));
    }

    void LoadContent() override { background_ = Load<Texture2D>("Images/titleScreen"); }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        (void)coveredByOtherScreen;
        GameScreen::Update(gameTime, otherScreenHasFocus, false);
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();

        spriteBatch.Begin();
        spriteBatch.Draw(*background_, Vector2(0, 0), mul(Color::White, TransitionAlpha()));
        spriteBatch.End();
    }

private:
    std::optional<Texture2D> background_;
};

} // namespace MarbleMazeSample

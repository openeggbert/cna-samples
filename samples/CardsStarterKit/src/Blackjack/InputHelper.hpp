#pragma once

// InputHelper.hpp -- C++ port of Misc/InputHelper.cs (XNA 4.0 CardsStarterKit
// sample). Simulates an on-screen cursor driven by a gamepad, for Xbox. This
// desktop port keeps it disabled (Visible/Enabled false), exactly matching
// the original's own `#if !XBOX` branch in GameplayScreen.LoadContent() --
// Button/BetGameComponent still look it up unconditionally on non-Xbox
// platforms too, so it must still exist and be harmless when idle.

#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "BlackjackCommon.hpp"

namespace Blackjack {

using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class InputHelper : public DrawableGameComponent {
public:
    bool IsEscape = false;
    bool IsPressed = false;

    explicit InputHelper(Game& game)
        : DrawableGameComponent(game), spriteBatch_(game.getGraphicsDeviceProperty()) {
        texture_ = game.getContentProperty().Load<Texture2D>("Images/GamePadCursor");
        maxVelocity_ = (float)(game.getGraphicsDeviceProperty().getViewportProperty().getWidthProperty() +
                               game.getGraphicsDeviceProperty().getViewportProperty().getHeightProperty()) / 3000.0f;
        drawPosition_ = Vector2((float)game.getGraphicsDeviceProperty().getViewportProperty().getWidthProperty() / 2.0f,
                               (float)game.getGraphicsDeviceProperty().getViewportProperty().getHeightProperty() / 2.0f);
    }

    Vector2 PointPosition() const {
        Rectangle bounds = texture_.getBoundsProperty();
        return drawPosition_ + Vector2(bounds.Width / 2.0f, bounds.Height / 2.0f);
    }

    void Update(GameTime& gameTime) override {
        GamePadState gamePadState = GamePad::GetState(PlayerIndex::One);

        IsPressed = gamePadState.IsButtonDown(Buttons::A);
        IsEscape = gamePadState.IsButtonDown(Buttons::Back);

        Vector2 stick = gamePadState.getThumbSticksProperty().getLeftProperty();
        drawPosition_ = drawPosition_ + Vector2(stick.X, -stick.Y) *
                        (float)gameTime.getElapsedGameTimeProperty().getMillisecondsProperty() * maxVelocity_;

        Rectangle bounds = texture_.getBoundsProperty();
        auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
        Vector2 maxPos((float)viewport.getWidthProperty() - bounds.Width, (float)viewport.getHeightProperty() - bounds.Height);
        drawPosition_ = Vector2::Clamp(drawPosition_, Vector2::Zero, maxPos);
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        spriteBatch_.Begin();
        spriteBatch_.Draw(texture_, drawPosition_, Color::White);
        spriteBatch_.End();
    }

private:
    Vector2 drawPosition_;
    Texture2D texture_;
    SpriteBatch spriteBatch_;
    float maxVelocity_ = 0.0f;
};

} // namespace Blackjack

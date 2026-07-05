#pragma once

// Button.hpp -- C++ port of UI/Button.cs (XNA 4.0 CardsStarterKit sample).
// A clickable AnimatedGameComponent: fires Click on mouse-button release
// while still within its Bounds (not on press), matching the original's
// desktop/Xbox input path -- the Windows-Phone tap-to-click branch is
// dropped, same precedent as every other input adaptation in this repo.

#include <functional>
#include <string>

#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/EventArgs.hpp"
#include "System/EventHandler.hpp"

#include "../CardsFramework/AnimatedGameComponent.hpp"
#include "../CardsFramework/CardsGame.hpp"
#include "../GameStateManagement/InputState.hpp"
#include "BlackjackCommon.hpp"
#include "InputHelper.hpp"

namespace Blackjack {

using CardsFramework::AnimatedGameComponent;
using CardsFramework::CardsGame;
using GameStateManagement::InputState;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class Button : public AnimatedGameComponent {
public:
    Texture2D RegularTexture;
    Texture2D PressedTexture;
    SpriteFont* Font = nullptr;
    Rectangle Bounds;

    System::EventHandler<System::EventArgs> Click;

    Button(const std::string& regularTexture, const std::string& pressedTexture, InputState& input,
           CardsGame& cardGame)
        : AnimatedGameComponent(cardGame, nullptr), input_(input),
          regularTextureName_(regularTexture), pressedTextureName_(pressedTexture) {
        (void)input_;
    }

    void Initialize() override {
        for (auto* c : getGameProperty().getComponentsProperty()) {
            inputHelper_ = dynamic_cast<InputHelper*>(c);
            if (inputHelper_) break;
        }

        spriteBatch_.emplace(getGraphicsDeviceProperty());
        AnimatedGameComponent::Initialize();
    }

    void LoadContent() override {
        if (!regularTextureName_.empty())
            RegularTexture = getGameProperty().getContentProperty().Load<Texture2D>("Images/" + regularTextureName_);
        if (!pressedTextureName_.empty())
            PressedTexture = getGameProperty().getContentProperty().Load<Texture2D>("Images/" + pressedTextureName_);

        AnimatedGameComponent::LoadContent();
    }

    void Update(GameTime& gameTime) override {
        HandleInput(Mouse::GetState());
        AnimatedGameComponent::Update(gameTime);
    }

    void FireClick() { Click.Raise(this, System::EventArgs()); }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        spriteBatch_->Begin();

        spriteBatch_->Draw(isPressed_ ? PressedTexture : RegularTexture, Bounds, Color::White);
        if (Font != nullptr && Text.has_value()) {
            Vector2 textSize = Font->MeasureString(*Text);
            Vector2 textPosition((Bounds.Width - textSize.X) / 2.0f + Bounds.X,
                                 (Bounds.Height - textSize.Y) / 2.0f + Bounds.Y);
            spriteBatch_->DrawString(*Font, *Text, textPosition, Color::White);
        }

        spriteBatch_->End();
    }

private:
    void HandleInput(MouseState mouseState) {
        bool pressed = false;
        Vector2 position;

        if (mouseState.getLeftButtonProperty() == ButtonState::Pressed) {
            pressed = true;
            position = Vector2((float)mouseState.getXProperty(), (float)mouseState.getYProperty());
        } else if (inputHelper_ && inputHelper_->IsPressed) {
            pressed = true;
            position = inputHelper_->PointPosition();
        } else {
            if (isPressed_) {
                Vector2 mousePos((float)mouseState.getXProperty(), (float)mouseState.getYProperty());
                if (IntersectWith(mousePos) || (inputHelper_ && IntersectWith(inputHelper_->PointPosition()))) {
                    FireClick();
                }
                isPressed_ = false;
            }
            isKeyDown_ = false;
        }

        if (pressed) {
            if (!isKeyDown_) {
                if (IntersectWith(position))
                    isPressed_ = true;
                isKeyDown_ = true;
            }
        } else {
            isKeyDown_ = false;
        }
    }

    bool IntersectWith(Vector2 position) const {
        Rectangle touchTap((int)position.X - 1, (int)position.Y - 1, 2, 2);
        return Bounds.Intersects(touchTap);
    }

    InputState& input_;
    std::string regularTextureName_;
    std::string pressedTextureName_;
    InputHelper* inputHelper_ = nullptr;
    std::optional<SpriteBatch> spriteBatch_;
    bool isKeyDown_ = false;
    bool isPressed_ = false;
};

} // namespace Blackjack

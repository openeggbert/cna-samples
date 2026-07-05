#pragma once

// TextDisplayComponent.hpp — C++ port of Elements/HUD/TextDisplayComponent.cs
// (XNA 4.0 NinjAcademy sample). A component used to display text at a
// specified location.

#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"

#include "../RestorableStateComponent.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;

// Port of Elements/HUD/TextDisplayComponent.cs.
class TextDisplayComponent : public RestorableStateComponent {
public:
    std::string Text;
    Vector2 Position;
    Color TextColor = Color::White;

    TextDisplayComponent(Game& game, SpriteFont& font)
        : RestorableStateComponent(game), font_(font),
          spriteBatch_(getGameProperty().getServicesProperty().GetService<SpriteBatch>()) {}

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        spriteBatch_->Begin();
        spriteBatch_->DrawString(font_, Text, Position, TextColor);
        spriteBatch_->End();
    }

private:
    SpriteFont& font_;
    SpriteBatch* spriteBatch_;
};

} // namespace NinjAcademy

#pragma once

// StaticTextureComponent.hpp — C++ port of
// Elements/General/StaticTextureComponent.cs (XNA 4.0 NinjAcademy sample).
// A component that simply displays a texture at a specified location.

#include "Microsoft/Xna/Framework/Color.hpp"

#include "TexturedDrawableGameComponent.hpp"

namespace NinjAcademy {

// Port of Elements/General/StaticTextureComponent.cs.
class StaticTextureComponent : public TexturedDrawableGameComponent {
public:
    StaticTextureComponent(Game& game, GameScreen* gameScreen, Texture2D texture, Vector2 position)
        : TexturedDrawableGameComponent(game, gameScreen, std::move(texture)) {
        Position = position;
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        spriteBatch_->Begin();
        spriteBatch_->Draw(texture_, Position, Color::White);
        spriteBatch_->End();
    }
};

} // namespace NinjAcademy

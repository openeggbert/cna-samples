#pragma once
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"

namespace SafeArea {

class AlignedSpriteBatch : public Microsoft::Xna::Framework::Graphics::SpriteBatch {
public:
    explicit AlignedSpriteBatch(Microsoft::Xna::Framework::Graphics::GraphicsDevice& graphicsDevice)
        : SpriteBatch(graphicsDevice) {}

    // DrawString(SpriteFont, text, position, color, Alignment) omitted — no SpriteFont in CNA yet.
};

} // namespace SafeArea

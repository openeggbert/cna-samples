#pragma once

#include <cmath>
#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace TouchThumbsticks {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// A single bullet fired by the player's ship.
class Bullet {
public:
    // One texture shared by all bullets, set once from the Game.
    inline static std::optional<Texture2D> SharedTexture;

    Vector2 Position;

    Bullet(const Vector2& pos, const Vector2& vel, const Color& color)
        : Position(pos), velocity_(vel), color_(color), rotation_(std::atan2(vel.Y, vel.X)) {}

    void Update() { Position = Position + velocity_; }

    void Draw(SpriteBatch& spriteBatch) const {
        spriteBatch.Draw(
            *SharedTexture,
            Position,
            std::optional<Rectangle>(std::nullopt),
            color_,
            rotation_,
            Vector2(static_cast<float>(SharedTexture->getWidthProperty()) / 2.0f,
                    static_cast<float>(SharedTexture->getHeightProperty()) / 2.0f),
            1.0f,
            SpriteEffects::None,
            0.0f);
    }

private:
    Vector2 velocity_;
    Color color_;
    float rotation_;
};

} // namespace TouchThumbsticks

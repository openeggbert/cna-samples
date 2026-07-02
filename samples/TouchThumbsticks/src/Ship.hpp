#pragma once

#include <optional>
#include <utility>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace TouchThumbsticks {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// Base class for all ships in the game.
class Ship {
public:
    Vector2 Position;
    Vector2 Velocity;
    float Rotation = 0.0f;

    explicit Ship(Texture2D texture) : texture_(std::move(texture)) {}
    virtual ~Ship() = default;

    // Used for collision detection with the bullets.
    [[nodiscard]] virtual bool ContainsPoint(const Vector2& /*point*/) const { return false; }

    virtual void Update(const GameTime& /*gameTime*/) {}

    virtual void Draw(SpriteBatch& spriteBatch) const {
        spriteBatch.Draw(
            texture_,
            Position,
            std::optional<Rectangle>(std::nullopt),
            Color::White,
            Rotation,
            Vector2(static_cast<float>(texture_.getWidthProperty()) / 2.0f,
                    static_cast<float>(texture_.getHeightProperty()) / 2.0f),
            1.0f,
            SpriteEffects::None,
            0.0f);
    }

protected:
    Texture2D texture_;
};

} // namespace TouchThumbsticks

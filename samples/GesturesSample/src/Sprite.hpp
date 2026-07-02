#pragma once

#include <array>
#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace GesturesSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// A cat sprite that can be moved, scaled, and re-colored by gestures. Port of
// the XNA 4.0 "TouchGestureSample" Sprite.cs.
class Sprite {
public:
    // Amount of velocity that is maintained after the sprite bounces off a wall.
    static constexpr float BounceMagnitude = 0.5f;

    // Percentage of velocity lost each second as the sprite moves around.
    static constexpr float Friction = 0.9f;

    static constexpr float MinScale = 0.5f;
    static constexpr float MaxScale = 2.0f;

    Vector2 Center;
    Color SpriteColor = Color::White;
    Vector2 Velocity;

    explicit Sprite(Texture2D texture) : texture_(std::move(texture)) {}

    [[nodiscard]] float getScale() const { return scale_; }

    void setScale(float value) { scale_ = MathHelper::Clamp(value, MinScale, MaxScale); }

    [[nodiscard]] Rectangle HitBounds() const {
        Rectangle r(
            static_cast<int>(Center.X - texture_.getWidthProperty() / 2 * scale_),
            static_cast<int>(Center.Y - texture_.getHeightProperty() / 2 * scale_),
            static_cast<int>(texture_.getWidthProperty() * scale_),
            static_cast<int>(texture_.getHeightProperty() * scale_));

        // Inflate the texture a little to give us some additional pad room.
        r.Inflate(10, 10);

        return r;
    }

    void ChangeColor() {
        colorIndex_ = (colorIndex_ + 1) % static_cast<int>(Palette().size());
        SpriteColor = Palette()[colorIndex_];
    }

    void Update(const GameTime& gameTime, const Rectangle& bounds) {
        float elapsed = static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

        Center = Center + Velocity * elapsed;
        Velocity = Velocity * (1.0f - (Friction * elapsed));

        float halfWidth = (texture_.getWidthProperty() * scale_) / 2.0f;
        float halfHeight = (texture_.getHeightProperty() * scale_) / 2.0f;

        // Check each side to make sure the sprite is in the bounds. If the
        // sprite is outside the bounds, move it back and reverse velocity on
        // that axis.

        if (Center.X < bounds.getLeftProperty() + halfWidth) {
            Center.X = static_cast<float>(bounds.getLeftProperty()) + halfWidth;
            Velocity.X = Velocity.X * -BounceMagnitude;
        }

        if (Center.X > bounds.getRightProperty() - halfWidth) {
            Center.X = static_cast<float>(bounds.getRightProperty()) - halfWidth;
            Velocity.X = Velocity.X * -BounceMagnitude;
        }

        if (Center.Y < bounds.getTopProperty() + halfHeight) {
            Center.Y = static_cast<float>(bounds.getTopProperty()) + halfHeight;
            Velocity.Y = Velocity.Y * -BounceMagnitude;
        }

        if (Center.Y > bounds.getBottomProperty() - halfHeight) {
            Center.Y = static_cast<float>(bounds.getBottomProperty()) - halfHeight;
            Velocity.Y = Velocity.Y * -BounceMagnitude;
        }
    }

    void Draw(SpriteBatch& spriteBatch) const {
        spriteBatch.Draw(
            texture_,
            Center,
            std::optional<Rectangle>(std::nullopt),
            SpriteColor,
            0.0f,
            Vector2(static_cast<float>(texture_.getWidthProperty()) / 2.0f,
                    static_cast<float>(texture_.getHeightProperty()) / 2.0f),
            scale_,
            SpriteEffects::None,
            0.0f);
    }

private:
    Texture2D texture_;
    int colorIndex_ = 0;
    float scale_ = 1.0f;

    static const std::array<Color, 4>& Palette() {
        static const std::array<Color, 4> colors = {Color::White, Color::Red, Color::Blue, Color::Green};
        return colors;
    }
};

} // namespace GesturesSample

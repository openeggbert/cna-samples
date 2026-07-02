#pragma once

#include <cmath>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "Ship.hpp"

namespace TouchThumbsticks {

using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// An enemy ship that flies straight towards the player.
class EnemyShip : public Ship {
public:
    // Non-owning; the player ship outlives every EnemyShip.
    Ship* Player = nullptr;

    explicit EnemyShip(Texture2D texture) : Ship(std::move(texture)) {
        // Radius of the ship, used for collision detection.
        radius_ = std::sqrt(static_cast<float>(texture_.getWidthProperty() * texture_.getWidthProperty() +
                                                texture_.getHeightProperty() * texture_.getHeightProperty())) *
                  0.75f;
    }

    void Update(const GameTime& /*gameTime*/) override {
        Vector2 d = Vector2::Normalize(Player->Position - Position);
        Rotation = std::atan2(d.Y, d.X);
        Position = Position + d * 4.0f;
    }

    [[nodiscard]] bool ContainsPoint(const Vector2& point) const override {
        return Vector2::Distance(Position, point) < radius_;
    }

private:
    float radius_ = 0.0f;
};

} // namespace TouchThumbsticks

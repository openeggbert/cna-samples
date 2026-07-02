#pragma once

#include <cmath>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/TimeSpan.hpp"

#include "Bullet.hpp"
#include "Ship.hpp"
#include "VirtualThumbsticks.hpp"

namespace TouchThumbsticks {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// The ship controlled by the player, driven by the two virtual thumbsticks:
// left moves the ship, right aims and fires.
class PlayerShip : public Ship {
public:
    std::vector<Bullet> Bullets;

    // World bounds, used to keep the player in-bounds.
    int WorldWidth = 0;
    int WorldHeight = 0;

    explicit PlayerShip(Texture2D texture) : Ship(std::move(texture)) {}

    void Update(const GameTime& gameTime) override {
        // Adjust velocity based on the left thumbstick, then apply drag.
        Velocity = Velocity + VirtualThumbsticks::getLeftThumbstick() * Acceleration;
        Position = Position + Velocity;
        Velocity = Velocity * 0.98f;

        fireTimer_ = fireTimer_ - gameTime.getElapsedGameTimeProperty();

        if (VirtualThumbsticks::getRightThumbstick().Length() > 0.3f) {
            Vector2 aim = VirtualThumbsticks::getRightThumbstick();
            Rotation = -std::atan2(-aim.Y, aim.X);

            if (fireTimer_ <= System::TimeSpan::Zero) {
                Vector2 bulletVelocity = Vector2::Normalize(VirtualThumbsticks::getRightThumbstick()) * BulletSpeed;
                Bullets.emplace_back(Position, bulletVelocity, Color::Red);
                fireTimer_ = Cooldown;
            }
        } else if (VirtualThumbsticks::getLeftThumbstick().Length() > 0.2f) {
            Vector2 aim = VirtualThumbsticks::getLeftThumbstick();
            Rotation = -std::atan2(-aim.Y, aim.X);
        }

        for (auto& b : Bullets) b.Update();

        for (std::size_t i = Bullets.size(); i-- > 0;) {
            const Vector2& p = Bullets[i].Position;
            if (p.X < -WorldWidth / 2.0f || p.X > WorldWidth / 2.0f ||
                p.Y < -WorldHeight / 2.0f || p.Y > WorldHeight / 2.0f) {
                Bullets.erase(Bullets.begin() + static_cast<std::ptrdiff_t>(i));
            }
        }

        ClampPlayerShip();
    }

    void Draw(SpriteBatch& spriteBatch) const override {
        for (const auto& b : Bullets) b.Draw(spriteBatch);
        Ship::Draw(spriteBatch);
    }

private:
    static constexpr float Acceleration = 0.75f;
    static constexpr float BulletSpeed = 20.0f;
    inline static const System::TimeSpan Cooldown = System::TimeSpan::FromSeconds(0.15);

    System::TimeSpan fireTimer_;

    void ClampPlayerShip() {
        if (Position.X < -WorldWidth / 2.0f) {
            Position.X = static_cast<float>(-WorldWidth) / 2.0f;
            if (Velocity.X < 0.0f) Velocity.X = 0.0f;
        }

        if (Position.X > WorldWidth / 2.0f) {
            Position.X = static_cast<float>(WorldWidth) / 2.0f;
            if (Velocity.X > 0.0f) Velocity.X = 0.0f;
        }

        if (Position.Y < -WorldHeight / 2.0f) {
            Position.Y = static_cast<float>(-WorldHeight) / 2.0f;
            if (Velocity.Y < 0.0f) Velocity.Y = 0.0f;
        }

        if (Position.Y > WorldHeight / 2.0f) {
            Position.Y = static_cast<float>(WorldHeight) / 2.0f;
            if (Velocity.Y > 0.0f) Velocity.Y = 0.0f;
        }
    }
};

} // namespace TouchThumbsticks

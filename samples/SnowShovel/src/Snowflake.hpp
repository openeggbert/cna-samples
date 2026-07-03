#pragma once

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "System/Random.hpp"

namespace SnowShovel {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;

// Encapsulates data for drawing one falling/bouncing snowflake sprite.
// Port of the XNA original's private nested `Snowflake` class in Game.cs.
class Snowflake {
public:
    Vector2 Position;
    Vector2 Velocity;
    float Scale;
    float Rotation = 0.0f;
    float AngularVelocity;
    int TextureIndex;
    Color Tint;

    // Create a new snowflake with a random scale, spin, and pastel tint.
    Snowflake(System::Random& rand, const Vector2& position, const Vector2& velocity, int index)
        : Position(position), Velocity(velocity), TextureIndex(index),
          Tint(
              (int)(255 - (40.0 * rand.NextDouble())),
              (int)(255 - (40.0 * rand.NextDouble())),
              (int)(255 - (20.0 * rand.NextDouble())),
              255) {
        Scale = 1.0f - (float)(0.5 * rand.NextDouble());
        AngularVelocity = (float)rand.NextDouble() * 0.05f;
    }

    // Update the snowflake's position, bouncing off the world bounds.
    void Update(const Rectangle& screen) {
        Position = Position + Velocity;

        if (Position.X < 0) {
            Velocity.X = -Velocity.X;
            Position.X = 0.0f;
        } else if (Position.X > screen.Width) {
            Velocity.X = -Velocity.X;
            Position.X = (float)screen.Width;
        }

        if (Position.Y < 0) {
            Velocity.Y = -Velocity.Y;
            Position.Y = 0.0f;
        } else if (Position.Y > screen.Height) {
            Velocity.Y = -Velocity.Y;
            Position.Y = (float)screen.Height;
        }

        // this is just for visuals, no need for bounds check
        Rotation += AngularVelocity;
    }
};

} // namespace SnowShovel

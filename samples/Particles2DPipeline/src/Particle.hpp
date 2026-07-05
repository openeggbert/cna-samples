#pragma once

// Particle.hpp — C++ port of Particle.cs (XNA 4.0 Particles2DPipeline sample).
// Particles are the little bits that make up an effect.

#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "ParticleHelpers.hpp"

namespace Particles2DPipelineSample {

using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Vector2;

// Port of Particle.cs.
struct Particle {
    Vector2 Position;
    Vector2 Velocity;
    Vector2 Acceleration;

    float Lifetime       = 0.0f;
    float TimeSinceStart = 0.0f;
    float Scale          = 1.0f;
    float Rotation       = 0.0f;
    float RotationSpeed  = 0.0f;

    // Once TimeSinceStart becomes greater than Lifetime, the particle should
    // no longer be drawn or updated.
    bool Active() const { return TimeSinceStart < Lifetime; }

    void Initialize(Vector2 position, Vector2 velocity, Vector2 acceleration,
                     float lifetime, float scale, float rotationSpeed) {
        Position     = position;
        Velocity     = velocity;
        Acceleration = acceleration;
        Lifetime     = lifetime;
        Scale        = scale;
        RotationSpeed = rotationSpeed;

        // Reset -- particles are reused.
        TimeSinceStart = 0.0f;

        Rotation = ParticleHelpers::RandomBetween(0.0f, MathHelper::TwoPi);
    }

    void Update(float dt) {
        Velocity = Velocity + Acceleration * dt;
        Position = Position + Velocity * dt;

        Rotation += RotationSpeed * dt;

        TimeSinceStart += dt;
    }
};

} // namespace Particles2DPipelineSample

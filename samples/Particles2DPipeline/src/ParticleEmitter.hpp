#pragma once

// ParticleEmitter.hpp — C++ port of ParticleEmitter.cs (XNA 4.0
// Particles2DPipeline sample). Helper for objects that want to leave
// particles behind them as they move, spacing particles evenly along the
// path travelled between updates regardless of frame rate or emission rate.

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "ParticleSystem.hpp"

namespace Particles2DPipelineSample {

using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Vector2;

// Port of ParticleEmitter.cs.
class ParticleEmitter {
public:
    ParticleEmitter(ParticleSystem& particleSystem, float particlesPerSecond, Vector2 initialPosition)
        : particleSystem_(particleSystem), timeBetweenParticles_(1.0f / particlesPerSecond),
          position_(initialPosition) {}

    Vector2 Position() const { return position_; }

    void Update(GameTime& gameTime, Vector2 newPosition) {
        float elapsedTime = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        if (elapsedTime > 0.0f) {
            Vector2 velocity = (newPosition - position_) / elapsedTime;

            float timeToSpend = timeLeftOver_ + elapsedTime;

            float currentTime = -timeLeftOver_;

            while (timeToSpend > timeBetweenParticles_) {
                currentTime += timeBetweenParticles_;
                timeToSpend -= timeBetweenParticles_;

                float mu = currentTime / elapsedTime;
                Vector2 particlePosition = Vector2::Lerp(position_, newPosition, mu);

                particleSystem_.AddParticles(particlePosition, velocity);
            }

            timeLeftOver_ = timeToSpend;
        }

        position_ = newPosition;
    }

private:
    ParticleSystem& particleSystem_;
    float timeBetweenParticles_;
    Vector2 position_;
    float timeLeftOver_ = 0.0f;
};

} // namespace Particles2DPipelineSample

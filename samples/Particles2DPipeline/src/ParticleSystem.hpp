#pragma once

// ParticleSystem.hpp — C++ port of ParticleSystem.cs and
// ParticleSettings/ParticleSystemSettings.cs (XNA 4.0 Particles2DPipeline
// sample). The XML content files under Content/*.xml describe each particle
// system's tuning; CNA has no generic content-pipeline deserializer for
// custom types, so per this project's established convention (DynamicMenu,
// NinjAcademy) the handful of XML files are hand-translated into the
// BuildXxxSettings() functions below rather than parsed at runtime.

#include <cmath>
#include <queue>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/Blend.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteSortMode.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "Particle.hpp"
#include "ParticleHelpers.hpp"

namespace Particles2DPipelineSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::Blend;
using Microsoft::Xna::Framework::Graphics::BlendState;
using Microsoft::Xna::Framework::Graphics::GraphicsDevice;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::SpriteSortMode;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// Port of ParticlesSettings.AccelerationMode.
enum class AccelerationMode {
    None,
    Scalar,
    EndVelocity,
    Vector
};

// Port of ParticlesSettings.ParticleSystemSettings. Defaults mirror the
// [ContentSerializer(Optional = true)] defaults in the original.
struct ParticleSystemSettings {
    int MinNumParticles = 0;
    int MaxNumParticles = 0;

    std::string TextureFilename;

    float MinDirectionAngle = 0.0f;
    float MaxDirectionAngle = 360.0f;

    float MinInitialSpeed = 0.0f;
    float MaxInitialSpeed = 0.0f;

    AccelerationMode Mode = AccelerationMode::None;

    float EndVelocity = 1.0f;

    float MinAccelerationScale = 0.0f;
    float MaxAccelerationScale = 0.0f;

    Vector2 MinAccelerationVector = Vector2::Zero;
    Vector2 MaxAccelerationVector = Vector2::Zero;

    float EmitterVelocitySensitivity = 0.0f;

    float MinRotationSpeed = 0.0f;
    float MaxRotationSpeed = 0.0f;

    float MinLifetime = 0.0f;
    float MaxLifetime = 0.0f;

    float MinSize = 1.0f;
    float MaxSize = 1.0f;

    Vector2 Gravity = Vector2::Zero;

    Blend SourceBlend = Blend::One;
    Blend DestinationBlend = Blend::InverseSourceAlpha;
};

// Hand-translated from Content/ExplosionSettings.xml.
inline ParticleSystemSettings BuildExplosionSettings() {
    ParticleSystemSettings s;
    s.MinNumParticles = 10;
    s.MaxNumParticles = 12;
    s.TextureFilename = "explosion";
    s.MinInitialSpeed = 40.0f;
    s.MaxInitialSpeed = 500.0f;
    s.Mode = AccelerationMode::EndVelocity;
    s.EndVelocity = 0.0f;
    s.MinRotationSpeed = -45.0f;
    s.MaxRotationSpeed = 45.0f;
    s.MinLifetime = 0.5f;
    s.MaxLifetime = 1.0f;
    s.MinSize = 0.3f;
    s.MaxSize = 1.0f;
    s.SourceBlend = Blend::SourceAlpha;
    s.DestinationBlend = Blend::One;
    return s;
}

// Hand-translated from Content/ExplosionSmokeSettings.xml.
inline ParticleSystemSettings BuildExplosionSmokeSettings() {
    ParticleSystemSettings s;
    s.MinNumParticles = 5;
    s.MaxNumParticles = 10;
    s.TextureFilename = "smoke";
    s.MinInitialSpeed = 20.0f;
    s.MaxInitialSpeed = 200.0f;
    s.Mode = AccelerationMode::Scalar;
    s.MinAccelerationScale = -10.0f;
    s.MaxAccelerationScale = -50.0f;
    s.MinRotationSpeed = -45.0f;
    s.MaxRotationSpeed = 45.0f;
    s.MinLifetime = 1.0f;
    s.MaxLifetime = 2.5f;
    s.MinSize = 1.0f;
    s.MaxSize = 2.0f;
    return s;
}

// Hand-translated from Content/SmokePlumeSettings.xml.
inline ParticleSystemSettings BuildSmokePlumeSettings() {
    ParticleSystemSettings s;
    s.MinNumParticles = 2;
    s.MaxNumParticles = 8;
    s.TextureFilename = "smoke";
    s.MinDirectionAngle = 260.0f;
    s.MaxDirectionAngle = 280.0f;
    s.MinInitialSpeed = 20.0f;
    s.MaxInitialSpeed = 100.0f;
    s.Mode = AccelerationMode::Vector;
    s.MinAccelerationVector = Vector2(10.0f, 0.0f);
    s.MaxAccelerationVector = Vector2(50.0f, 0.0f);
    s.MinRotationSpeed = -22.5f;
    s.MaxRotationSpeed = 22.5f;
    s.MinLifetime = 5.0f;
    s.MaxLifetime = 7.0f;
    s.MinSize = 0.5f;
    s.MaxSize = 1.0f;
    return s;
}

// Hand-translated from Content/EmitterSettings.xml.
inline ParticleSystemSettings BuildEmitterSettings() {
    ParticleSystemSettings s;
    s.MinNumParticles = 1;
    s.MaxNumParticles = 5;
    s.TextureFilename = "BlockParticle";
    s.MinDirectionAngle = 260.0f;
    s.MaxDirectionAngle = 280.0f;
    s.MinInitialSpeed = 20.0f;
    s.MaxInitialSpeed = 200.0f;
    s.Mode = AccelerationMode::None;
    s.MinRotationSpeed = -45.0f;
    s.MaxRotationSpeed = 45.0f;
    s.MinLifetime = 1.0f;
    s.MaxLifetime = 2.5f;
    s.MinSize = 0.5f;
    s.MaxSize = 1.0f;
    s.Gravity = Vector2(0.0f, 300.0f);
    return s;
}

// Port of ParticleSystem.cs. Unlike the sibling ParticleSample port (which
// used per-effect subclasses of an abstract base), this original defines a
// single, data-driven ParticleSystem class configured entirely by a
// ParticleSystemSettings value -- ported the same way here.
//
// The original inherits DrawableGameComponent and is registered in
// Game.Components so Update/Draw run automatically. This port instead
// manages each ParticleSystem as a plain member of ParticleSampleGame with
// explicit Update()/Draw() calls, matching the precedent already set by
// samples/ParticleSample (see its missing.md) -- avoids the ownership
// complexity of GameComponent-based lifetime for an object with no need for
// automatic dispatch, since the game already needs an explicit draw order
// (alpha-blended systems before additive ones) regardless.
class ParticleSystem {
public:
    static constexpr int AlphaBlendDrawOrder = 100;
    static constexpr int AdditiveDrawOrder = 200;

    int DrawOrder = AlphaBlendDrawOrder;

    ParticleSystem(GraphicsDevice& graphicsDevice, ParticleSystemSettings settings, Texture2D texture,
                   int initialParticleCount = 10)
        : settings_(settings), texture_(texture), spriteBatch_(graphicsDevice) {
        origin_ = Vector2((float)(texture_.getWidthProperty() / 2), (float)(texture_.getHeightProperty() / 2));

        blendState_.setColorSourceBlendProperty(settings_.SourceBlend);
        blendState_.setAlphaSourceBlendProperty(settings_.SourceBlend);
        blendState_.setColorDestinationBlendProperty(settings_.DestinationBlend);
        blendState_.setAlphaDestinationBlendProperty(settings_.DestinationBlend);

        particles_.reserve(initialParticleCount);
        for (int i = 0; i < initialParticleCount; i++) {
            particles_.emplace_back();
            freeParticles_.push(&particles_.back());
        }
    }

    int FreeParticleCount() const { return (int)freeParticles_.size(); }

    // AddParticles adds an effect somewhere on the screen. If there aren't
    // enough free particles, it uses as many as it can.
    void AddParticles(Vector2 where, Vector2 velocity) {
        int numParticles = ParticleHelpers::Random.Next(settings_.MinNumParticles, settings_.MaxNumParticles);

        for (int i = 0; i < numParticles; i++) {
            if (freeParticles_.empty()) {
                // Grow by 10, same as the original.
                particles_.reserve(particles_.size() + 10);
                for (int j = 0; j < 10; j++) {
                    particles_.emplace_back();
                }
                // particles_ may have reallocated -- rebuild the free queue
                // pointers from the tail 10 entries just added.
                for (size_t j = particles_.size() - 10; j < particles_.size(); j++) {
                    freeParticles_.push(&particles_[j]);
                }
            }

            Particle* p = freeParticles_.front();
            freeParticles_.pop();
            InitializeParticle(*p, where, velocity);
        }
    }

    void Update(float dt) {
        for (auto& p : particles_) {
            if (p.Active()) {
                p.Acceleration = p.Acceleration + settings_.Gravity * dt;
                p.Update(dt);
                if (!p.Active())
                    freeParticles_.push(&p);
            }
        }
    }

    void Draw() {
        spriteBatch_.Begin(SpriteSortMode::Deferred, blendState_);

        for (auto& p : particles_) {
            if (!p.Active())
                continue;

            float normalizedLifetime = p.TimeSinceStart / p.Lifetime;

            // Fade in and out: 4 * t * (1-t) peaks at 1.0 when t = 0.5.
            float alpha = 4.0f * normalizedLifetime * (1.0f - normalizedLifetime);
            Color color((int)(255 * alpha), (int)(255 * alpha), (int)(255 * alpha), (int)(255 * alpha));

            // Particles grow from 75% to 100% of their size over their life.
            float scale = p.Scale * (0.75f + 0.25f * normalizedLifetime);

            spriteBatch_.Draw(texture_, p.Position, std::nullopt, color, p.Rotation, origin_, scale,
                              SpriteEffects::None, 0.0f);
        }

        spriteBatch_.End();
    }

private:
    Vector2 PickRandomDirection() {
        float angle = ParticleHelpers::RandomBetween(settings_.MinDirectionAngle, settings_.MaxDirectionAngle);
        angle = MathHelper::ToRadians(angle);
        return Vector2(std::cos(angle), std::sin(angle));
    }

    void InitializeParticle(Particle& p, Vector2 where, Vector2 velocity) {
        velocity = velocity * settings_.EmitterVelocitySensitivity;

        Vector2 direction = PickRandomDirection();
        float speed = ParticleHelpers::RandomBetween(settings_.MinInitialSpeed, settings_.MaxInitialSpeed);
        velocity = velocity + direction * speed;

        float lifetime = ParticleHelpers::RandomBetween(settings_.MinLifetime, settings_.MaxLifetime);
        float scale = ParticleHelpers::RandomBetween(settings_.MinSize, settings_.MaxSize);
        float rotationSpeed = ParticleHelpers::RandomBetween(settings_.MinRotationSpeed, settings_.MaxRotationSpeed);
        rotationSpeed = MathHelper::ToRadians(rotationSpeed);

        Vector2 acceleration = Vector2::Zero;
        switch (settings_.Mode) {
            case AccelerationMode::Scalar: {
                float accelerationScale =
                    ParticleHelpers::RandomBetween(settings_.MinAccelerationScale, settings_.MaxAccelerationScale);
                acceleration = direction * accelerationScale;
                break;
            }
            case AccelerationMode::EndVelocity:
                // vt = v0 + a0*t  =>  a0 = (vt - v0) / t, with vt = v0 * EndVelocity.
                acceleration = (velocity * (settings_.EndVelocity - 1.0f)) / lifetime;
                break;
            case AccelerationMode::Vector:
                acceleration = Vector2(
                    ParticleHelpers::RandomBetween(settings_.MinAccelerationVector.X, settings_.MaxAccelerationVector.X),
                    ParticleHelpers::RandomBetween(settings_.MinAccelerationVector.Y, settings_.MaxAccelerationVector.Y));
                break;
            case AccelerationMode::None:
            default:
                break;
        }

        p.Initialize(where, velocity, acceleration, lifetime, scale, rotationSpeed);
    }

    ParticleSystemSettings settings_;
    Texture2D texture_;
    Vector2 origin_;
    SpriteBatch spriteBatch_;
    BlendState blendState_;

    std::vector<Particle> particles_;
    std::queue<Particle*> freeParticles_;
};

} // namespace Particles2DPipelineSample

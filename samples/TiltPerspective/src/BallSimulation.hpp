#pragma once

// Port of BallSimulation.cs (XNA 4.0 TiltPerspective sample) -- sphere/sphere
// and sphere/plane collision physics, with gravity driven directly by
// IAccelerometerService::RawAcceleration (fed either by a real accelerometer
// on the original, or by this port's own keyboard-tilt substitute -- see
// AccelerometerHelper.hpp). This file needed NO change at all to account for
// that substitution: it only ever reads RawAcceleration through the same
// service-locator interface the original used, exactly matching this task's
// design goal of touching the input source, not the physics.

#include <array>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameComponent.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Plane.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "System/Random.hpp"

#include "AccelerometerHelper.hpp"
#include "RandomUtil.hpp"
#include "SpherePrimitive.hpp"

namespace TiltPerspectiveSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

// Matches the original's plain `Ball` class.
struct Ball {
    Vector3 Position;
    Vector3 Velocity;

    float Radius = 0.0f;
    float Mass = 0.0f;
    Color BallColor{0, 0, 0, 255};
};

class BallSimulation : public GameComponent {
public:
    explicit BallSimulation(Game& game) : GameComponent(game) {}

    void AddWall(const Plane& wallPlane) { walls_.push_back(wallPlane); }

    void AddWalls(const BoundingBox& worldBox) {
        AddWall(Plane(-Vector3::UnitX, worldBox.Max.X));
        AddWall(Plane(Vector3::UnitX, -worldBox.Min.X));
        AddWall(Plane(-Vector3::UnitY, worldBox.Max.Y));
        AddWall(Plane(Vector3::UnitY, -worldBox.Min.Y));
        // Matches the original: the near/screen-side wall (-UnitZ, Max.Z) is
        // intentionally never added, so balls can be seen popping toward the
        // viewer with nothing stopping them at the screen plane. DebugDraw's
        // own CreateBoxInterior() likewise never emits geometry for that face.
        // AddWall(Plane(-Vector3::UnitZ, worldBox.Max.Z));
        AddWall(Plane(Vector3::UnitZ, -worldBox.Min.Z));
    }

    // Adds `n` balls with random radius/position inside `inBox`.
    void AddBalls(int n, float minRadius, float maxRadius, const BoundingBox& inBox) {
        System::Random rng;

        for (int i = 0; i < n; ++i) {
            float r = NextFloat(rng, minRadius, maxRadius);

            Ball newBall;
            newBall.Velocity = Vector3::Zero;
            newBall.BallColor = ballColors_[static_cast<std::size_t>(i) % ballColors_.size()];
            newBall.Radius = r;
            newBall.Position = Vector3(NextFloat(rng, inBox.Min.X + r, inBox.Max.X - r),
                                        NextFloat(rng, inBox.Min.Y + r, inBox.Max.Y - r),
                                        NextFloat(rng, inBox.Min.Z + r, inBox.Max.Z - r));
            // Since the spheres we draw are kind of plastic looking, it looks
            // better if we make mass proportional to r^2 (like a hollow
            // inflatable ball) rather than r^3 (solid).
            newBall.Mass = r * r;

            balls_.push_back(newBall);
        }
    }

    void Initialize() override {
        GameComponent::Initialize();

        spherePrimitive_ = std::make_unique<SpherePrimitive>(getGameProperty().getGraphicsDeviceProperty());
    }

    void Update(GameTime& gameTime) override {
        // We break each game update into multiple simulation substeps, to get
        // more accurate handling of multi-object collisions.
        constexpr int substeps = 4;
        float dt = static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());
        for (int i = 0; i < substeps; ++i) {
            UpdateBalls(dt / substeps);
        }
        GameComponent::Update(gameTime);
    }

    void Draw(const Matrix& view, const Matrix& projection, Vector3 lightDirection) {
        GraphicsDevice& graphicsDevice = getGameProperty().getGraphicsDeviceProperty();

        spherePrimitive_->setLightDirectionProperty(lightDirection);

        // NOXNA simplification: the original also sets
        // `RasterizerState.MultiSampleAntiAlias = true` on a fresh instance
        // here; CNA's RasterizerState presets (used directly per CLAUDE.md)
        // don't expose a distinct MSAA-only variant, and the fill/cull mode
        // below is identical to the default anyway. Kept the cull mode only.
        graphicsDevice.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);

        for (const Ball& ball : balls_) {
            Matrix world = Matrix::CreateScale(ball.Radius * 2.0f);
            world.setTranslationProperty(ball.Position);
            spherePrimitive_->Draw(world, view, projection, ball.BallColor, false);
        }

        // Since we know where the ground planes are, generate simple shadows
        // by squashing the geometry onto these planes and rendering it in black.
        for (const Plane& plane : walls_) {
            constexpr float epsilon = 0.01f;
            constexpr float planeOffset = 0.5f; // offset shadows to avoid depth buffer artifacts
            if (plane.DotNormal(lightDirection) >= -epsilon)
                continue;

            Matrix shadowMatrix = Matrix::CreateShadow(lightDirection, Plane(-plane.Normal, planeOffset - plane.D));

            for (const Ball& ball : balls_) {
                Matrix world = Matrix::CreateScale(ball.Radius * 2.0f);
                world.setTranslationProperty(ball.Position);

                world = world * shadowMatrix;
                spherePrimitive_->Draw(world, view, projection, Color::Black, false);
            }
        }

        // Reset the fill mode renderstate.
        graphicsDevice.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
    }

private:
    static constexpr float wallElasticity_ = 0.75f;
    static constexpr float ballElasticity_ = 0.75f;

    std::unique_ptr<SpherePrimitive> spherePrimitive_;

    std::vector<Ball> balls_;
    std::vector<Plane> walls_;

    const std::array<Color, 5> ballColors_{Color::Red, Color::Green, Color::Blue, Color::White, Color::Black};

    // Given 2 balls with velocity, mass, and size, evaluate whether a
    // collision occurred. If it did, move them to be non-penetrating and
    // update their velocities.
    static void SphereCollisionImplicit(Ball& ball1, Ball& ball2) {
        Vector3 relativepos = ball2.Position - ball1.Position;
        float distance2 = relativepos.LengthSquared();
        float radii = ball1.Radius + ball2.Radius;
        if (distance2 >= radii * radii)
            return; // no collision

        float distance = relativepos.Length();
        Vector3 relativeUnit = relativepos * (1.0f / distance);
        Vector3 penetration = relativeUnit * (radii - distance);

        // Adjust the spheres' relative positions
        float mass1 = ball1.Mass;
        float mass2 = ball2.Mass;

        float mInv = 1.0f / (mass1 + mass2);
        float weight1 = mass1 * mInv; // relative weight of ball 1
        float weight2 = mass2 * mInv; // relative weight of ball 2. w1+w2==1.0

        ball1.Position -= weight2 * penetration;
        ball2.Position += weight1 * penetration;

        // Adjust the objects' relative velocities, if they are moving toward
        // each other.
        //
        // Note that we're assuming no friction, or equivalently, no angular
        // momentum.
        //
        // velocityTotal = velocity of v2 in v1 stationary ref. frame
        // get reference frame of common center of mass
        Vector3 centerVelocity = ball1.Velocity * weight1 + ball2.Velocity * weight2;
        Vector3 relativeMomentum = (ball2.Velocity - centerVelocity) * mass2;
        float contactImpulse = Vector3::Dot(relativeMomentum, relativeUnit);
        if (contactImpulse < 0) {
            relativeMomentum -= relativeUnit * (contactImpulse * (ballElasticity_ + 1));
            ball1.Velocity = (-relativeMomentum) / mass1 + centerVelocity;
            ball2.Velocity = relativeMomentum / mass2 + centerVelocity;
        }
    }

    // Evaluates ball physics, generates new velocities based on collisions,
    // and updates ball positions.
    void UpdateBalls(float dt) {
        auto* accel = getGameProperty().getServicesProperty().GetService<IAccelerometerService>();
        Vector3 gravity = accel->getRawAccelerationProperty() * 2000.0f;

        // Update positions, add air friction and gravity
        for (Ball& ball : balls_) {
            ball.Position += ball.Velocity * dt;
            ball.Velocity += gravity * dt;
        }

        // Resolve ball-ball collisions
        std::size_t ballCount = balls_.size();
        for (std::size_t i = 0; i < ballCount; ++i) {
            for (std::size_t j = i + 1; j < ballCount; ++j) {
                SphereCollisionImplicit(balls_[i], balls_[j]);
            }
        }

        // Resolve collisions with walls
        for (std::size_t i = 0; i < ballCount; ++i) {
            Ball& ball = balls_[i];

            for (const Plane& wall : walls_) {
                float depth = wall.DotCoordinate(ball.Position) - ball.Radius;
                if (depth < 0) {
                    // collision with wall
                    ball.Position -= wall.Normal * depth;

                    float impact = wall.DotNormal(ball.Velocity);
                    if (impact < 0) {
                        ball.Velocity -= wall.Normal * (impact * (wallElasticity_ + 1));
                    }
                }
            }
        }
    }
};

} // namespace TiltPerspectiveSample

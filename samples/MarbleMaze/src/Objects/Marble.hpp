#pragma once

// Marble.hpp — C++ port of Objects/Marble.cs (XNA 4.0 MarbleMaze sample). Pure
// application-level physics math, ported directly (see missing.md for what changed
// vs. the original Model-based rendering -- RawMesh.hpp, DEFERRED.md item #26).

#include <algorithm>
#include <cmath>
#include <memory>

#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "DrawableComponent3D.hpp"
#include "Maze.hpp"
#include "RawMesh.hpp"
#include "../Misc/AudioManager.hpp"
#include "../Misc/TriangleSphereCollisionDetection.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::BoundingSphere;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Graphics::BasicEffect;
using Microsoft::Xna::Framework::Graphics::SamplerState;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class Marble : public DrawableComponent3D {
public:
    Maze* MazePtr = nullptr;
    float angleX = 0.0f;
    float angleZ = 0.0f;

    explicit Marble(Game& game) : DrawableComponent3D(game) { preferPerPixelLighting_ = true; }

    // Model.Meshes[0].BoundingSphere.Transform(AbsoluteBoneTransforms[0]) in the
    // original -- AbsoluteBoneTransforms[0] is identity for this single-bone model,
    // so this reduces to the mesh's own local bounding sphere offset by Position.
    BoundingSphere BoundingSphereTransformed() const {
        BoundingSphere result = localBoundingSphere_;
        result.Center = result.Center + Position;
        return result;
    }

    void Update(GameTime& gameTime) override {
        DrawableComponent3D::Update(gameTime);

        CameraPtr->ObjectToFollow =
            Vector3::Transform(Position, Matrix::CreateFromYawPitchRoll(MazePtr->Rotation.Y, MazePtr->Rotation.X,
                                                                          MazePtr->Rotation.Z));

        PlaySounds();
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        auto& device = getGraphicsDeviceProperty();
        SamplerState originalSampler = device.getSamplerStatesProperty()[0];
        device.getSamplerStatesProperty()[0] = SamplerState::LinearClamp;

        effect_->EnableDefaultLighting();
        effect_->setPreferPerPixelLightingProperty(preferPerPixelLighting_);
        effect_->setTextureEnabledProperty(true);
        effect_->setTextureProperty(&marbleTexture_);
        effect_->Projection = CameraPtr->Projection;
        effect_->View = CameraPtr->View;
        effect_->World = FinalWorldTransforms;

        mesh_.Draw(*effect_, device);

        device.getSamplerStatesProperty()[0] = originalSampler;
    }

protected:
    void LoadContent() override {
        auto& content = getGameProperty().getContentProperty();
        auto& device = getGraphicsDeviceProperty();

        mesh_.Load(content.getRootDirectoryProperty(), "Models/marble", device);
        marbleTexture_ = content.Load<Texture2D>("Textures/Marble");

        localBoundingSphere_ = BoundingSphere::CreateFromPoints(mesh_.Positions());

        effect_ = std::make_unique<BasicEffect>(device);
    }

    // Properly place the marble in the game world.
    void UpdateFinalWorldTransform() override {
        rollMatrix_ = rollMatrix_ * (Matrix::CreateFromAxisAngle(Vector3::Right, Rotation.Z) *
                                      Matrix::CreateFromAxisAngle(Vector3::Forward, Rotation.X));

        FinalWorldTransforms = rollMatrix_ * Matrix::CreateTranslation(Position) *
                                Matrix::CreateFromYawPitchRoll(MazePtr->Rotation.Y, MazePtr->Rotation.X,
                                                                MazePtr->Rotation.Z);
    }

    // Perform collision checks with the maze.
    void CalculateCollisions() override {
        MazePtr->GetCollisionDetails(BoundingSphereTransformed(), intersectDetails_, false);

        if (intersectDetails_.IntersectWithWalls) {
            for (const auto& triangle : intersectDetails_.IntersectedWallTriangle) {
                int direction = CollideDirection(triangle);
                if ((direction & AxisX) == AxisX && (direction & AxisZ) == AxisZ) {
                    MazePtr->GetCollisionDetails(BoundingSphereTransformed(), intersectDetails_, true);
                }
            }
        }
    }

    // Calculate the marble's acceleration according to the maze's tilt.
    void CalculateAcceleration() override {
        if (intersectDetails_.IntersectWithGround) {
            angleX = 0;
            angleZ = 0;
            if (intersectDetails_.IntersectedGroundTriangle.has_value()) {
                intersectDetails_.IntersectedGroundTriangle->Normal(normal_);
                angleX = std::atan(normal_.Y / normal_.X);
                angleZ = std::atan(normal_.Y / normal_.Z);

                if (angleX > 0)
                    angleX = MathHelper::PiOver2 - angleX;
                else if (angleX < 0)
                    angleX = -(angleX + MathHelper::PiOver2);

                if (angleZ > 0)
                    angleZ = MathHelper::PiOver2 - angleZ;
                else if (angleZ < 0)
                    angleZ = -(angleZ + MathHelper::PiOver2);
            }

            Acceleration.X = -Gravity * std::sin(MazePtr->Rotation.Z - angleX);
            Acceleration.Z = Gravity * std::sin(MazePtr->Rotation.X - angleZ);
            Acceleration.Y = 0;
        } else {
            Acceleration.Y = -Gravity;
        }

        if (intersectDetails_.IntersectWithWalls) {
            UpdateWallCollisionAcceleration(intersectDetails_.IntersectedWallTriangle);
        }
        if (intersectDetails_.IntersectWithFloorSides) {
            UpdateWallCollisionAcceleration(intersectDetails_.IntersectedFloorSidesTriangle);
        }
    }

    // Calculate friction between the marble and the maze.
    void CalculateFriction() override {
        if (intersectDetails_.IntersectWithGround) {
            if (Velocity.X > 0)
                Acceleration.X -= staticGroundFriction_ * Gravity * std::cos(MazePtr->Rotation.Z - angleX);
            else if (Velocity.X < 0)
                Acceleration.X += staticGroundFriction_ * Gravity * std::cos(MazePtr->Rotation.Z - angleX);

            if (Velocity.Z > 0)
                Acceleration.Z -= staticGroundFriction_ * Gravity * std::cos(MazePtr->Rotation.X - angleZ);
            else if (Velocity.Z < 0)
                Acceleration.Z += staticGroundFriction_ * Gravity * std::cos(MazePtr->Rotation.X - angleZ);
        }
    }

    // Calculate the marble's new velocity and position based on its current
    // acceleration.
    void CalculateVelocityAndPosition(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        Vector3 currentVelocity = Velocity;
        Velocity = currentVelocity + (Acceleration * elapsed);

        Velocity.X = MathHelper::Clamp(Velocity.X, -250.0f, 250.0f);
        Velocity.Z = MathHelper::Clamp(Velocity.Z, -250.0f, 250.0f);

        if (intersectDetails_.IntersectWithGround) {
            Velocity.Y = 0;
        }

        if (intersectDetails_.IntersectWithWalls) {
            UpdateWallCollisionVelocity(intersectDetails_.IntersectedWallTriangle, currentVelocity);
        }
        if (intersectDetails_.IntersectWithFloorSides) {
            UpdateWallCollisionVelocity(intersectDetails_.IntersectedFloorSidesTriangle, currentVelocity);
        }

        if (-1 < Velocity.X && Velocity.X < 1) Velocity.X = 0;
        if (-1 < Velocity.Z && Velocity.Z < 1) Velocity.Z = 0;

        UpdateMovement((Velocity + currentVelocity) / 2.0f, elapsed);
    }

private:
    // Play sounds when the marble is rolling, collides, or falls into a pit.
    void PlaySounds() {
        float volumeX = MathHelper::Clamp(std::abs(Velocity.X) / 400.0f, 0.0f, 1.0f);
        float volumeZ = MathHelper::Clamp(std::abs(Velocity.Z) / 400.0f, 0.0f, 1.0f);
        float volume = std::max(volumeX, volumeZ);
        float pitch = volume - 1.0f;

        if (intersectDetails_.IntersectWithGround && (Velocity.X != 0 || Velocity.Z != 0)) {
            auto* rolling = AudioManager::Get("rolling");
            if (rolling && rolling->getStateProperty() != SoundState::Playing) AudioManager::PlaySound("rolling", true);
            if (rolling) {
                rolling->setVolumeProperty(volume);
                rolling->setPitchProperty(pitch);
            }
        } else {
            AudioManager::StopSound("rolling");
        }

        if (Position.Y < -50) {
            AudioManager::PlaySound("pit");
        }

        if (intersectDetails_.IntersectWithWalls) {
            AudioManager::PlaySound("collision");
            auto* collision = AudioManager::Get("collision");
            if (collision) collision->setVolumeProperty(std::max(volumeX, volumeZ));
        }
    }

    // Returns the direction of the collision between the component and a triangle.
    static int CollideDirection(const Triangle& t) {
        if (t.A.Z == t.B.Z && t.B.Z == t.C.Z) return AxisZ;
        if (t.A.X == t.B.X && t.B.X == t.C.X) return AxisX;
        if (t.A.Y == t.B.Y && t.B.Y == t.C.Y) return AxisY;
        return AxisX | AxisZ;
    }

    void UpdateWallCollisionAcceleration(const std::vector<Triangle>& wallTriangles) {
        for (const auto& triangle : wallTriangles) {
            int direction = CollideDirection(triangle);
            if ((direction & AxisX) == AxisX) {
                if (Velocity.X > 0) Acceleration.X -= WallFriction;
                else if (Velocity.X < 0) Acceleration.X += WallFriction;
            }
            if ((direction & AxisZ) == AxisZ) {
                if (Velocity.Z > 0) Acceleration.Z -= WallFriction;
                else if (Velocity.Z < 0) Acceleration.Z += WallFriction;
            }
        }
    }

    void UpdateWallCollisionVelocity(const std::vector<Triangle>& wallTriangles, Vector3& currentVelocity) {
        for (const auto& triangle : wallTriangles) {
            int direction = CollideDirection(triangle);
            if ((direction & AxisX) == AxisX && (direction & AxisZ) == AxisZ) {
                float tmp = Velocity.X;
                Velocity.X = Velocity.Z;
                Velocity.Z = tmp;

                tmp = currentVelocity.X;
                currentVelocity.X = currentVelocity.Z * 0.3f;
                currentVelocity.Z = tmp * 0.3f;
            } else if ((direction & AxisX) == AxisX) {
                if ((Position.X > triangle.A.X && Velocity.X < 0) || (Position.X < triangle.A.X && Velocity.X > 0)) {
                    Velocity.X = -Velocity.X * 0.3f;
                    currentVelocity.X = -currentVelocity.X * 0.3f;
                }
            } else if ((direction & AxisZ) == AxisZ) {
                if ((Position.Z > triangle.A.Z && Velocity.Z < 0) || (Position.Z < triangle.A.Z && Velocity.Z > 0)) {
                    Velocity.Z = -Velocity.Z * 0.3f;
                    currentVelocity.Z = -currentVelocity.Z * 0.3f;
                }
            }
        }
    }

    // Update the marble's position based on momentary velocity.
    void UpdateMovement(Vector3 deltaVelocity, float deltaTime) {
        Vector3 deltaPosition = deltaVelocity * deltaTime;

        BoundingSphere nextPosition = BoundingSphereTransformed();
        nextPosition.Center = nextPosition.Center + deltaPosition;
        IntersectDetails nextIntersectDetails;
        MazePtr->GetCollisionDetails(nextPosition, nextIntersectDetails, true);
        nextPosition.Radius += 1.0f;

        Position += deltaPosition;

        Vector3 forwardVecX = Vector3::Transform(normal_, Matrix::CreateRotationZ(-MathHelper::PiOver2));
        Vector3 forwardVecZ = Vector3::Transform(normal_, Matrix::CreateRotationX(-MathHelper::PiOver2));

        bool isGroundStraight = true;
        if (forwardVecX.X != -1 && forwardVecX.X != 0) {
            Position.Y += deltaPosition.X / forwardVecX.X * forwardVecX.Y;
            isGroundStraight = false;
        }
        if (forwardVecZ.X != -1 && forwardVecZ.X != 0) {
            Position.Y += deltaPosition.Z / forwardVecZ.Z * forwardVecZ.Y;
            isGroundStraight = false;
        }
        if (isGroundStraight && nextIntersectDetails.IntersectWithGround) {
            Position.Y = nextIntersectDetails.IntersectedGroundTriangle->A.Y + BoundingSphereTransformed().Radius;
        }

        if (BoundingSphereTransformed().Radius != 0) {
            Rotation.Z = deltaPosition.Z / BoundingSphereTransformed().Radius;
            Rotation.X = deltaPosition.X / BoundingSphereTransformed().Radius;
        }
    }

    RawMesh mesh_;
    Texture2D marbleTexture_;
    std::unique_ptr<BasicEffect> effect_;
    BoundingSphere localBoundingSphere_;
    Matrix rollMatrix_ = Matrix::getIdentityProperty();
    Vector3 normal_ = Vector3::Zero;
};

} // namespace MarbleMazeSample

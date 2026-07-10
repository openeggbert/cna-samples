#pragma once

// DrawableComponent3D.hpp — C++ port of Objects/DrawableComponent3D.cs (XNA 4.0
// MarbleMaze sample). The base class provides the shared physics-update skeleton
// (CalculateCollisions -> CalculateAcceleration -> CalculateFriction ->
// CalculateVelocityAndPosition) and Position/Rotation/Velocity/Acceleration state.
//
// Deviation from the C# original (documented in missing.md): the original's Draw()
// iterates a generic Model's Meshes/Effects (Model = Game.Content.Load<Model>(...)).
// Every model this sample needs is instead loaded via RawMesh.hpp (NOXNA, see its
// own header comment) to route around DEFERRED.md item #26's ModelTypeReader vertex
// corruption bug -- and Marble's single mesh vs. Maze's 6 differently-textured
// submeshes need different Draw() shapes entirely (Marble: one texture, one
// BasicEffect; Maze: 6 textures sharing one BasicEffect with the texture swapped per
// part). So Draw() here is left pure virtual instead of providing a generic
// Model-based default -- Marble.hpp/Maze.hpp each implement their own, still calling
// EnableDefaultLighting()/setting Projection+View+World exactly as the C# original's
// shared Draw() body did.
//
// Neither Marble nor Maze (nor Camera) is ever added to Game.Components in the C#
// original -- GameplayScreen.cs owns and calls Update()/Draw() on them directly, so
// DEFERRED.md item #23 (Game::DoInitialize()'s ComponentAdded timing gap) simply
// does not apply here, the same as HeightmapCollision found.

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"

#include "Camera.hpp"
#include "../Misc/IntersectDetails.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::Vector3;

// axis flags, matching the C# [Flags] enum Axis.
enum Axis {
    AxisX = 0x1,
    AxisY = 0x2,
    AxisZ = 0x4,
};

class DrawableComponent3D : public DrawableGameComponent {
public:
    static constexpr float Gravity = 100.0f * 9.81f;
    static constexpr float WallFriction = 100.0f * 0.8f;

    Camera* CameraPtr = nullptr;

    Vector3 Position = Vector3::Zero;
    Vector3 Rotation = Vector3::Zero;
    Vector3 Velocity = Vector3::Zero;
    Vector3 Acceleration = Vector3::Zero;

    Matrix FinalWorldTransforms = Matrix::getIdentityProperty();
    Matrix OriginalWorldTransforms = Matrix::getIdentityProperty();

    explicit DrawableComponent3D(Game& game) : DrawableGameComponent(game) {}

    void Update(GameTime& gameTime) override {
        CalcPhysics(gameTime);
        UpdateFinalWorldTransform();

        GameComponent::Update(gameTime);
    }

protected:
    bool preferPerPixelLighting_ = false;
    float staticGroundFriction_ = 0.1f;
    IntersectDetails intersectDetails_;

    // Default final transformation update; Marble overrides this.
    virtual void UpdateFinalWorldTransform() {
        FinalWorldTransforms = Matrix::CreateFromYawPitchRoll(Rotation.Y, Rotation.X, Rotation.Z) *
                                OriginalWorldTransforms * Matrix::CreateTranslation(Position);
    }

    virtual void CalcPhysics(GameTime& gameTime) {
        CalculateCollisions();
        CalculateAcceleration();
        CalculateFriction();
        CalculateVelocityAndPosition(gameTime);
    }

    virtual void CalculateFriction() = 0;
    virtual void CalculateAcceleration() = 0;
    virtual void CalculateVelocityAndPosition(GameTime& gameTime) = 0;
    virtual void CalculateCollisions() = 0;
};

} // namespace MarbleMazeSample

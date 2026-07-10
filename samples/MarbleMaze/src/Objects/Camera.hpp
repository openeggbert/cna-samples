#pragma once

// Camera.hpp — C++ port of Objects/Camera.cs (XNA 4.0 MarbleMaze sample).

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameComponent.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameComponent;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::GraphicsDevice;

class Camera : public GameComponent {
public:
    Vector3 ObjectToFollow = Vector3::Zero;
    Matrix Projection = Matrix::getIdentityProperty();
    Matrix View = Matrix::getIdentityProperty();

    Camera(Game& game, GraphicsDevice& graphics) : GameComponent(game), graphicsDevice_(graphics) {}

    void Initialize() override {
        Projection = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::ToRadians(50.0f), graphicsDevice_.getViewportProperty().getAspectRatioProperty(), 1.0f, 10000.0f);

        GameComponent::Initialize();
    }

    void Update(GameTime& gameTime) override {
        position_ = ObjectToFollow + cameraPositionOffset_;
        target_ = ObjectToFollow + cameraTargetOffset_;

        View = Matrix::CreateLookAt(position_, target_, Vector3::Up);

        GameComponent::Update(gameTime);
    }

private:
    GraphicsDevice& graphicsDevice_;
    Vector3 position_ = Vector3::Zero;
    Vector3 target_ = Vector3::Zero;

    const Vector3 cameraPositionOffset_ = Vector3(0.0f, 450.0f, 100.0f);
    const Vector3 cameraTargetOffset_ = Vector3(0.0f, 0.0f, -50.0f);
};

} // namespace MarbleMazeSample

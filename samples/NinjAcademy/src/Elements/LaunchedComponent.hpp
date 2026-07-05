#pragma once

// LaunchedComponent.hpp — C++ port of Elements/General/LaunchedComponent.cs
// (XNA 4.0 NinjAcademy sample). A game component that can be launched at a
// specified velocity, affected by gravity, and may spin while moving.

#include <array>
#include <functional>
#include <optional>
#include <vector>

#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

#include "../Line.hpp"
#include "AnimatedComponent.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;

// Port of Elements/General/LaunchedComponent.cs.
class LaunchedComponent : public AnimatedComponent {
public:
    // Fired once per Launch() call, when the component drops past NotifyHeight
    // (only while its Y velocity is downward).
    std::function<void()> DroppedPastHeight;

    // The height, in pixels, that fires DroppedPastHeight when crossed. nullopt = never fires.
    std::optional<float> NotifyHeight;

    LaunchedComponent(Game& game, GameScreen* gameScreen, Texture2D texture)
        : AnimatedComponent(game, gameScreen, std::move(texture)) {}

    LaunchedComponent(Game& game, GameScreen* gameScreen, Animation animation)
        : AnimatedComponent(game, gameScreen, std::move(animation)) {}

    float AngularVelocity() const { return angularVelocity_; }
    Vector2 Velocity() const { return velocity_; }
    Vector2 Acceleration() const { return acceleration_; }
    float Rotation() const { return rotation_; }

    BoundingBox Bounds() const override {
        auto corners = GetCurrentBoundCornerPositions();
        return BoundingBox::CreateFromPoints(std::vector<Vector3>(corners.begin(), corners.end()));
    }
    Vector2 Center() const override { return Position; }

    void Update(GameTime& gameTime) override {
        AnimatedComponent::Update(gameTime);

        float elapsedSeconds = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        velocity_ = velocity_ + acceleration_ * elapsedSeconds;
        Position = Position + velocity_ * elapsedSeconds;

        if (!isEventFired_ && NotifyHeight.has_value() && Position.Y > NotifyHeight.value() && velocity_.Y > 0.0f) {
            if (DroppedPastHeight)
                DroppedPastHeight();
            isEventFired_ = true;
        }

        rotation_ += angularVelocity_ * elapsedSeconds;
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        spriteBatch_->Begin();
        animation_.Draw(*spriteBatch_, Position, rotation_, VisualCenter, 1.0f, SpriteEffects::None, 0.0f);
        spriteBatch_->End();
    }

    // Launches from initialPosition at initialVelocity, with the given acceleration and angular velocity.
    void Launch(Vector2 initialPosition, Vector2 initialVelocity, Vector2 acceleration, float angularVelocity) {
        Launch(initialPosition, initialVelocity, acceleration, 0.0f, angularVelocity);
    }

    void Launch(Vector2 initialPosition, Vector2 initialVelocity, Vector2 acceleration, float initialRotation,
                float angularVelocity) {
        Position = initialPosition;
        velocity_ = initialVelocity;
        rotation_ = initialRotation;
        acceleration_ = acceleration;
        angularVelocity_ = angularVelocity;
        isEventFired_ = false;
    }

    // Returns the component's edges, ordered: top, right, bottom, left.
    std::array<Line, 4> GetEdges() const {
        std::array<Vector2, 4> corners = GetCurrentBoundCornerPositions2D();

        std::array<Line, 4> result;
        result[0] = Line(corners[3], corners[2]);
        result[1] = Line(corners[2], corners[1]);
        result[2] = Line(corners[1], corners[0]);
        result[3] = Line(corners[0], corners[3]);
        return result;
    }

protected:
    // Returns the correct corner positions of the bounding box in light of
    // rotation. Order matches BoundingBox::GetCorners()'s first four corners.
    std::array<Vector3, 4> GetCurrentBoundCornerPositions() const {
        BoundingBox baseBounds = AnimatedComponent::Bounds();
        float w = baseBounds.Max.X - baseBounds.Min.X;
        float h = baseBounds.Max.Y - baseBounds.Min.Y;

        std::array<Vector3, 4> unrotatedCornersAroundCenter = {
            Vector3(-w / 2.0f, h / 2.0f, 0.0f),
            Vector3(w / 2.0f, h / 2.0f, 0.0f),
            Vector3(w / 2.0f, -h / 2.0f, 0.0f),
            Vector3(-w / 2.0f, -h / 2.0f, 0.0f),
        };

        Matrix rotationMatrix = Matrix::CreateRotationZ(rotation_);
        Matrix translationMatrix = Matrix::CreateTranslation(Vector3(Position, 0.0f));

        std::array<Vector3, 4> result;
        for (int i = 0; i < 4; i++) {
            Vector3 rotated;
            Vector3::Transform(unrotatedCornersAroundCenter[i], rotationMatrix, rotated);
            Vector3::Transform(rotated, translationMatrix, result[i]);
        }
        return result;
    }

    std::array<Vector2, 4> GetCurrentBoundCornerPositions2D() const {
        std::array<Vector3, 4> result3D = GetCurrentBoundCornerPositions();
        std::array<Vector2, 4> result;
        for (int i = 0; i < 4; i++)
            result[i] = Vector2(result3D[i].X, result3D[i].Y);
        return result;
    }

private:
    float angularVelocity_ = 0.0f;
    Vector2 velocity_;
    Vector2 acceleration_;
    float rotation_ = 0.0f;
    bool isEventFired_ = true;
};

} // namespace NinjAcademy

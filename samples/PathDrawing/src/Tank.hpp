#pragma once
#include <cmath>
#include <optional>
#include <string>
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "WaypointList.hpp"

namespace PathDrawing {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class Tank {
    static constexpr float atDestinationLimit    = 5.0f;
    static constexpr float maxAngularVelocity    = MathHelper::Pi;
    static constexpr float maxMoveSpeed          = 100.0f;
    static constexpr float maxMoveSpeedDelta     = maxMoveSpeed / 2.0f;

    std::optional<Texture2D> tankTexture_;
    Vector2 tankTextureCenter_;

    float rotation_              = 0.0f;
    bool  recomputeTargetRotation_ = true;
    float targetRotation_        = 0.0f;
    float previousRotation_      = 0.0f;
    float rotationInterpolation_ = 0.0f;

    Vector2 direction_;
    float   moveSpeed_ = 0.0f;
    Vector2 location_;

    WaypointList waypoints_;

public:
    Tank(GraphicsDevice& /*graphicsDevice*/, Content::ContentManager& content) {
        tankTexture_.emplace(content.Load<Texture2D>("tank"));
        tankTextureCenter_ = Vector2(
            (float)(tankTexture_->getWidthProperty()  / 2),
            (float)(tankTexture_->getHeightProperty() / 2));
    }

    WaypointList& Waypoints() { return waypoints_; }

    Vector2 Location() const { return location_; }

    float MoveSpeed() const       { return moveSpeed_; }
    void  SetMoveSpeed(float v)   { moveSpeed_ = v; }

    float DistanceToDestination() const {
        return Vector2::Distance(location_, waypoints_.Peek());
    }
    bool AtDestination() const {
        return DistanceToDestination() < atDestinationLimit;
    }

    void Reset(Vector2 newLocation) {
        location_ = newLocation;
        waypoints_.Clear();
    }

    bool HitTest(Vector2 point) const {
        return Vector2::DistanceSquared(point, location_) <
               tankTextureCenter_.LengthSquared() * 1.5f;
    }

    void Update(const GameTime& gameTime) {
        float elapsedTime = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        if (waypoints_.Count() > 0) {
            if (AtDestination()) {
                waypoints_.Dequeue();
                recomputeTargetRotation_ = true;
            } else {
                direction_ = Vector2(waypoints_.Peek().X - location_.X,
                                     waypoints_.Peek().Y - location_.Y);
                direction_.Normalize();

                if (recomputeTargetRotation_) {
                    previousRotation_      = rotation_;
                    targetRotation_        = std::atan2(direction_.Y, direction_.X);
                    rotationInterpolation_ = 0.0f;

                    if (targetRotation_ - previousRotation_ > MathHelper::Pi)
                        targetRotation_ -= MathHelper::TwoPi;
                    else if (targetRotation_ - previousRotation_ < -MathHelper::Pi)
                        targetRotation_ += MathHelper::TwoPi;

                    recomputeTargetRotation_ = false;
                }

                rotationInterpolation_ = MathHelper::Clamp(
                    rotationInterpolation_ + elapsedTime * 10.0f, 0.0f, 1.0f);

                rotation_ = previousRotation_ +
                            (targetRotation_ - previousRotation_) * rotationInterpolation_;

                location_.X += direction_.X * moveSpeed_ * elapsedTime;
                location_.Y += direction_.Y * moveSpeed_ * elapsedTime;
            }
        }
    }

    void Draw(SpriteBatch& spriteBatch) {
        spriteBatch.Draw(*tankTexture_, location_,
                         std::nullopt, Color(255, 255, 255, 255),
                         rotation_, tankTextureCenter_,
                         1.0f, SpriteEffects::None, 0.0f);
    }
};

} // namespace PathDrawing

#pragma once
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "WaypointList.hpp"

namespace WaypointSample {

enum class BehaviorType { Linear, Steering };

class Behavior;

class Tank {
public:
    static constexpr float MaxMoveSpeed      = 100.0f;
    static constexpr float MaxAngularVelocity= MathHelper::Pi;
    static constexpr float MaxMoveSpeedDelta = MaxMoveSpeed / 2.0f;

private:
    static constexpr float atDestinationLimit = 5.0f;

    std::optional<Texture2D> tankTexture_;
    Vector2 tankTextureCenter_;

    std::unique_ptr<Behavior> currentBehavior_;
    BehaviorType behaviorType_ = BehaviorType::Linear;

    Vector2 direction_;
    float   moveSpeed_ = MaxMoveSpeed;
    Vector2 location_;
    WaypointList waypoints_;

public:
    BehaviorType GetBehaviorType() const { return behaviorType_; }
    void SetBehaviorType(BehaviorType t);   // defined after Behavior headers

    Vector2& Direction()               { return direction_; }
    void SetDirection(const Vector2& d){ direction_ = d; }
    float MoveSpeed() const            { return moveSpeed_; }
    void  SetMoveSpeed(float v)        { moveSpeed_ = v; }
    Vector2 Location() const           { return location_; }
    WaypointList& Waypoints()          { return waypoints_; }

    float DistanceToDestination() const {
        return Vector2::Distance(location_, waypoints_.Peek());
    }
    bool AtDestination() const {
        return DistanceToDestination() < atDestinationLimit;
    }

    void LoadContent(Content::ContentManager& content) {
        tankTexture_.emplace(content.Load<Texture2D>("tank"));
        tankTextureCenter_ = Vector2(
            (float)(tankTexture_->getWidthProperty()  / 2),
            (float)(tankTexture_->getHeightProperty() / 2));
        waypoints_.LoadContent(content);
        SetBehaviorType(BehaviorType::Linear);
    }

    void Reset(Vector2 newLocation) {
        location_ = newLocation;
        waypoints_.Clear();
    }

    void Update(const GameTime& gameTime);  // defined in TankBehaviorImpl.hpp

    // Draw assumes spriteBatch is already begun
    void Draw(SpriteBatch& spriteBatch) {
        waypoints_.Draw(spriteBatch);
        float facing = std::atan2(direction_.Y, direction_.X);
        spriteBatch.Draw(*tankTexture_, location_, std::nullopt,
            Color(255, 255, 255, 255), facing,
            tankTextureCenter_, 1.0f, SpriteEffects::None, 0.0f);
    }

    void CycleBehaviorType() {
        SetBehaviorType(behaviorType_ == BehaviorType::Linear
            ? BehaviorType::Steering : BehaviorType::Linear);
    }
};

} // namespace WaypointSample

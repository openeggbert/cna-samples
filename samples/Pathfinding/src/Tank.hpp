#pragma once
#include <cmath>
#include <optional>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Map.hpp"
#include "WaypointList.hpp"

namespace Pathfinding {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class Tank {
    static constexpr float atDestinationLimit = 5.0f;
    static constexpr float moveSpeed_         = 100.0f;

    std::optional<Texture2D> tankTexture_;
    Vector2 tankTextureCenter_;
    Map*    map_ = nullptr;

    float   scale_    = 1.0f;
    Vector2 direction_;
    bool    moving_   = false;
    Vector2 destination_;
    Vector2 location_;

    WaypointList waypoints_;

public:
    WaypointList& Waypoints()  { return waypoints_; }
    Vector2 Location()   const { return location_; }
    bool    Moving()     const { return moving_; }
    void    SetMoving(bool v)  { moving_ = v; }

    void SetScale(float v) {
        scale_ = v;
        waypoints_.SetScale(v);
    }

    float DistanceToDestination() const {
        return Vector2::Distance(location_, destination_);
    }
    bool AtDestination() const {
        return DistanceToDestination() < atDestinationLimit;
    }

    void Initialize(Map& m) {
        map_ = &m;
        location_ = Vector2::Zero;
        destination_ = location_;
    }

    void LoadContent(Content::ContentManager& content) {
        tankTexture_.emplace(content.Load<Texture2D>("tank"));
        tankTextureCenter_ = Vector2(
            (float)(tankTexture_->getWidthProperty()  / 2),
            (float)(tankTexture_->getHeightProperty() / 2));
        waypoints_.LoadContent(content);
    }

    // Draw assumes spriteBatch is already begun
    void Draw(SpriteBatch& spriteBatch) {
        waypoints_.Draw(spriteBatch);

        float facingDirection = std::atan2(direction_.Y, direction_.X);
        spriteBatch.Draw(*tankTexture_, location_, std::nullopt,
            Color(255, 255, 255, 255), facingDirection,
            tankTextureCenter_, scale_, SpriteEffects::None, 0.0f);
    }

    void Update(const GameTime& gameTime) {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        if (moving_) {
            if (waypoints_.Count() >= 1)
                destination_ = waypoints_.Peek();

            if (AtDestination() && waypoints_.Count() >= 1)
                waypoints_.Dequeue();

            if (!AtDestination()) {
                direction_ = Vector2(destination_.X - location_.X,
                                     destination_.Y - location_.Y);
                direction_.Normalize();
                location_.X += direction_.X * moveSpeed_ * elapsed;
                location_.Y += direction_.Y * moveSpeed_ * elapsed;
            }
        }
    }

    void Reset() {
        waypoints_.Clear();
        direction_ = Vector2::Zero;
        moving_    = false;
        SetScale(map_->Scale());
        location_    = map_->MapToWorld(map_->StartTile(), true);
        destination_ = location_;
    }
};

} // namespace Pathfinding

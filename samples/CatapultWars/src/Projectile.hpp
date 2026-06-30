#pragma once

#include <cmath>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

namespace CatapultWars {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;

// A boulder fired by a catapult. Its position over time is computed by simple
// projectile-motion formulas, taking the wind as a horizontal force.
class Projectile {
public:
    Projectile(Game& game, SpriteBatch& screenSpriteBatch, const std::string& textureName,
               Vector2 startPosition, float groundHitOffset, bool isAi, float gravity)
        : game_(&game), spriteBatch_(&screenSpriteBatch), textureName_(textureName),
          isAI_(isAi), hitOffset_(groundHitOffset), gravity_(gravity),
          projectileStartPosition_(startPosition) {}

    void Initialize() {
        projectileTexture_.emplace(game_->getContentProperty().Load<Texture2D>(textureName_));
    }

    Vector2 getProjectileStartPosition() const { return projectileStartPosition_; }
    void setProjectileStartPosition(Vector2 v) { projectileStartPosition_ = v; }

    Vector2 getProjectilePosition() const { return projectilePosition_; }
    void setProjectilePosition(Vector2 v) { projectilePosition_ = v; }

    Vector2 getProjectileHitPosition() const { return projectileHitPosition_; }

    const Texture2D& getProjectileTexture() const { return *projectileTexture_; }

    void Draw(const GameTime& /*gameTime*/) {
        spriteBatch_->Draw(*projectileTexture_, projectilePosition_, std::nullopt,
                          Color::White, projectileRotation_,
                          Vector2((float)(projectileTexture_->getWidthProperty() / 2),
                                  (float)(projectileTexture_->getHeightProperty() / 2)),
                          1.0f, SpriteEffects::None, 0.0f);
    }

    // Calculates the projectile position and velocity based on elapsed time.
    void UpdateProjectileFlightData(const GameTime& gameTime, float wind, float gravity,
                                    bool& groundHit) {
        flightTime_ += (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        int direction = isAI_ ? -1 : 1;

        float previousXPosition = projectilePosition_.X;
        float previousYPosition = projectilePosition_.Y;

        projectilePosition_.X = projectileStartPosition_.X +
            (direction * projectileVelocity_.X * flightTime_) +
            0.5f * (8 * wind * (flightTime_ * flightTime_));

        projectilePosition_.Y = projectileStartPosition_.Y -
            (projectileVelocity_.Y * flightTime_) +
            0.5f * (gravity * (flightTime_ * flightTime_));

        projectileRotation_ += MathHelper::ToRadians(projectileVelocity_.X * 0.5f);

        if (projectilePosition_.Y >= 332 + hitOffset_) {
            projectilePosition_.X = previousXPosition;
            projectilePosition_.Y = previousYPosition;
            projectileHitPosition_ = Vector2(previousXPosition, 332);
            groundHit = true;
        } else {
            groundHit = false;
        }
    }

    void Fire(float velocityX, float velocityY) {
        projectileVelocity_.X = velocityX;
        projectileVelocity_.Y = velocityY;
        projectileInitialVelocityY_ = projectileVelocity_.Y;
        flightTime_ = 0;
    }

private:
    Game* game_;
    SpriteBatch* spriteBatch_;
    std::string textureName_;

    Vector2 projectileVelocity_ = Vector2::Zero;
    float projectileInitialVelocityY_ = 0.0f;
    float projectileRotation_ = 0.0f;
    float flightTime_ = 0.0f;
    bool isAI_;
    float hitOffset_;
    float gravity_;

    Vector2 projectileStartPosition_ = Vector2::Zero;
    Vector2 projectilePosition_ = Vector2::Zero;
    Vector2 projectileHitPosition_ = Vector2::Zero;

    std::optional<Texture2D> projectileTexture_;
};

} // namespace CatapultWars

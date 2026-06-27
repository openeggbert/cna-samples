#pragma once
#include <cmath>
#include <map>
#include <memory>
#include <vector>
#include "Animal.hpp"
#include "../AIParameters.hpp"
#include "../Behaviors/AlignBehavior.hpp"
#include "../Behaviors/CohesionBehavior.hpp"
#include "../Behaviors/FleeBehavior.hpp"
#include "../Behaviors/SeparationBehavior.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "System/Random.hpp"

namespace Flocking {

class Bird : public Animal {
    System::Random random_;
    Vector2 aiNewDir_;
    int     aiNumSeen_ = 0;

    std::map<AnimalType, std::vector<std::unique_ptr<Behavior>>> behaviors_;

public:
    Bird(Texture2D tex, Vector2 dir, Vector2 loc, int screenWidth, int screenHeight)
        : Animal(std::move(tex), screenWidth, screenHeight)
        , random_((int)loc.X + (int)loc.Y)
    {
        direction_ = dir;
        direction_.Normalize();
        location_  = loc;
        moveSpeed_ = 125.0f;
        fleeing_   = false;
        animalType_ = AnimalType::Bird;
        BuildBehaviors();
    }

    void ResetThink() {
        fleeing_          = false;
        aiNewDir_         = Vector2::Zero;
        aiNumSeen_        = 0;
        reactionDistance_ = 0.0f;
        reactionLocation_ = Vector2::Zero;
    }

    void ReactTo(Animal* other, const AIParameters& params) {
        if (!other) return;

        Vector2 otherLoc = other->Location();
        ClosestLocation(location_, otherLoc, reactionLocation_);
        reactionDistance_ = Vector2::Distance(location_, reactionLocation_);

        if (reactionDistance_ < params.DetectionDistance) {
            auto it = behaviors_.find(other->GetAnimalType());
            if (it != behaviors_.end()) {
                for (auto& b : it->second) {
                    b->Update(*other, params);
                    if (b->Reacted()) {
                        aiNewDir_ = aiNewDir_ + b->Reaction();
                        aiNumSeen_++;
                    }
                }
            }
        }
    }

    void Update(const GameTime& gameTime, const AIParameters& params) {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        Vector2 randomDir((float)random_.NextDouble() - 0.5f,
                          (float)random_.NextDouble() - 0.5f);
        randomDir.Normalize();

        if (aiNumSeen_ > 0) {
            aiNewDir_ = (direction_ * params.MoveInOldDirectionInfluence)
                      + (aiNewDir_ * (params.MoveInFlockDirectionInfluence
                                      / (float)aiNumSeen_));
        } else {
            aiNewDir_ = direction_ * params.MoveInOldDirectionInfluence;
        }

        aiNewDir_ = aiNewDir_ + randomDir * params.MoveInRandomDirectionInfluence;
        aiNewDir_.Normalize();
        aiNewDir_  = ChangeDirection(direction_, aiNewDir_,
                                     params.MaxTurnRadians * elapsed);
        direction_ = aiNewDir_;

        if (direction_.LengthSquared() > 0.01f) {
            location_.X += direction_.X * moveSpeed_ * elapsed;
            location_.Y += direction_.Y * moveSpeed_ * elapsed;

            if      (location_.X < 0.0f)          location_.X = (float)boundaryWidth_  + location_.X;
            else if (location_.X > boundaryWidth_) location_.X = location_.X - (float)boundaryWidth_;

            if      (location_.Y < 0.0f)           location_.Y = (float)boundaryHeight_ + location_.Y;
            else if (location_.Y > boundaryHeight_) location_.Y = location_.Y - (float)boundaryHeight_;
        }
    }

    void Draw(SpriteBatch& spriteBatch, const GameTime& gameTime) override {
        Color tintColor = color_;
        if (fleeing_) {
            float t = (float)std::sin(10.0 * gameTime.getTotalGameTimeProperty().getTotalSecondsProperty());
            t = 0.5f + 0.5f * t;
            tintColor = Color(Vector4::Lerp(
                Color(255, 0, 0, 255).ToVector4(),
                Color(255, 255, 255, 255).ToVector4(), t));
        }
        float rotation = std::atan2(direction_.Y, direction_.X);
        spriteBatch.Draw(*texture_, location_, std::nullopt,
            tintColor, rotation, textureCenter_, 1.0f, SpriteEffects::None, 0.0f);
    }

private:
    void BuildBehaviors() {
        std::vector<std::unique_ptr<Behavior>> catReactions;
        catReactions.push_back(std::make_unique<FleeBehavior>(*this));
        behaviors_[AnimalType::Cat] = std::move(catReactions);

        std::vector<std::unique_ptr<Behavior>> birdReactions;
        birdReactions.push_back(std::make_unique<AlignBehavior>(*this));
        birdReactions.push_back(std::make_unique<CohesionBehavior>(*this));
        birdReactions.push_back(std::make_unique<SeparationBehavior>(*this));
        behaviors_[AnimalType::Bird] = std::move(birdReactions);
    }

    void ClosestLocation(const Vector2& src, const Vector2& dest, Vector2& out) const {
        float x  = dest.X, y = dest.Y;
        float dX = std::abs(dest.X - src.X);
        float dY = std::abs(dest.Y - src.Y);

        if (std::abs((float)boundaryWidth_  - dest.X + src.X) < dX) { dX = (float)boundaryWidth_  - dest.X + src.X; x = dest.X - (float)boundaryWidth_; }
        if (std::abs((float)boundaryWidth_  - src.X  + dest.X) < dX) { dX = (float)boundaryWidth_  - src.X  + dest.X; x = dest.X + (float)boundaryWidth_; }
        if (std::abs((float)boundaryHeight_ - dest.Y + src.Y) < dY) { dY = (float)boundaryHeight_ - dest.Y + src.Y; y = dest.Y - (float)boundaryHeight_; }
        if (std::abs((float)boundaryHeight_ - src.Y  + dest.Y) < dY) {                                               y = dest.Y + (float)boundaryHeight_; }

        out = Vector2(x, y);
    }

    static Vector2 ChangeDirection(const Vector2& oldDir, const Vector2& newDir,
                                   float maxTurnRadians) {
        float oldAngle     = std::atan2(oldDir.Y, oldDir.X);
        float desiredAngle = std::atan2(newDir.Y, newDir.X);
        float newAngle     = MathHelper::Clamp(desiredAngle,
                                WrapAngle(oldAngle - maxTurnRadians),
                                WrapAngle(oldAngle + maxTurnRadians));
        return Vector2(std::cos(newAngle), std::sin(newAngle));
    }

    static float WrapAngle(float r) {
        while (r < -MathHelper::Pi) r += MathHelper::TwoPi;
        while (r >  MathHelper::Pi) r -= MathHelper::TwoPi;
        return r;
    }
};

} // namespace Flocking

#pragma once

// Bee.hpp — C++ port of Objects/Bee.cs (XNA 4.0 HoneycombRush sample). Base
// class for WorkerBee/SoldierBee. Update()/Draw() are declared here but
// defined out-of-line in GameplayScreen.hpp — see missing.md.

#include <string>

#include "System/Random.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "System/TimeSpan.hpp"

#include "Beehive.hpp"
#include "TexturedDrawableGameComponent.hpp"

namespace HoneycombRush {

class SmokePuff; // forward declaration

// Represents the base bee component. Port of Objects/Bee.cs.
class Bee : public TexturedDrawableGameComponent {
public:
    Bee(Game& game, GameplayScreen* gamePlayScreen, Beehive* beehive)
        : TexturedDrawableGameComponent(game, gamePlayScreen), relatedBeehive_(beehive) {
        setDrawOrderProperty(INT32_MAX - 20);
    }

    bool IsBeeHit() const { return isHitBySmoke_; }
    Beehive* GetBeehive() const { return relatedBeehive_; }

    Rectangle Bounds() const override {
        if (!texture.HasBackend())
            return Rectangle();
        // The bee's texture is an animation strip; divide width by three to
        // get the bee's actual width.
        return Rectangle((int)position.X, (int)position.Y, texture.getWidthProperty() / 3,
                          texture.getHeightProperty());
    }

    void Initialize() override {
        SetStartupPosition();
        if (!animationKey_.empty()) {
            (*AnimationDefinitions)[animationKey_].PlayFromFrameIndex(0);
        }
        DrawableGameComponent::Initialize();
    }

    void Update(GameTime& gameTime) override;
    void Draw(const GameTime& gameTime) override;

    void HitBySmoke(SmokePuff& smokePuff);

    virtual void SetStartupPosition() {
        if (relatedBeehive_->AllowBeesToGenerate) {
            Rectangle rect = relatedBeehive_->Bounds();
            position = Vector2((float)(rect.X + rect.Width / 2), (float)(rect.Y + rect.Height / 2));
            velocity_ = Vector2((float)random_.Next(-MaxVelocity() * 100, MaxVelocity() * 100) / 100.0f,
                                 (float)random_.Next(-MaxVelocity() * 100, MaxVelocity() * 100) / 100.0f);
            isHitBySmoke_ = false;
            timeToRegenerate_ = System::TimeSpan::Zero;
            timeHit_ = System::TimeSpan::Zero;
        }
    }

    void Collide(Rectangle bounds) {
        if (!isGotHit_) {
            velocityChangeCounter_ = -10;
            if (position.X < (float)bounds.X || position.X > (float)(bounds.X + bounds.Width)) {
                velocity_.X *= -1.0f;
            } else {
                velocity_.Y *= -1.0f;
            }
            isGotHit_ = true;
        }
    }

protected:
    virtual int MaxVelocity() const = 0;
    virtual float AccelerationFactor() const = 0;

    static inline System::Random random_;

    Beehive* relatedBeehive_;
    Vector2 velocity_;

    float rotation_ = 0.0f;
    bool isHitBySmoke_ = false;
    bool isGotHit_ = false;

    std::string animationKey_;

    int velocityChangeCounter_ = 0;
    System::TimeSpan timeToRegenerate_;
    System::TimeSpan timeHit_;

    virtual int VelocityChangeInterval() const { return 15; }

private:
    void SetStartupPositionWithTimer() {
        timeToRegenerate_ = System::TimeSpan::FromMilliseconds(random_.Next(3000, 5000));
    }

    bool HandleRegeneration(const GameTime& gameTime) {
        if (timeToRegenerate_ != System::TimeSpan::Zero) {
            if (timeHit_ == System::TimeSpan::Zero) {
                timeHit_ = gameTime.getTotalGameTimeProperty();
            }

            if (timeToRegenerate_ + timeHit_ < gameTime.getTotalGameTimeProperty()) {
                SetStartupPosition();
            } else {
                position = Vector2((float)-texture.getWidthProperty(), (float)-texture.getHeightProperty());
                return false;
            }
        }
        return true;
    }

    void SetRandomMovement() {
        velocityChangeCounter_++;
        if (velocityChangeCounter_ == VelocityChangeInterval()) {
            velocity_ = Vector2((float)random_.Next(-MaxVelocity() * 100, MaxVelocity() * 100) / 100.0f,
                                 (float)random_.Next(-MaxVelocity() * 100, MaxVelocity() * 100) / 100.0f);
            velocityChangeCounter_ = 0;

            if (isGotHit_) {
                isGotHit_ = false;
            }
        }
    }
};

} // namespace HoneycombRush

#pragma once

// BeeKeeper.hpp — C++ port of Objects/BeeKeeper.cs (XNA 4.0 HoneycombRush
// sample). Represents the player's avatar. Initialize()/Update()/Draw() are
// declared here but defined out-of-line in GameplayScreen.hpp, since they
// need GameplayScreen::IsActive()/IsStarted() (an incomplete type here) —
// see missing.md.

#include <cmath>
#include <deque>
#include <functional>
#include <memory>
#include <stack>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "System/TimeSpan.hpp"

#include "../Misc/AudioManager.hpp"
#include "../Misc/ExtensionMethods.hpp"
#include "../Misc/VirtualThumbsticks.hpp"
#include "SmokePuff.hpp"
#include "TexturedDrawableGameComponent.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Graphics::SpriteEffects;

// Represents the beekeeper, the player's avatar. Port of Objects/BeeKeeper.cs.
class BeeKeeper : public TexturedDrawableGameComponent {
public:
    static constexpr const char* LegAnimationKey = "LegAnimation";
    static constexpr const char* BodyAnimationKey = "BodyAnimation";
    static constexpr const char* SmokeAnimationKey = "SmokeAnimation";
    static constexpr const char* ShootingAnimationKey = "ShootingAnimation";
    static constexpr const char* BeekeeperCollectingHoneyAnimationKey = "BeekeeperCollectingHoney";
    static constexpr const char* BeekeeperDespositingHoneyAnimationKey = "BeekeeperDespositingHoney";

    static constexpr int MaxSmokePuffs = 20;

    BeeKeeper(Game& game, GameplayScreen* gamePlayScreen) : TexturedDrawableGameComponent(game, gamePlayScreen) {}

    void Initialize() override;

    void LoadContent() override {
        smokeAnimationTexture_ = getGameProperty().getContentProperty().Load<Texture2D>("Textures/SmokeAnimationStrip");
        smokePuffTexture_ = getGameProperty().getContentProperty().Load<Texture2D>("Textures/smokePuff");
        hitTexture_ = getGameProperty().getContentProperty().Load<Texture2D>("Textures/hit");
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        position = Vector2((float)(vp.getWidthProperty() / 2 - (int)bodySize_.X / 2),
                            (float)(vp.getHeightProperty() / 2 - (int)bodySize_.Y / 2));

        for (int i = 0; i < MaxSmokePuffs; i++) {
            availableSmokePuffs_.push(std::make_shared<SmokePuff>(getGameProperty(), gamePlayScreen, smokePuffTexture_));
        }

        TexturedDrawableGameComponent::LoadContent();
    }

    void Update(GameTime& gameTime) override;
    void Draw(const GameTime& gameTime) override;

    bool IsStung() const { return isStung_; }
    bool IsFlashing() const { return isFlashing_; }

    void setIsShootingSmoke(bool value) {
        if (!isStung_) {
            needToShootSmoke_ = value;
            if (value) {
                AudioManager::PlaySound("SmokeGun_Loop");
            } else {
                shootSmokePuffTimer_ = System::TimeSpan::Zero;
            }
        }
    }

    Rectangle Bounds() const override {
        int height = (int)bodySize_.Y / 10 * 8;
        int width = (int)bodySize_.X / 10 * 5;
        int offsetY = ((int)bodySize_.Y - height) / 2;
        int offsetX = ((int)bodySize_.X - width) / 2;
        return Rectangle((int)position.X + offsetX, (int)position.Y + offsetY, width, height);
    }

    Rectangle CentralCollisionArea() const override {
        Rectangle bounds = Bounds();
        int height = bounds.Height / 10 * 5;
        int width = bounds.Width / 10 * 8;
        int offsetY = (bounds.Height - height) / 2;
        int offsetX = (bounds.Width - width) / 2;
        return Rectangle(bounds.X + offsetX, bounds.Y + offsetY, width, height);
    }

    bool IsDepositingHoney() const { return isDepositingHoney_; }

    bool IsCollectingHoney = false;

    Vector2 Position() const { return position; }

    Rectangle ThumbStickArea;

    bool IsInMotion = false;

    // Checks if a given rectangle intersects with a fired smoke puff.
    SmokePuff* CheckSmokeCollision(Rectangle checkRectangle) {
        for (auto& smokePuff : firedSmokePuffs_) {
            if (HasCollision(checkRectangle, smokePuff->CentralCollisionArea())) {
                return smokePuff.get();
            }
        }
        return nullptr;
    }

    void Stung(System::TimeSpan occurTime) {
        if (!isStung_ && !isFlashing_) {
            isStung_ = true;
            isFlashing_ = true;
            stungTime_ = occurTime;
            needToShootSmoke_ = false;
        }
    }

    void SetMovement(Vector2 movement) {
        if (!IsStung()) {
            velocity_ = movement;
            position = position + velocity_;
        }
    }

    void SetDirection(Vector2 movementDirection) { currentEffect_ = GetSpriteEffect(movementDirection); }

    void StartTransferHoney(int honeyDepositFrameCount, std::function<void()> callback) {
        depositHoneyCallback_ = std::move(callback);
        this->honeyDepositFrameCount_ = honeyDepositFrameCount;
        isDepositingHoney_ = true;
        depositHoneyTimerCounter_ = 0;

        AudioManager::PlaySound("DepositingIntoVat_Loop");
    }

    void EndTransferHoney() { isDepositingHoney_ = false; }

    std::deque<std::shared_ptr<SmokePuff>>& FiredSmokePuffs() { return firedSmokePuffs_; }

private:
    enum class WalkingDirection { Down = 0, Up = 8, Left = 16, Right = 24, LeftDown = 32, RightDown = 40, LeftUp = 48, RightUp = 56 };

    void ShootSmoke() {
        std::shared_ptr<SmokePuff> availableSmokePuff;

        if (!availableSmokePuffs_.empty()) {
            availableSmokePuff = availableSmokePuffs_.top();
            availableSmokePuffs_.pop();
        } else {
            availableSmokePuff = firedSmokePuffs_.front();
            firedSmokePuffs_.pop_front();
        }

        Rectangle bounds = Bounds();
        Vector2 beeKeeperCenter((float)(bounds.X + bounds.Width / 2), (float)(bounds.Y + bounds.Height / 2));

        availableSmokePuff->Fire(beeKeeperCenter, GetSmokeVelocityVector());
        firedSmokePuffs_.push_back(availableSmokePuff);
    }

    Vector2 GetSmokeVelocityVector() const {
        Vector2 initialVector;
        switch (direction_) {
            case WalkingDirection::Down: initialVector = Vector2(0, 1); break;
            case WalkingDirection::Up: initialVector = Vector2(0, -1); break;
            case WalkingDirection::Left: initialVector = Vector2(-1, 0); break;
            case WalkingDirection::Right: initialVector = Vector2(1, 0); break;
            case WalkingDirection::LeftDown: initialVector = Vector2(-1, 1); break;
            case WalkingDirection::RightDown: initialVector = Vector2(1, 1); break;
            case WalkingDirection::LeftUp: initialVector = Vector2(-1, -1); break;
            case WalkingDirection::RightUp: initialVector = Vector2(1, -1); break;
        }
        return initialVector * 2.0f + velocity_ * 1.0f;
    }

    SpriteEffects GetSpriteEffect(Vector2 movementDirection) {
        if (VirtualThumbsticks::getLeftThumbstickCenter().has_value() &&
            ThumbStickArea.Contains((int)VirtualThumbsticks::getLeftThumbstickCenter()->X,
                                     (int)VirtualThumbsticks::getLeftThumbstickCenter()->Y)) {
            if (movementDirection.X < 0) {
                lastEffect_ = SpriteEffects::FlipHorizontally;
            } else if (movementDirection.X > 0) {
                lastEffect_ = SpriteEffects::None;
            }
        }
        return lastEffect_;
    }

    void DetermineDirection(WalkingDirection& tempDirection, Vector2& adjustment) {
        if (!VirtualThumbsticks::getLeftThumbstickCenter().has_value()) {
            return;
        }

        Rectangle touchPointRectangle((int)VirtualThumbsticks::getLeftThumbstickCenter()->X,
                                       (int)VirtualThumbsticks::getLeftThumbstickCenter()->Y, 1, 1);

        if (ThumbStickArea.Intersects(touchPointRectangle)) {
            Vector2 stick = VirtualThumbsticks::getLeftThumbstick();
            if (std::abs(stick.X) > std::abs(stick.Y)) {
                DetermineDirectionDominantX(tempDirection, adjustment);
            } else {
                DetermineDirectionDominantY(tempDirection, adjustment);
            }
        }
    }

    void DetermineDirectionDominantX(WalkingDirection& tempDirection, Vector2& adjustment) {
        Vector2 stick = VirtualThumbsticks::getLeftThumbstick();
        if (stick.X > 0) {
            if (stick.Y > 0.25f) {
                tempDirection = WalkingDirection::RightDown;
                adjustment = Vector2(85, 30);
            } else if (stick.Y < -0.25f) {
                tempDirection = WalkingDirection::RightUp;
                adjustment = Vector2(85, 0);
            } else {
                tempDirection = WalkingDirection::Right;
                adjustment = Vector2(85, 15);
            }
        } else {
            if (stick.Y > 0.25f) {
                tempDirection = WalkingDirection::LeftDown;
                adjustment = Vector2(-85, 30);
            } else if (stick.Y < -0.25f) {
                tempDirection = WalkingDirection::LeftUp;
                adjustment = Vector2(-85, 0);
            } else {
                tempDirection = WalkingDirection::Left;
                adjustment = Vector2(-85, 15);
            }
        }
    }

    void DetermineDirectionDominantY(WalkingDirection& tempDirection, Vector2& adjustment) {
        Vector2 stick = VirtualThumbsticks::getLeftThumbstick();
        if (stick.Y > 0) {
            if (stick.X > 0.25f) {
                tempDirection = WalkingDirection::RightDown;
                adjustment = Vector2(85, 0);
            } else if (stick.X < -0.25f) {
                tempDirection = WalkingDirection::LeftDown;
                adjustment = Vector2(-85, 0);
            } else {
                tempDirection = WalkingDirection::Down;
                adjustment = Vector2::Zero;
            }
        } else {
            if (stick.X > 0.25f) {
                tempDirection = WalkingDirection::RightUp;
                adjustment = Vector2(85, 30);
            } else if (stick.X < -0.25f) {
                tempDirection = WalkingDirection::LeftUp;
                adjustment = Vector2(-85, 30);
            } else {
                tempDirection = WalkingDirection::Up;
                adjustment = Vector2::Zero;
            }
        }
    }

    Vector2 bodySize_ = Vector2(85, 132);
    Vector2 velocity_;
    Vector2 smokeAdjustment_;
    SpriteEffects lastEffect_ = SpriteEffects::None;
    SpriteEffects currentEffect_ = SpriteEffects::None;

    bool needToShootSmoke_ = false;
    bool isStung_ = false;
    bool isFlashing_ = false;
    bool isDrawnLastStungInterval_ = false;
    bool isDepositingHoney_ = false;

    System::TimeSpan stungTime_;
    System::TimeSpan stungDuration_ = System::TimeSpan::FromSeconds(1);
    System::TimeSpan flashingDuration_ = System::TimeSpan::FromSeconds(2);
    System::TimeSpan depositHoneyUpdatingInterval_ = System::TimeSpan::FromMilliseconds(200);
    System::TimeSpan depositHoneyUpdatingTimer_ = System::TimeSpan::Zero;
    System::TimeSpan shootSmokePuffTimer_ = System::TimeSpan::Zero;
    System::TimeSpan shootSmokePuffTimerInitialValue_ = System::TimeSpan::FromMilliseconds(325);

    Texture2D smokeAnimationTexture_;
    Texture2D smokePuffTexture_;
    Texture2D hitTexture_;

    std::deque<std::shared_ptr<SmokePuff>> firedSmokePuffs_;
    std::stack<std::shared_ptr<SmokePuff>> availableSmokePuffs_;

    int stungDrawingInterval_ = 5;
    int stungDrawingCounter_ = 0;
    int honeyDepositFrameCount_ = 0;
    int depositHoneyTimerCounter_ = -1;
    int collectingHoneyFrameCounter_ = 0;

    std::function<void()> depositHoneyCallback_;

    WalkingDirection direction_ = WalkingDirection::Up;
    int lastFrameCounter_ = 0;
};

} // namespace HoneycombRush

#pragma once
#include <cmath>
#include <optional>
#include "Animation.hpp"
#include "AnimationPlayer.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

namespace Platformer {

class Level;
class Enemy;

class Player {
    std::optional<Animation> idleAnimation_;
    std::optional<Animation> runAnimation_;
    std::optional<Animation> jumpAnimation_;
    std::optional<Animation> celebrateAnimation_;
    std::optional<Animation> dieAnimation_;
    Microsoft::Xna::Framework::Graphics::SpriteEffects flip_ = Microsoft::Xna::Framework::Graphics::SpriteEffects::None;
    AnimationPlayer sprite_;

    std::optional<Microsoft::Xna::Framework::Audio::SoundEffect> killedSound_;
    std::optional<Microsoft::Xna::Framework::Audio::SoundEffect> jumpSound_;
    std::optional<Microsoft::Xna::Framework::Audio::SoundEffect> fallSound_;

    Level* level_;
    bool isAlive_ = false;
    Microsoft::Xna::Framework::Vector2 position_;
    float previousBottom_ = 0.0f;
    Microsoft::Xna::Framework::Vector2 velocity_;

    static constexpr float MoveAcceleration = 13000.0f;
    static constexpr float MaxMoveSpeed     = 1750.0f;
    static constexpr float GroundDragFactor  = 0.48f;
    static constexpr float AirDragFactor     = 0.58f;
    static constexpr float MaxJumpTime       = 0.35f;
    static constexpr float JumpLaunchVelocity = -3500.0f;
    static constexpr float GravityAcceleration = 3400.0f;
    static constexpr float MaxFallSpeed      = 550.0f;
    static constexpr float JumpControlPower  = 0.14f;
    static constexpr float MoveStickScale    = 1.0f;

    bool isOnGround_ = false;
    float movement_ = 0.0f;
    bool isJumping_ = false;
    bool wasJumping_ = false;
    float jumpTime_ = 0.0f;
    Microsoft::Xna::Framework::Rectangle localBounds_;

public:
    Player(Level* level, Microsoft::Xna::Framework::Vector2 position);
    void LoadContent();
    void Reset(Microsoft::Xna::Framework::Vector2 position);

    Level* getLevelProperty() const { return level_; }
    bool getIsAliveProperty() const { return isAlive_; }
    bool getIsOnGroundProperty() const { return isOnGround_; }

    Microsoft::Xna::Framework::Vector2 getPositionProperty() const { return position_; }
    void setPositionProperty(Microsoft::Xna::Framework::Vector2 value) { position_ = value; }

    Microsoft::Xna::Framework::Vector2 getVelocityProperty() const { return velocity_; }
    void setVelocityProperty(Microsoft::Xna::Framework::Vector2 value) { velocity_ = value; }

    Microsoft::Xna::Framework::Rectangle getBoundingRectangleProperty() const {
        int left = (int)std::round(position_.X - sprite_.getOriginProperty().X) + localBounds_.X;
        int top  = (int)std::round(position_.Y - sprite_.getOriginProperty().Y) + localBounds_.Y;
        return Microsoft::Xna::Framework::Rectangle(left, top, localBounds_.Width, localBounds_.Height);
    }

    void Update(Microsoft::Xna::Framework::GameTime& gameTime,
                const Microsoft::Xna::Framework::Input::KeyboardState& keyboardState,
                const Microsoft::Xna::Framework::Input::GamePadState& gamePadState);
    void ApplyPhysics(Microsoft::Xna::Framework::GameTime& gameTime);
    void OnKilled(Enemy* killedBy);
    void OnReachedExit();
    void Draw(Microsoft::Xna::Framework::GameTime& gameTime,
              Microsoft::Xna::Framework::Graphics::SpriteBatch& spriteBatch);

private:
    void GetInput(const Microsoft::Xna::Framework::Input::KeyboardState& keyboardState,
                  const Microsoft::Xna::Framework::Input::GamePadState& gamePadState);
    float DoJump(float velocityY, Microsoft::Xna::Framework::GameTime& gameTime);
    void HandleCollisions();
};

} // namespace Platformer

#pragma once
#include <optional>
#include <string>
#include "Animation.hpp"
#include "AnimationPlayer.hpp"
#include "Tile.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"

namespace Platformer {

class Level;
class Player;

enum class FaceDirection { Left = -1, Right = 1 };

class Enemy {
    Level* level_;
    Microsoft::Xna::Framework::Vector2 position_;
    Microsoft::Xna::Framework::Rectangle localBounds_;
    std::optional<Animation> runAnimation_;
    std::optional<Animation> idleAnimation_;
    AnimationPlayer sprite_;
    FaceDirection direction_ = FaceDirection::Left;
    float waitTime_ = 0.0f;

    static constexpr float MaxWaitTime = 0.5f;
    static constexpr float MoveSpeed   = 64.0f;

public:
    Enemy(Level* level, Microsoft::Xna::Framework::Vector2 position, const std::string& spriteSet);
    void LoadContent(const std::string& spriteSet);

    Level* getLevelProperty() const { return level_; }

    Microsoft::Xna::Framework::Vector2 getPositionProperty() const { return position_; }

    Microsoft::Xna::Framework::Rectangle getBoundingRectangleProperty() const {
        int left = (int)std::round(position_.X - sprite_.getOriginProperty().X) + localBounds_.X;
        int top  = (int)std::round(position_.Y - sprite_.getOriginProperty().Y) + localBounds_.Y;
        return Microsoft::Xna::Framework::Rectangle(left, top, localBounds_.Width, localBounds_.Height);
    }

    void Update(Microsoft::Xna::Framework::GameTime& gameTime);
    void Draw(Microsoft::Xna::Framework::GameTime& gameTime,
              Microsoft::Xna::Framework::Graphics::SpriteBatch& spriteBatch);
};

} // namespace Platformer

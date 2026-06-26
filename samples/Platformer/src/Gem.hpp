#pragma once
#include <optional>
#include "Circle.hpp"
#include "Tile.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace Platformer {

class Level;
class Player;

class Gem {
    std::optional<Microsoft::Xna::Framework::Graphics::Texture2D> texture_;
    Microsoft::Xna::Framework::Vector2 origin_;
    std::optional<Microsoft::Xna::Framework::Audio::SoundEffect> collectedSound_;
    Microsoft::Xna::Framework::Vector2 basePosition_;
    float bounce_ = 0.0f;
    Level* level_ = nullptr;

public:
    static constexpr int PointValue = 30;
    inline static const Microsoft::Xna::Framework::Color Color_{255, 255, 0, 255};

    Gem(Level* level, Microsoft::Xna::Framework::Vector2 position);
    void LoadContent();

    Microsoft::Xna::Framework::Vector2 getPositionProperty() const {
        return basePosition_ + Microsoft::Xna::Framework::Vector2(0.0f, bounce_);
    }

    Circle getBoundingCircleProperty() const {
        return Circle(getPositionProperty(), Tile::Width / 3.0f);
    }

    void Update(Microsoft::Xna::Framework::GameTime& gameTime);
    void OnCollected(Player& collectedBy);
    void Draw(Microsoft::Xna::Framework::GameTime& gameTime,
              Microsoft::Xna::Framework::Graphics::SpriteBatch& spriteBatch);
};

} // namespace Platformer

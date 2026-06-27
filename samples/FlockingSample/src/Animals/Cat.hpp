#pragma once
#include "Animal.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"

namespace Flocking {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class Cat : public Animal {
public:
    Cat(Texture2D tex, int screenWidth, int screenHeight)
        : Animal(std::move(tex), screenWidth, screenHeight)
    {
        location_   = Vector2((float)screenWidth / 2.0f, (float)screenHeight / 2.0f);
        moveSpeed_  = 500.0f;
        animalType_ = AnimalType::Cat;
    }

    void SetInputDirection(float dx, float dy) {
        direction_.X = dx;
        direction_.Y = dy;
    }

    void Update(const GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        if (direction_.LengthSquared() > 0.01f) {
            location_.X += direction_.X * moveSpeed_ * elapsed;
            location_.X = MathHelper::Clamp(location_.X, 0.0f, (float)boundaryWidth_);
            location_.Y += direction_.Y * moveSpeed_ * elapsed;
            location_.Y = MathHelper::Clamp(location_.Y, 0.0f, (float)boundaryHeight_);
        }
    }

    void Draw(SpriteBatch& spriteBatch, const GameTime& /*gameTime*/) override {
        spriteBatch.Draw(*texture_, location_, std::nullopt,
            color_, 0.0f, textureCenter_, 1.0f, SpriteEffects::None, 0.0f);
    }
};

} // namespace Flocking

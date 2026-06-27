#pragma once
#include <cmath>
#include <optional>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace Flocking {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

enum class AnimalType { Generic, Bird, Cat };

class Animal {
public:
    AnimalType GetAnimalType() const { return animalType_; }

    float ReactionDistance() const  { return reactionDistance_; }
    Vector2 ReactionLocation() const { return reactionLocation_; }

    bool Fleeing() const      { return fleeing_; }
    void SetFleeing(bool v)   { fleeing_ = v; }

    int BoundaryWidth()  const { return boundaryWidth_; }
    int BoundaryHeight() const { return boundaryHeight_; }

    const Vector2& Direction() const { return direction_; }
    Vector2& Direction()             { return direction_; }

    const Vector2& Location() const { return location_; }
    Vector2& Location()             { return location_; }

protected:
    std::optional<Texture2D> texture_;
    Color color_ = Color(255, 255, 255, 255);
    Vector2 textureCenter_;
    float moveSpeed_ = 0.0f;

    AnimalType animalType_ = AnimalType::Generic;
    float reactionDistance_ = 0.0f;
    Vector2 reactionLocation_;
    bool fleeing_ = false;

    int boundaryWidth_  = 0;
    int boundaryHeight_ = 0;

    Vector2 direction_;
    Vector2 location_;

    Animal(Texture2D tex, int screenWidth, int screenHeight)
        : texture_(std::move(tex))
        , boundaryWidth_(screenWidth)
        , boundaryHeight_(screenHeight)
    {
        textureCenter_ = Vector2(
            (float)(texture_->getWidthProperty()  / 2),
            (float)(texture_->getHeightProperty() / 2));
    }

public:
    virtual ~Animal() = default;

    virtual void Update(const GameTime& /*gameTime*/) {}

    virtual void Draw(SpriteBatch& spriteBatch, const GameTime& /*gameTime*/) {
        float rotation = std::atan2(direction_.Y, direction_.X);
        spriteBatch.Draw(*texture_, location_, std::nullopt,
            color_, rotation, textureCenter_, 1.0f, SpriteEffects::None, 0.0f);
    }
};

} // namespace Flocking

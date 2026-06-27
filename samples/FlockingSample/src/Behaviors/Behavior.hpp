#pragma once
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "../AIParameters.hpp"

namespace Flocking {

class Animal;  // forward declaration — implementations include Animal.hpp

using namespace Microsoft::Xna::Framework;

class Behavior {
public:
    Animal* GetAnimal() const { return animal_; }

    const Vector2& Reaction() const { return reaction_; }
    bool Reacted() const            { return reacted_; }

protected:
    Animal* animal_;
    Vector2 reaction_;
    bool reacted_ = false;

    explicit Behavior(Animal& animal) : animal_(&animal) {}

    void ResetReaction() {
        reacted_ = false;
        reaction_ = Vector2::Zero;
    }

public:
    virtual ~Behavior() = default;
    virtual void Update(Animal& otherAnimal, const AIParameters& aiParams) = 0;
};

} // namespace Flocking

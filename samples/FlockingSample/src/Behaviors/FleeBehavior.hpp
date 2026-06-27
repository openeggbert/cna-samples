#pragma once
#include "Behavior.hpp"
#include "../Animals/Animal.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"

namespace Flocking {

class FleeBehavior : public Behavior {
public:
    explicit FleeBehavior(Animal& animal) : Behavior(animal) {}

    void Update(Animal& /*otherAnimal*/, const AIParameters& aiParams) override {
        ResetReaction();

        if (Vector2::Dot(animal_->Location(), animal_->ReactionLocation())
            >= -(MathHelper::Pi / 2.0f))
        {
            animal_->SetFleeing(true);
            reacted_ = true;

            Vector2 danger = animal_->Location() - animal_->ReactionLocation();
            danger.Normalize();
            reaction_ = danger * aiParams.PerDangerWeight;
        }
    }
};

} // namespace Flocking

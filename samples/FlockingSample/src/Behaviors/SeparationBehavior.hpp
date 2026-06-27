#pragma once
#include "Behavior.hpp"
#include "../Animals/Animal.hpp"

namespace Flocking {

class SeparationBehavior : public Behavior {
public:
    explicit SeparationBehavior(Animal& animal) : Behavior(animal) {}

    void Update(Animal& /*otherAnimal*/, const AIParameters& aiParams) override {
        ResetReaction();

        if (animal_->ReactionDistance() > 0.0f
            && animal_->ReactionDistance() <= aiParams.SeparationDistance)
        {
            Vector2 push = animal_->Location() - animal_->ReactionLocation();
            push.Normalize();

            float w = aiParams.PerMemberWeight
                * (1.0f - animal_->ReactionDistance() / aiParams.SeparationDistance);
            reaction_ = push * w;
            reacted_  = true;
        }
    }
};

} // namespace Flocking

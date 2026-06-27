#pragma once
#include <cmath>
#include "Behavior.hpp"
#include "../Animals/Animal.hpp"

namespace Flocking {

class CohesionBehavior : public Behavior {
public:
    explicit CohesionBehavior(Animal& animal) : Behavior(animal) {}

    void Update(Animal& /*otherAnimal*/, const AIParameters& aiParams) override {
        ResetReaction();

        if (animal_->ReactionDistance() > 0.0f
            && animal_->ReactionDistance() > aiParams.SeparationDistance)
        {
            Vector2 pull = -(animal_->Location() - animal_->ReactionLocation());
            pull.Normalize();

            float w = aiParams.PerMemberWeight
                * (float)std::pow(
                    (double)(animal_->ReactionDistance() - aiParams.SeparationDistance)
                    / (aiParams.DetectionDistance  - aiParams.SeparationDistance), 2.0);

            reaction_ = pull * w;
            reacted_  = true;
        }
    }
};

} // namespace Flocking

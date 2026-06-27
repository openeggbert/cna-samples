#pragma once
#include "Behavior.hpp"
#include "../Animals/Animal.hpp"

namespace Flocking {

class AlignBehavior : public Behavior {
public:
    explicit AlignBehavior(Animal& animal) : Behavior(animal) {}

    void Update(Animal& otherAnimal, const AIParameters& aiParams) override {
        ResetReaction();
        reacted_ = true;
        reaction_ = otherAnimal.Direction() * aiParams.PerMemberWeight;
    }
};

} // namespace Flocking

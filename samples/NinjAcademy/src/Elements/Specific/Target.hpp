#pragma once

// Target.hpp — C++ port of Elements/Specific/Target.cs (XNA 4.0 NinjAcademy
// sample). A target which moves along a "conveyer belt" or is placed
// anywhere (golden targets).

#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/ContainmentType.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

#include "../../GameConstants.hpp"
#include "../StraightLineMovementComponent.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::BoundingSphere;
using Microsoft::Xna::Framework::ContainmentType;
using Microsoft::Xna::Framework::Vector3;

// Possible target positions on the screen.
enum class TargetPosition {
    Upper,
    Middle,
    Lower,
    Anywhere,
};

// Port of Elements/Specific/Target.cs.
class Target : public StraightLineMovementComponent {
public:
    TargetPosition Designation = TargetPosition::Upper;
    bool IsGolden = false;

    Target(Game& game, GameScreen* gameScreen, Texture2D texture)
        : StraightLineMovementComponent(game, gameScreen, std::move(texture)) {}

    Target(Game& game, GameScreen* gameScreen, Animation animation)
        : StraightLineMovementComponent(game, gameScreen, std::move(animation)) {}

    // Checks whether a specified position (Z expected to be 0) constitutes a hit on the target.
    bool CheckHit(Vector3 hitLocation) const {
        BoundingSphere exactBounds(Vector3(Position, 0.0f), GameConstants::TargetRadius);
        return exactBounds.Contains(hitLocation) == ContainmentType::Contains;
    }
};

} // namespace NinjAcademy

#pragma once

// ThrowingStar.hpp — C++ port of Elements/Specific/ThrowingStar.cs (XNA 4.0
// NinjAcademy sample). Represents a thrown shuriken.

#include "System/Random.hpp"

#include "../../GameConstants.hpp"
#include "../StraightLineScalingComponent.hpp"

namespace NinjAcademy {

// Port of Elements/Specific/ThrowingStar.cs.
class ThrowingStar : public StraightLineScalingComponent {
public:
    ThrowingStar(Game& game, GameScreen* gameScreen, Animation animation)
        : StraightLineScalingComponent(game, gameScreen, std::move(animation)) {}

    // Launches the throwing star to a specified destination.
    void Throw(Vector2 destination) {
        setEnabledProperty(true);
        setVisibleProperty(true);

        // Cause each star thrown to spin differently.
        animation_.setFrameIndex(random_.Next(animation_.FrameCount()));

        MoveAndScale(GameConstants::ThrowingStarFlightDuration, GameConstants::ThrowingStarOrigin, destination, 1.0f,
                     GameConstants::ThrowingStarEndScale);
    }

private:
    System::Random random_;
};

} // namespace NinjAcademy

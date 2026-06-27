#pragma once
// Include this ONCE from Program.cpp after all headers to resolve the
// circular dependency Tank <-> Behavior.
#include "Tank.hpp"
#include "Behaviors/Behavior.hpp"
#include "Behaviors/LinearBehavior.hpp"
#include "Behaviors/SteeringBehavior.hpp"

namespace WaypointSample {

inline Behavior::Behavior(Tank& tank) : tank_(tank) {
    tank_.SetMoveSpeed(Tank::MaxMoveSpeed);
}

inline void Tank::Update(const GameTime& gameTime) {
    float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
    if (waypoints_.Count() > 0) {
        if (AtDestination()) {
            waypoints_.Dequeue();
        } else {
            if (currentBehavior_) currentBehavior_->Update(gameTime);
            location_.X += direction_.X * moveSpeed_ * elapsed;
            location_.Y += direction_.Y * moveSpeed_ * elapsed;
        }
    }
}

inline void Tank::SetBehaviorType(BehaviorType t) {
    if (behaviorType_ != t || !currentBehavior_) {
        behaviorType_ = t;
        switch (t) {
            case BehaviorType::Linear:
                currentBehavior_ = std::make_unique<LinearBehavior>(*this);
                break;
            case BehaviorType::Steering:
                currentBehavior_ = std::make_unique<SteeringBehavior>(*this);
                break;
        }
    }
}

} // namespace WaypointSample

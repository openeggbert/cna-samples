#pragma once

// WorkerBee.hpp — C++ port of Objects/WorkerBee.cs (XNA 4.0 HoneycombRush sample).

#include "../Misc/ConfigurationManager.hpp"
#include "Bee.hpp"

namespace HoneycombRush {

// A component that represents a worker bee. Port of Objects/WorkerBee.cs.
class WorkerBee : public Bee {
public:
    WorkerBee(Game& game, GameplayScreen* gamePlayScreen, Beehive* beehive) : Bee(game, gamePlayScreen, beehive) {
        animationKey_ = "WorkerBee";
    }

    void LoadContent() override {
        texture = getGameProperty().getContentProperty().Load<Texture2D>("Textures/beeWingFlap");
        DrawableGameComponent::LoadContent();
    }

protected:
    int MaxVelocity() const override {
        return (int)ConfigurationManager::ModesConfiguration()
            .at(ConfigurationManager::CurrentDifficultyMode().value())
            .MaxWorkerBeeVelocity;
    }

    float AccelerationFactor() const override { return 1.5f; }
};

} // namespace HoneycombRush

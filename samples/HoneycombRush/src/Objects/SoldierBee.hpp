#pragma once

// SoldierBee.hpp — C++ port of Objects/SoldierBee.cs (XNA 4.0 HoneycombRush
// sample). Adds beekeeper-chasing logic to the base Bee behavior.

#include "../Misc/ConfigurationManager.hpp"
#include "Bee.hpp"

namespace HoneycombRush {

// A component that represents a soldier bee (chases the beekeeper). Port of
// Objects/SoldierBee.cs.
class SoldierBee : public Bee {
public:
    float DistanceFromBeeKeeper = 0.0f;
    Vector2 BeeKeeperVector;

    SoldierBee(Game& game, GameplayScreen* gamePlayScreen, Beehive* beehive) : Bee(game, gamePlayScreen, beehive) {
        animationKey_ = "SoldierBee";
    }

    void LoadContent() override {
        texture = getGameProperty().getContentProperty().Load<Texture2D>("Textures/SoldierBeeWingFlap");
        DrawableGameComponent::LoadContent();
    }

    // Defined out-of-line in GameplayScreen.hpp — needs
    // GameplayScreen::IsActive() and AnimationDefinitions access already used
    // by the base Bee::Update, but this override has its own chase logic.
    void Update(GameTime& gameTime) override;

protected:
    int MaxVelocity() const override {
        return (int)ConfigurationManager::ModesConfiguration()
            .at(ConfigurationManager::CurrentDifficultyMode().value())
            .MaxSoldierBeeVelocity;
    }

    float AccelerationFactor() const override { return 20.0f; }

private:
    float chaseDistance_ = 70.0f;
    bool isChaseMode_ = false;
};

} // namespace HoneycombRush

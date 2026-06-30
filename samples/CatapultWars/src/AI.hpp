#pragma once

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "System/Random.hpp"

#include "Player.hpp"

namespace CatapultWars {

// The computer player. Aims for a random number of frames, then fires at a
// random strength.
class AI : public Player {
public:
    AI(Game& game, SpriteBatch& screenSpriteBatch)
        : Player(game, screenSpriteBatch) {
        setCatapult(std::make_shared<Catapult>(
            game, screenSpriteBatch,
            "Textures/Catapults/Red/redIdle/redIdle",
            Vector2(600, 332), SpriteEffects::FlipHorizontally, true));
    }

    void Initialize() override {
        catapult_->Initialize();
        Player::Initialize();
    }

    void Update(const GameTime& gameTime) override {
        // Check if it is time to take a shot.
        if (catapult_->getCurrentState() == CatapultState::Aiming &&
            !catapult_->getAnimationRunning()) {
            float shotVelocity = (float)random_.Next((int)MinShotStrength, (int)MaxShotStrength);
            catapult_->setShotStrength(shotVelocity / MaxShotStrength);
            catapult_->setShotVelocity(shotVelocity);
        }
        Player::Update(gameTime);
    }

private:
    System::Random random_;
};

} // namespace CatapultWars

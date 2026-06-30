#pragma once

#include <algorithm>
#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"

#include "Catapult.hpp"
#include "AudioManager.hpp"

namespace CatapultWars {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;

// Base class for the two players. Owns a Catapult and a score.
class Player {
public:
    // Constants used for calculating shot strength.
    static constexpr float MinShotStrength = 150;
    static constexpr float MaxShotStrength = 400;

    explicit Player(Game& game) : game_(&game) {}
    Player(Game& game, SpriteBatch& screenSpriteBatch)
        : game_(&game), spriteBatch_(&screenSpriteBatch) {}
    virtual ~Player() = default;

    Catapult* getCatapult() const { return catapult_.get(); }
    void setCatapult(std::shared_ptr<Catapult> c) { catapult_ = std::move(c); }

    int getScore() const { return score_; }
    void setScore(int v) { score_ = v; }
    void addScore(int delta) { score_ += delta; }

    const std::string& getName() const { return name_; }
    void setName(const std::string& v) { name_ = v; }

    // Setting the enemy wires up the catapult's enemy/self back-pointers.
    void setEnemy(Player* enemy) {
        catapult_->setEnemy(enemy);
        catapult_->setSelf(this);
    }

    bool getIsActive() const { return isActive_; }
    void setIsActive(bool v) { isActive_ = v; }

    virtual void Initialize() {
        score_ = 0;
    }

    virtual void Update(const GameTime& gameTime) {
        catapult_->Update(gameTime);
    }

    virtual void Draw(const GameTime& gameTime) {
        catapult_->Draw(gameTime);
    }

protected:
    Game* game_ = nullptr;
    SpriteBatch* spriteBatch_ = nullptr;
    std::shared_ptr<Catapult> catapult_;
    int score_ = 0;
    std::string name_;
    bool isActive_ = false;
};

// ---- Catapult methods that need the full Player definition ----

inline int Catapult::getEnemyScore() const {
    return enemy_->getScore();
}

inline bool Catapult::CheckHit() {
    bool bRes = false;

    // Build a sphere around the projectile.
    Vector2 pp = projectile_->getProjectilePosition();
    Vector3 center(pp.X, pp.Y, 0);
    int radius = std::max(projectile_->getProjectileTexture().getWidthProperty() / 2,
                          projectile_->getProjectileTexture().getHeightProperty() / 2);
    BoundingSphere sphere(center, (float)radius);

    float fw = (float)animations_.at("Fire").getFrameSize().X;
    float fh = (float)animations_.at("Fire").getFrameSize().Y;

    // Bounding box around self.
    BoundingBox selfBox(Vector3(catapultPosition_.X, catapultPosition_.Y, 0),
                        Vector3(catapultPosition_.X + fw, catapultPosition_.Y + fh, 0));

    // Bounding box around the enemy.
    Vector2 enemyPos = enemy_->getCatapult()->getPosition();
    BoundingBox enemyBox(Vector3(enemyPos.X, enemyPos.Y, 0),
                         Vector3(enemyPos.X + fw, enemyPos.Y + fh, 0));

    if (sphere.Intersects(selfBox) && currentState_ != CatapultState::Hit) {
        AudioManager::PlaySound("catapultExplosion");
        Hit();
        enemy_->addScore(1);
        bRes = true;
    } else if (sphere.Intersects(enemyBox) &&
               enemy_->getCatapult()->getCurrentState() != CatapultState::Hit &&
               enemy_->getCatapult()->getCurrentState() != CatapultState::Reset) {
        AudioManager::PlaySound("catapultExplosion");
        enemy_->getCatapult()->Hit();
        self_->addScore(1);
        bRes = true;
        currentState_ = CatapultState::Reset;
    }

    return bRes;
}

} // namespace CatapultWars

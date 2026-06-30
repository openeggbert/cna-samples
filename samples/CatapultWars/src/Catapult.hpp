#pragma once

#include <algorithm>
#include <map>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/BoundingSphere.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

#include "Animation.hpp"
#include "Projectile.hpp"
#include "AudioManager.hpp"

namespace CatapultWars {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::BoundingBox;
using Microsoft::Xna::Framework::BoundingSphere;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;

class Player;  // forward declaration (Catapult holds non-owning back-pointers)

// Bit-flag state of a catapult during its update cycle.
enum class CatapultState : int {
    Idle             = 0x0,
    Aiming           = 0x1,
    Firing           = 0x2,
    ProjectileFlying = 0x4,
    ProjectileHit    = 0x8,
    Hit              = 0x10,
    Reset            = 0x20,
    Stalling         = 0x40,
};

constexpr CatapultState operator|(CatapultState a, CatapultState b) {
    return static_cast<CatapultState>(static_cast<int>(a) | static_cast<int>(b));
}
constexpr CatapultState operator&(CatapultState a, CatapultState b) {
    return static_cast<CatapultState>(static_cast<int>(a) & static_cast<int>(b));
}

class Catapult {
public:
    Catapult(Game& game, SpriteBatch& screenSpriteBatch, const std::string& idleTexture,
             Vector2 catapultPosition, SpriteEffects spriteEffect, bool isAI)
        : game_(&game), spriteBatch_(&screenSpriteBatch), idleTextureName_(idleTexture),
          catapultPosition_(catapultPosition), spriteEffects_(spriteEffect), isAI_(isAI) {}

    bool getAnimationRunning() const { return animationRunning_; }
    void setAnimationRunning(bool v) { animationRunning_ = v; }

    bool getIsActive() const { return isActive_; }
    void setIsActive(bool v) { isActive_ = v; }

    CatapultState getCurrentState() const { return currentState_; }
    void setCurrentState(CatapultState v) { currentState_ = v; }

    void setWind(float v) { wind_ = v; }

    void setEnemy(Player* v) { enemy_ = v; }
    void setSelf(Player* v) { self_ = v; }

    Vector2 getPosition() const { return catapultPosition_; }

    float getShotStrength() const { return shotStrength_; }
    void setShotStrength(float v) { shotStrength_ = v; }

    float getShotVelocity() const { return shotVelocity_; }
    void setShotVelocity(float v) { shotVelocity_ = v; }

    bool getGameOver() const { return gameOver_; }
    void setGameOver(bool v) { gameOver_ = v; }

    // Loads animations from the (hardcoded) definition table and the projectile.
    void Initialize() {
        isActive_ = true;
        animationRunning_ = false;
        currentState_ = CatapultState::Idle;
        stallUpdateCycles_ = 0;

        // Animation definitions, ported from
        // Content/Textures/Catapults/AnimationsDef.xml (the original parses the
        // XML at runtime; we inline the data — see missing.md).
        struct AnimDef {
            const char* alias;
            int frameW, frameH, cols, rows;
            bool hasOffset; int offX, offY;
            bool hasSplit; int split;
            const char* sheet;
        };
        static const AnimDef humanDefs[] = {
            {"Fire", 75, 60, 15, 2, false, 0, 0, true, 20,
                "Textures/Catapults/Blue/blueFire/blueCatapult_fire"},
            {"Aim", 75, 60, 18, 1, false, 0, 0, false, 0,
                "Textures/Catapults/Blue/bluePullback/blueCatapult_Pullback"},
            {"Destroyed", 122, 62, 15, 2, true, -40, 0, false, 0,
                "Textures/Catapults/Blue/blueDestroyed/blueCatapult_destroyed"},
            {"fireMiss", 90, 80, 15, 2, true, -50, 0, false, 0,
                "Textures/Catapults/Fire_Miss/fire_miss"},
            {"hitSmoke", 128, 128, 15, 2, true, 0, -64, false, 0,
                "Textures/Catapults/Hit_Smoke/smoke"},
        };
        static const AnimDef aiDefs[] = {
            {"Fire", 75, 60, 15, 2, false, 0, 0, true, 20,
                "Textures/Catapults/Red/redFire/redCatapult_fire"},
            {"Destroyed", 122, 62, 15, 2, true, -11, 0, false, 0,
                "Textures/Catapults/Red/redDestroyed/redCatapult_destroyed"},
            {"Aim", 75, 60, 18, 1, false, 0, 0, false, 0,
                "Textures/Catapults/Red/redPullback/redCatapult_Pullback"},
            {"fireMiss", 90, 80, 15, 2, true, -50, 0, false, 0,
                "Textures/Catapults/Fire_Miss/fire_miss"},
            {"hitSmoke", 128, 128, 15, 2, true, -30, -64, false, 0,
                "Textures/Catapults/Hit_Smoke/smoke"},
        };

        const AnimDef* defs = isAI_ ? aiDefs : humanDefs;
        size_t defCount = isAI_ ? (sizeof(aiDefs) / sizeof(aiDefs[0]))
                                 : (sizeof(humanDefs) / sizeof(humanDefs[0]));

        auto& content = game_->getContentProperty();
        for (size_t i = 0; i < defCount; i++) {
            const AnimDef& d = defs[i];
            animTextures_.emplace(d.alias, content.Load<Texture2D>(d.sheet));
            const Texture2D* tex = &animTextures_.at(d.alias);

            if (d.hasSplit)
                splitFrames_[d.alias] = d.split;

            Animation animation(tex, Point(d.frameW, d.frameH), Point(d.cols, d.rows));
            if (d.hasOffset)
                animation.setOffset(Vector2((float)d.offX, (float)d.offY));

            animations_.emplace(d.alias, animation);
        }

        idleTexture_.emplace(content.Load<Texture2D>(idleTextureName_));

        Vector2 projectileStartPosition = isAI_ ? Vector2(630, 340) : Vector2(175, 340);
        projectile_.emplace(*game_, *spriteBatch_, "Textures/Ammo/rock_ammo",
                            projectileStartPosition,
                            (float)animations_.at("Fire").getFrameSize().Y, isAI_, gravity);
        projectile_->Initialize();
    }

    void Update(const GameTime& gameTime) {
        bool isGroundHit;
        bool startStall;
        CatapultState postUpdateStateChange = CatapultState::Idle;

        if (!isActive_)
            return;

        switch (currentState_) {
        case CatapultState::Idle:
            break;
        case CatapultState::Aiming:
            if (lastUpdateState_ != CatapultState::Aiming) {
                AudioManager::PlaySound("ropeStretch", true);
                animationRunning_ = true;
                if (isAI_) {
                    animations_.at("Aim").PlayFromFrameIndex(0);
                    stallUpdateCycles_ = 20;
                    startStall = false;
                }
            }
            if (!isAI_) {
                UpdateAimAccordingToShotStrength();
            } else {
                animations_.at("Aim").Update();
                startStall = AimReachedShotStrength();
                currentState_ = startStall ? CatapultState::Stalling : CatapultState::Aiming;
            }
            break;
        case CatapultState::Stalling:
            if (stallUpdateCycles_-- <= 0) {
                Fire(shotVelocity_);
                postUpdateStateChange = CatapultState::Firing;
            }
            break;
        case CatapultState::Firing:
            if (lastUpdateState_ != CatapultState::Firing) {
                AudioManager::StopSound("ropeStretch");
                AudioManager::PlaySound("catapultFire");
                StartFiringFromLastAimPosition();
            }
            animations_.at("Fire").Update();
            if (animations_.at("Fire").getFrameIndex() == splitFrames_["Fire"]) {
                postUpdateStateChange = currentState_ | CatapultState::ProjectileFlying;
                projectile_->setProjectilePosition(projectile_->getProjectileStartPosition());
            }
            break;
        case CatapultState::Firing | CatapultState::ProjectileFlying:
            animations_.at("Fire").Update();
            projectile_->UpdateProjectileFlightData(gameTime, wind_, gravity, isGroundHit);
            if (isGroundHit) {
                postUpdateStateChange = CatapultState::ProjectileHit;
                animations_.at("fireMiss").PlayFromFrameIndex(0);
            }
            break;
        case CatapultState::ProjectileFlying:
            projectile_->UpdateProjectileFlightData(gameTime, wind_, gravity, isGroundHit);
            if (isGroundHit) {
                postUpdateStateChange = CatapultState::ProjectileHit;
                animations_.at("fireMiss").PlayFromFrameIndex(0);
            }
            break;
        case CatapultState::ProjectileHit:
            if (!CheckHit()) {
                if (lastUpdateState_ != CatapultState::ProjectileHit) {
                    // (Windows Phone vibration omitted on desktop.)
                    AudioManager::PlaySound("boulderHit");
                }
                if (animations_.at("fireMiss").getIsActive() == false) {
                    postUpdateStateChange = CatapultState::Reset;
                }
                animations_.at("fireMiss").Update();
            }
            break;
        case CatapultState::Hit:
            if ((animations_.at("Destroyed").getIsActive() == false) &&
                (animations_.at("hitSmoke").getIsActive() == false)) {
                if (getEnemyScore() >= winScore) {
                    gameOver_ = true;
                    break;
                }
                postUpdateStateChange = CatapultState::Reset;
            }
            animations_.at("Destroyed").Update();
            animations_.at("hitSmoke").Update();
            break;
        case CatapultState::Reset:
            animationRunning_ = false;
            break;
        default:
            break;
        }

        lastUpdateState_ = currentState_;
        if (postUpdateStateChange != CatapultState::Idle) {
            currentState_ = postUpdateStateChange;
        }
    }

    void Draw(const GameTime& gameTime) {
        switch (lastUpdateState_) {
        case CatapultState::Idle:
            DrawIdleCatapult();
            break;
        case CatapultState::Aiming:
        case CatapultState::Stalling:
            animations_.at("Aim").Draw(*spriteBatch_, catapultPosition_, spriteEffects_);
            break;
        case CatapultState::Firing:
            animations_.at("Fire").Draw(*spriteBatch_, catapultPosition_, spriteEffects_);
            break;
        case CatapultState::Firing | CatapultState::ProjectileFlying:
        case CatapultState::ProjectileFlying:
            animations_.at("Fire").Draw(*spriteBatch_, catapultPosition_, spriteEffects_);
            projectile_->Draw(gameTime);
            break;
        case CatapultState::ProjectileHit:
            DrawIdleCatapult();
            animations_.at("fireMiss").Draw(*spriteBatch_,
                projectile_->getProjectileHitPosition(), spriteEffects_);
            break;
        case CatapultState::Hit:
            animations_.at("Destroyed").Draw(*spriteBatch_, catapultPosition_, spriteEffects_);
            animations_.at("hitSmoke").Draw(*spriteBatch_, catapultPosition_, spriteEffects_);
            break;
        case CatapultState::Reset:
            DrawIdleCatapult();
            break;
        default:
            break;
        }
    }

    // Start the hit/destroyed sequence (called on self or by the enemy).
    void Hit() {
        animationRunning_ = true;
        animations_.at("Destroyed").PlayFromFrameIndex(0);
        animations_.at("hitSmoke").PlayFromFrameIndex(0);
        currentState_ = CatapultState::Hit;
    }

    void Fire(float velocity) {
        projectile_->Fire(velocity, velocity);
    }

private:
    bool AimReachedShotStrength() {
        return animations_.at("Aim").getFrameIndex() ==
               (int)(animations_.at("Aim").getFrameCount() * shotStrength_ + 0.5f) - 1;
    }

    void UpdateAimAccordingToShotStrength() {
        Animation& aimAnimation = animations_.at("Aim");
        int frameToDisplay = (int)(aimAnimation.getFrameCount() * shotStrength_ + 0.5f);
        aimAnimation.setFrameIndex(frameToDisplay);
    }

    void StartFiringFromLastAimPosition() {
        int startFrame = animations_.at("Aim").getFrameCount() -
                         animations_.at("Aim").getFrameIndex();
        animations_.at("Fire").PlayFromFrameIndex(startFrame);
    }

    // Defined in Player.hpp (needs the full Player definition).
    bool CheckHit();
    int getEnemyScore() const;

    void DrawIdleCatapult() {
        spriteBatch_->Draw(*idleTexture_, catapultPosition_, std::nullopt, Color::White,
                          0.0f, Vector2::Zero, 1.0f, spriteEffects_, 0.0f);
    }

    static constexpr float gravity = 500.0f;
    static constexpr int winScore = 5;

    Game* game_;
    SpriteBatch* spriteBatch_;

    bool animationRunning_ = false;
    bool isActive_ = false;

    std::map<std::string, int> splitFrames_;
    std::optional<Texture2D> idleTexture_;
    std::map<std::string, Texture2D> animTextures_;
    std::map<std::string, Animation> animations_;

    SpriteEffects spriteEffects_;
    std::optional<Projectile> projectile_;

    std::string idleTextureName_;
    bool isAI_;

    CatapultState lastUpdateState_ = CatapultState::Idle;
    int stallUpdateCycles_ = 0;
    CatapultState currentState_ = CatapultState::Idle;

    float wind_ = 0.0f;
    Player* enemy_ = nullptr;
    Player* self_ = nullptr;

    Vector2 catapultPosition_;
    float shotStrength_ = 0.0f;
    float shotVelocity_ = 0.0f;
    bool gameOver_ = false;

    friend class Player;
};

} // namespace CatapultWars

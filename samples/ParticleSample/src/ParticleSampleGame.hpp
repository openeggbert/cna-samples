#pragma once
#include <cmath>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "System/Random.hpp"

namespace ParticleSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

// ─────────────────────────────────────────────────────────────────────────────
// Namespace-level random shared across all particle systems (mirrors XNA's
// static ParticleSampleGame.Random)
// ─────────────────────────────────────────────────────────────────────────────
inline System::Random gRandom;

inline float RandomBetween(float min, float max) {
    return min + (float)gRandom.NextDouble() * (max - min);
}

// ─────────────────────────────────────────────────────────────────────────────
// Particle
// ─────────────────────────────────────────────────────────────────────────────
struct Particle {
    Vector2 Position;
    Vector2 Velocity;
    Vector2 Acceleration;
    float Lifetime       = 0.0f;
    float TimeSinceStart = 0.0f;
    float Scale          = 1.0f;
    float Rotation       = 0.0f;
    float RotationSpeed  = 0.0f;

    bool Active() const { return TimeSinceStart < Lifetime; }

    void Initialize(Vector2 pos, Vector2 vel, Vector2 accel,
                    float lifetime, float scale, float rotSpeed) {
        Position       = pos;
        Velocity       = vel;
        Acceleration   = accel;
        Lifetime       = lifetime;
        Scale          = scale;
        RotationSpeed  = rotSpeed;
        TimeSinceStart = 0.0f;
        Rotation       = RandomBetween(0.0f, MathHelper::TwoPi);
    }

    void Update(float dt) {
        Velocity     = Velocity     + Acceleration * dt;
        Position     = Position     + Velocity     * dt;
        Rotation    += RotationSpeed * dt;
        TimeSinceStart += dt;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ParticleSystem  (abstract base — replaces DrawableGameComponent)
// ─────────────────────────────────────────────────────────────────────────────
class ParticleSystem {
public:
    static constexpr int AlphaBlendDrawOrder = 100;
    static constexpr int AdditiveDrawOrder   = 200;

    int DrawOrder = AlphaBlendDrawOrder;
    int FreeParticleCount() const { return (int)freeParticles_.size(); }

    // Subclass constructors must call InitializeConstants() themselves (setting
    // maxNumParticles_ etc.), then call Allocate() to finish setup.
    ParticleSystem() = default;

    void Allocate(int howManyEffects, Texture2D texture) {
        howManyEffects_ = howManyEffects;
        texture_        = texture;
        origin_         = Vector2((float)(texture_.getWidthProperty()  / 2),
                                  (float)(texture_.getHeightProperty() / 2));
        int total = howManyEffects_ * maxNumParticles_;
        particles_.resize(total);
        for (int i = 0; i < total; ++i)
            freeParticles_.push(&particles_[i]);
    }

    virtual ~ParticleSystem() = default;

    void AddParticles(Vector2 where) {
        int count = gRandom.Next(minNumParticles_, maxNumParticles_);
        for (int i = 0; i < count && !freeParticles_.empty(); ++i) {
            Particle* p = freeParticles_.front();
            freeParticles_.pop();
            InitializeParticle(*p, where);
        }
    }

    void Update(float dt) {
        for (auto& p : particles_) {
            if (!p.Active()) continue;
            p.Update(dt);
            if (!p.Active()) freeParticles_.push(&p);
        }
    }

    void Draw(SpriteBatch& sb) {
        for (auto& p : particles_) {
            if (!p.Active()) continue;
            float t     = p.TimeSinceStart / p.Lifetime;
            float alpha = 4.0f * t * (1.0f - t);
            int   a     = (int)(255.0f * alpha);
            Color color(255, 255, 255, a);
            float scale = p.Scale * (0.75f + 0.25f * t);
            sb.Draw(texture_, p.Position, std::nullopt, color,
                    p.Rotation, origin_, scale, SpriteEffects::None, 0.0f);
        }
    }

protected:
    virtual void InitializeConstants() = 0;

    virtual void InitializeParticle(Particle& p, Vector2 where) {
        Vector2 dir = PickRandomDirection();
        float speed  = RandomBetween(minInitialSpeed_,  maxInitialSpeed_);
        float accel  = RandomBetween(minAcceleration_,  maxAcceleration_);
        float life   = RandomBetween(minLifetime_,       maxLifetime_);
        float scale  = RandomBetween(minScale_,          maxScale_);
        float rot    = RandomBetween(minRotationSpeed_,  maxRotationSpeed_);
        p.Initialize(where, dir * speed, dir * accel, life, scale, rot);
    }

    virtual Vector2 PickRandomDirection() {
        float angle = RandomBetween(0.0f, MathHelper::TwoPi);
        return Vector2(std::cos(angle), std::sin(angle));
    }

    // Constants to be set by subclasses in InitializeConstants()
    int   minNumParticles_  = 0;
    int   maxNumParticles_  = 1;
    float minInitialSpeed_  = 0.0f;
    float maxInitialSpeed_  = 0.0f;
    float minAcceleration_  = 0.0f;
    float maxAcceleration_  = 0.0f;
    float minLifetime_      = 1.0f;
    float maxLifetime_      = 1.0f;
    float minScale_         = 1.0f;
    float maxScale_         = 1.0f;
    float minRotationSpeed_ = 0.0f;
    float maxRotationSpeed_ = 0.0f;

private:
    int                   howManyEffects_;
    Texture2D             texture_;
    Vector2               origin_;
    std::vector<Particle> particles_;
    std::queue<Particle*> freeParticles_;
};

// ─────────────────────────────────────────────────────────────────────────────
// ExplosionParticleSystem
// ─────────────────────────────────────────────────────────────────────────────
class ExplosionParticleSystem : public ParticleSystem {
public:
    ExplosionParticleSystem(int howManyEffects, Texture2D tex) {
        InitializeConstants();
        Allocate(howManyEffects, tex);
    }

protected:
    void InitializeConstants() override {
        minInitialSpeed_  = 40.0f;  maxInitialSpeed_  = 500.0f;
        minAcceleration_  = 0.0f;   maxAcceleration_  = 0.0f;
        minLifetime_      = 0.5f;   maxLifetime_      = 1.0f;
        minScale_         = 0.3f;   maxScale_         = 1.0f;
        minNumParticles_  = 20;     maxNumParticles_  = 25;
        minRotationSpeed_ = -MathHelper::PiOver4;
        maxRotationSpeed_ =  MathHelper::PiOver4;
        DrawOrder = AdditiveDrawOrder;
    }

    void InitializeParticle(Particle& p, Vector2 where) override {
        ParticleSystem::InitializeParticle(p, where);
        // Decelerate to zero: vt = v0 + a*t → 0 = v0 + a*life → a = -v0/life
        p.Acceleration = p.Velocity * (-1.0f / p.Lifetime);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ExplosionSmokeParticleSystem
// ─────────────────────────────────────────────────────────────────────────────
class ExplosionSmokeParticleSystem : public ParticleSystem {
public:
    ExplosionSmokeParticleSystem(int howManyEffects, Texture2D tex) {
        InitializeConstants();
        Allocate(howManyEffects, tex);
    }

protected:
    void InitializeConstants() override {
        minInitialSpeed_  = 20.0f;   maxInitialSpeed_  = 200.0f;
        minAcceleration_  = -10.0f;  maxAcceleration_  = -50.0f;
        minLifetime_      = 1.0f;    maxLifetime_      = 2.5f;
        minScale_         = 1.0f;    maxScale_         = 2.0f;
        minNumParticles_  = 10;      maxNumParticles_  = 20;
        minRotationSpeed_ = -MathHelper::PiOver4;
        maxRotationSpeed_ =  MathHelper::PiOver4;
        DrawOrder = AlphaBlendDrawOrder;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// SmokePlumeParticleSystem
// ─────────────────────────────────────────────────────────────────────────────
class SmokePlumeParticleSystem : public ParticleSystem {
public:
    SmokePlumeParticleSystem(int howManyEffects, Texture2D tex) {
        InitializeConstants();
        Allocate(howManyEffects, tex);
    }

protected:
    void InitializeConstants() override {
        minInitialSpeed_  = 20.0f;  maxInitialSpeed_  = 100.0f;
        minAcceleration_  = 0.0f;   maxAcceleration_  = 0.0f;
        minLifetime_      = 5.0f;   maxLifetime_      = 7.0f;
        minScale_         = 0.5f;   maxScale_         = 1.0f;
        minNumParticles_  = 7;      maxNumParticles_  = 15;
        minRotationSpeed_ = -MathHelper::PiOver4 / 2.0f;
        maxRotationSpeed_ =  MathHelper::PiOver4 / 2.0f;
        DrawOrder = AlphaBlendDrawOrder;
    }

    Vector2 PickRandomDirection() override {
        float radians = RandomBetween(MathHelper::ToRadians(80.0f),
                                      MathHelper::ToRadians(100.0f));
        return Vector2(std::cos(radians), -std::sin(radians));
    }

    void InitializeParticle(Particle& p, Vector2 where) override {
        ParticleSystem::InitializeParticle(p, where);
        p.Acceleration.X = p.Acceleration.X + RandomBetween(10.0f, 50.0f);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ParticleSampleGame
// ─────────────────────────────────────────────────────────────────────────────
class ParticleSampleGame : public Microsoft::Xna::Framework::Game {
    enum class State { Explosions, SmokePlume };
    static constexpr int   NumStates              = 2;
    static constexpr float TimeBetweenExplosions  = 2.0f;
    static constexpr float TimeBetweenSmokePuffs  = 0.5f;

    GraphicsDeviceManager        graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont>    font_;

    std::optional<Texture2D> explosionTex_;
    std::optional<Texture2D> smokeTex_;

    std::unique_ptr<ExplosionParticleSystem>      explosion_;
    std::unique_ptr<ExplosionSmokeParticleSystem> smoke_;
    std::unique_ptr<SmokePlumeParticleSystem>     smokePlume_;

    State currentState_       = State::Explosions;
    float timeTillExplosion_  = 0.0f;
    float timeTillPuff_       = 0.0f;

    KeyboardState lastKeyboardState_;
    GamePadState  lastGamePadState_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "ParticleSampleGame";
        return name;
    }

    ParticleSampleGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");
    }

protected:
    void LoadContent() override {
        spriteBatch_  = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        font_.emplace(getContentProperty().Load<SpriteFont>("font"));

        explosionTex_.emplace(getContentProperty().Load<Texture2D>("explosion"));
        smokeTex_.emplace(getContentProperty().Load<Texture2D>("smoke"));
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        explosion_  = std::make_unique<ExplosionParticleSystem>(1, *explosionTex_);
        smoke_      = std::make_unique<ExplosionSmokeParticleSystem>(2, *smokeTex_);
        smokePlume_ = std::make_unique<SmokePlumeParticleSystem>(9, *smokeTex_);
    }

    void Update(GameTime& gameTime) override {
        float dt = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= dt;

        HandleInput();

        switch (currentState_) {
            case State::Explosions: UpdateExplosions(dt); break;
            case State::SmokePlume: UpdateSmokePlume(dt); break;
        }

        explosion_->Update(dt);
        smoke_->Update(dt);
        smokePlume_->Update(dt);

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(0, 0, 0, 255));

        spriteBatch_->Begin();

        // Draw alpha-blended systems first (DrawOrder 100), then additive (200)
        smoke_->Draw(*spriteBatch_);
        smokePlume_->Draw(*spriteBatch_);
        explosion_->Draw(*spriteBatch_);

        // Status text
        std::string stateName = (currentState_ == State::Explosions) ? "Explosions" : "SmokePlume";
        std::string line1 = "Current effect: " + stateName + "!";
        std::string line2 = "Hit Space or A button to switch.";
        std::string line3 = "Free particles: exp=" + std::to_string(explosion_->FreeParticleCount())
                          + "  smoke=" + std::to_string(smoke_->FreeParticleCount())
                          + "  plume=" + std::to_string(smokePlume_->FreeParticleCount());
        spriteBatch_->DrawString(*font_, line1, Vector2(50.0f, 50.0f), Color(255, 255, 255, 255));
        spriteBatch_->DrawString(*font_, line2, Vector2(50.0f, 68.0f), Color(255, 255, 255, 255));
        spriteBatch_->DrawString(*font_, line3, Vector2(50.0f, 86.0f), Color(255, 255, 255, 255));

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty()  - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();
        Game::Draw(gameTime);
    }

private:
    void UpdateExplosions(float dt) {
        timeTillExplosion_ -= dt;
        if (timeTillExplosion_ < 0.0f) {
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            Vector2 where(RandomBetween(0.0f, (float)vp.getWidthProperty()),
                          RandomBetween(0.0f, (float)vp.getHeightProperty()));
            explosion_->AddParticles(where);
            smoke_->AddParticles(where);
            timeTillExplosion_ = TimeBetweenExplosions;
        }
    }

    void UpdateSmokePlume(float dt) {
        timeTillPuff_ -= dt;
        if (timeTillPuff_ < 0.0f) {
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            Vector2 where((float)(vp.getWidthProperty() / 2),
                          (float)vp.getHeightProperty());
            smokePlume_->AddParticles(where);
            timeTillPuff_ = TimeBetweenSmokePuffs;
        }
    }

    void HandleInput() {
        KeyboardState kb  = Keyboard::GetState();
        GamePadState  pad = GamePad::GetState(PlayerIndex::One);

        if (kb.IsKeyDown(Keys::Escape) || pad.IsButtonDown(Buttons::Back))
            Exit();

        bool spaceReleased = !kb.IsKeyDown(Keys::Space) && lastKeyboardState_.IsKeyDown(Keys::Space);
        bool aPressed      = pad.IsButtonDown(Buttons::A) && !lastGamePadState_.IsButtonDown(Buttons::A);

        if (spaceReleased || aPressed)
            currentState_ = (State)(((int)currentState_ + 1) % NumStates);

        lastKeyboardState_ = kb;
        lastGamePadState_  = pad;
    }
};

} // namespace ParticleSample

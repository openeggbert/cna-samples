#pragma once

// ParticleSampleGame.hpp — C++ port of ParticleSampleGame.cs (XNA 4.0
// Particles2DPipeline sample). Demos three particle effects (explosions,
// a smoke plume, and a mouse-driven particle emitter).

#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"

#include "ParticleEmitter.hpp"
#include "ParticleHelpers.hpp"
#include "ParticleSystem.hpp"

namespace Particles2DPipelineSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// Port of ParticleSampleGame.cs.
class ParticleSampleGame : public Microsoft::Xna::Framework::Game {
public:
    const std::string& GetTypeName() const override {
        static const std::string name = "ParticleSampleGame";
        return name;
    }

    ParticleSampleGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");

        // Enable the tap gesture for changing particle effects. On this
        // desktop (no touchscreen) TouchPanel never actually produces a
        // gesture, matching how the original behaves on real Windows
        // without a touch digitizer -- Space/A/Escape already cover
        // desktop input, exactly as in the original's own non-phone path.
        TouchPanel::setEnabledGesturesProperty(GestureType::Tap);
    }

protected:
    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        font_.emplace(getContentProperty().Load<SpriteFont>("font"));
        emitterSprite_.emplace(getContentProperty().Load<Texture2D>("BlockEmitter"));
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        explosionTexture_.emplace(getContentProperty().Load<Texture2D>("explosion"));
        smokeTexture_.emplace(getContentProperty().Load<Texture2D>("smoke"));
        blockParticleTexture_.emplace(getContentProperty().Load<Texture2D>("BlockParticle"));

        explosion_.emplace(getGraphicsDeviceProperty(), BuildExplosionSettings(), *explosionTexture_);
        explosion_->DrawOrder = ParticleSystem::AdditiveDrawOrder;

        smoke_.emplace(getGraphicsDeviceProperty(), BuildExplosionSmokeSettings(), *smokeTexture_);
        smoke_->DrawOrder = ParticleSystem::AlphaBlendDrawOrder;

        smokePlume_.emplace(getGraphicsDeviceProperty(), BuildSmokePlumeSettings(), *smokeTexture_);
        smokePlume_->DrawOrder = ParticleSystem::AlphaBlendDrawOrder;

        emitterSystem_.emplace(getGraphicsDeviceProperty(), BuildEmitterSettings(), *blockParticleTexture_);
        emitterSystem_->DrawOrder = ParticleSystem::AlphaBlendDrawOrder;
        emitter_.emplace(*emitterSystem_, 60.0f, Vector2(400.0f, 240.0f));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        float dt = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        switch (currentState_) {
            case State::Explosions:
                UpdateExplosions(dt);
                break;
            case State::SmokePlume:
                UpdateSmokePlume(dt);
                break;
            case State::Emitter:
                UpdateEmitter(gameTime);
                break;
        }

        explosion_->Update(dt);
        smoke_->Update(dt);
        smokePlume_->Update(dt);
        emitterSystem_->Update(dt);

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(0, 0, 0, 255));

        spriteBatch_->Begin();

        std::string stateName;
        switch (currentState_) {
            case State::Explosions: stateName = "Explosions"; break;
            case State::SmokePlume: stateName = "SmokePlume"; break;
            case State::Emitter:    stateName = "Emitter"; break;
        }

        std::string message = "Current effect: " + stateName + "!\n" +
            "Hit the A button or space bar, or tap the screen, to switch.\n\n" +
            "Free particles:\n" +
            "    ExplosionParticleSystem:      " + std::to_string(explosion_->FreeParticleCount()) + "\n" +
            "    ExplosionSmokeParticleSystem: " + std::to_string(smoke_->FreeParticleCount()) + "\n" +
            "    SmokePlumeParticleSystem:     " + std::to_string(smokePlume_->FreeParticleCount()) + "\n" +
            "    EmitterParticleSystem:        " + std::to_string(emitterSystem_->FreeParticleCount());
        spriteBatch_->DrawString(*font_, message, Vector2(50.0f, 50.0f), Color(255, 255, 255, 255));

        if (currentState_ == State::Emitter) {
            int w = emitterSprite_->getWidthProperty();
            int h = emitterSprite_->getHeightProperty();
            spriteBatch_->Draw(*emitterSprite_, emitter_->Position() - Vector2((float)(w / 2), (float)(h / 2)),
                               Color(255, 255, 255, 255));
        }

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty() - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();

        // Particle systems draw on top, in the original's DrawOrder-sorted
        // GameComponent order: AlphaBlend (100) systems first, in the order
        // they were added, then Additive (200) last.
        smoke_->Draw();
        smokePlume_->Draw();
        emitterSystem_->Draw();
        explosion_->Draw();

        Game::Draw(gameTime);
    }

private:
    enum class State { Explosions, SmokePlume, Emitter };
    static constexpr int NumStates = 3;

    static constexpr float TimeBetweenExplosions = 2.0f;
    static constexpr float TimeBetweenSmokePlumePuffs = 0.5f;

    void UpdateEmitter(GameTime& gameTime) {
        // The original's non-Xbox (Windows/Windows Phone) branch uses the
        // Mouse to position the emitter; ported as-is for this desktop
        // target.
        MouseState mouseState = Mouse::GetState();
        Vector2 newPosition((float)mouseState.getXProperty(), (float)mouseState.getYProperty());
        emitter_->Update(gameTime, newPosition);
    }

    void UpdateSmokePlume(float dt) {
        timeTillPuff_ -= dt;
        if (timeTillPuff_ < 0.0f) {
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            Vector2 where((float)(vp.getWidthProperty() / 2), (float)vp.getHeightProperty());
            smokePlume_->AddParticles(where, Vector2::Zero);
            timeTillPuff_ = TimeBetweenSmokePlumePuffs;
        }
    }

    void UpdateExplosions(float dt) {
        timeTillExplosion_ -= dt;
        if (timeTillExplosion_ < 0.0f) {
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            Vector2 where(ParticleHelpers::RandomBetween(0.0f, (float)vp.getWidthProperty()),
                          ParticleHelpers::RandomBetween(0.0f, (float)vp.getHeightProperty()));
            explosion_->AddParticles(where, Vector2::Zero);
            smoke_->AddParticles(where, Vector2::Zero);
            timeTillExplosion_ = TimeBetweenExplosions;
        }
    }

    void HandleInput() {
        KeyboardState currentKeyboardState = Keyboard::GetState();
        GamePadState currentGamePadState = GamePad::GetState(PlayerIndex::One);

        if (currentGamePadState.IsButtonDown(Buttons::Back) || currentKeyboardState.IsKeyDown(Keys::Escape))
            Exit();

        bool keyboardSpace =
            !currentKeyboardState.IsKeyDown(Keys::Space) && lastKeyboardState_.IsKeyDown(Keys::Space);

        bool gamepadA =
            currentGamePadState.IsButtonDown(Buttons::A) && !lastGamePadState_.IsButtonDown(Buttons::A);

        // Drain all available gestures even if a tap occurred, same as the
        // original.
        bool tapGesture = false;
        while (TouchPanel::getIsGestureAvailableProperty()) {
            GestureSample sample = TouchPanel::ReadGesture();
            if (sample.getGestureTypeProperty() == GestureType::Tap)
                tapGesture = true;
        }

        if (keyboardSpace || gamepadA || tapGesture)
            currentState_ = (State)(((int)currentState_ + 1) % NumStates);

        lastKeyboardState_ = currentKeyboardState;
        lastGamePadState_ = currentGamePadState;
    }

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> font_;

    std::optional<Texture2D> emitterSprite_;
    std::optional<Texture2D> helpTexture_;
    std::optional<Texture2D> explosionTexture_;
    std::optional<Texture2D> smokeTexture_;
    std::optional<Texture2D> blockParticleTexture_;

    std::optional<ParticleSystem> explosion_;
    std::optional<ParticleSystem> smoke_;
    std::optional<ParticleSystem> smokePlume_;
    std::optional<ParticleSystem> emitterSystem_;
    std::optional<ParticleEmitter> emitter_;

    State currentState_ = State::Explosions;

    float timeTillExplosion_ = 0.0f;
    float timeTillPuff_ = 0.0f;

    KeyboardState lastKeyboardState_;
    GamePadState lastGamePadState_;

    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

} // namespace Particles2DPipelineSample

#pragma once

#include <memory>
#include <optional>
#include <string>

#include "Accelerometer.hpp"

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/DisplayOrientation.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteSortMode.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/Viewport.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "System/TimeSpan.hpp"

namespace AccelerometerSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::DisplayOrientation;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::BlendState;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteSortMode;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Graphics::Viewport;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::PlayerIndex;

// Port of the XNA 4.0 "Accelerometer" Windows Phone sample: a simple example
// showing how to use the accelerometer to move a sprite (an asteroid) around
// the screen. See Accelerometer.hpp for the input-substitution story -- this
// port always takes the original's own emulator keyboard-arrow fallback,
// since there is no phone hardware (or emulator distinction to make) on
// desktop.
class AccelerometerSampleGame : public Game {
public:
    AccelerometerSampleGame() : graphics_(this) {
        graphics_.setSupportedOrientationsProperty(DisplayOrientation::Portrait);
        graphics_.setPreferredBackBufferWidthProperty(GraphicsWidth);
        graphics_.setPreferredBackBufferHeightProperty(GraphicsHeight);
        // graphics.IsFullScreen = true in the original -- omitted, see
        // missing.md (kept windowed, matching every other phone sample in
        // this repo).
        getContentProperty().setRootDirectoryProperty("Content");

        // Frame rate is 30 fps by default for Windows Phone.
        setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(333333));
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "AccelerometerSampleGame";
        return name;
    }

protected:
    void Initialize() override {
        // Initialize the Accelerometer.
        accelerometer_.Initialize();

        Game::Initialize();
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        auto& content = getContentProperty();

        // load the sprite's texture
        asteroidTexture_.emplace(content.Load<Texture2D>("asteroid"));

        // load the background texture
        backgroundTexture_.emplace(content.Load<Texture2D>("space"));

        // center the sprite on screen. The original reads
        // graphics.GraphicsDevice.Viewport.Width/Height here; CNA's
        // Viewport hasn't synced to the just-created window this early
        // (Game::Initialize() calls LoadContent() directly -- see
        // SoccerPitch/SnowShovel's own missing.md for the same gotcha), so
        // use the known preferred back-buffer size instead -- the exact
        // value requested in the constructor above.
        logoPosition_ = Vector2(
            (float)((GraphicsWidth  - asteroidTexture_->getWidthProperty())  / 2),
            (float)((GraphicsHeight - asteroidTexture_->getHeightProperty()) / 2));

        // F1 help overlay (CNA addition beyond the XNA original).
        helpTexture_.emplace(content.Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        // Allows the game to exit
        if (GamePad::GetState(PlayerIndex::One).IsButtonDown(Buttons::Back))
            Exit();
        // CNA addition (not in the original): Escape also exits, matching
        // the desktop-convenience pattern used across this repo's other
        // phone-sample ports (e.g. Bounce, Yacht) since a desktop keyboard
        // has no physical Back button.
        if (Keyboard::GetState().IsKeyDown(Keys::Escape))
            Exit();

        // poll the acceleration value
        Vector3 acceleration = accelerometer_.GetState().Acceleration;

        logoVelocity_.X += acceleration.X;
        logoVelocity_.Y += -acceleration.Y;

        logoPosition_ = logoPosition_ + logoVelocity_;

        // keep the sprite on the screen - clamp X
        const Viewport& viewport = getGraphicsDeviceProperty().getViewportProperty();

        if (logoPosition_.X < 0) {
            logoPosition_.X = 0;
            logoVelocity_.X = 0;
        } else if (logoPosition_.X > viewport.getWidthProperty() - asteroidTexture_->getWidthProperty()) {
            logoPosition_.X = (float)(viewport.getWidthProperty() - asteroidTexture_->getWidthProperty());
            logoVelocity_.X = 0;
        }

        // keep the sprite on the screen - clamp Y
        if (logoPosition_.Y < 0) {
            logoPosition_.Y = 0;
            logoVelocity_.Y = 0;
        } else if (logoPosition_.Y > viewport.getHeightProperty() - asteroidTexture_->getHeightProperty()) {
            logoPosition_.Y = (float)(viewport.getHeightProperty() - asteroidTexture_->getHeightProperty());
            logoVelocity_.Y = 0;
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color::White);

        spriteBatch_->Begin(SpriteSortMode::Immediate, BlendState::AlphaBlend);

        spriteBatch_->Draw(*backgroundTexture_, Vector2::Zero, Color::White);

        spriteBatch_->Draw(*asteroidTexture_, logoPosition_, Color::White);

        // F1 help overlay (CNA addition beyond the XNA original), drawn
        // last so it's on top of everything else.
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
    static constexpr int GraphicsWidth  = 480;
    static constexpr int GraphicsHeight = 800;

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;

    std::optional<Texture2D> asteroidTexture_;
    std::optional<Texture2D> backgroundTexture_;

    Vector2 logoPosition_;
    Vector2 logoVelocity_;

    Accelerometer accelerometer_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

} // namespace AccelerometerSample

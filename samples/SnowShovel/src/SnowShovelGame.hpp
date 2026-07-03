#pragma once

#include <cmath>
#include <iomanip>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/DisplayOrientation.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Audio/SoundEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteSortMode.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchCollection.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocationState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/Random.hpp"
#include "System/TimeSpan.hpp"

#include "Accelerometer.hpp"
#include "Snowflake.hpp"

namespace SnowShovel {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::DisplayOrientation;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Audio::SoundEffect;
using Microsoft::Xna::Framework::Graphics::BlendState;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::SpriteSortMode;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::Touch::TouchCollection;
using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// SnowShovel: tilt (or the arrow keys) to slide a shovel around and scoop up
// snowflakes before the clock runs out. Each wave adds more snowflakes and a
// shorter bonus timer. Port of the XNA 4.0 "SnowShovel" Windows Phone sample.
class SnowShovelGame : public Game {
public:
    SnowShovelGame() : graphics_(this) {
        graphics_.setSupportedOrientationsProperty(DisplayOrientation::Portrait);
        graphics_.setPreferredBackBufferWidthProperty(GraphicsWidth);
        graphics_.setPreferredBackBufferHeightProperty(GraphicsHeight);
        getContentProperty().setRootDirectoryProperty("Content");
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "SnowShovelGame";
        return name;
    }

protected:
    void Initialize() override {
        TouchPanel::setDisplayWidthProperty(GraphicsWidth);
        TouchPanel::setDisplayHeightProperty(GraphicsHeight);
        TouchPanel::setDisplayOrientationProperty(DisplayOrientation::Portrait);

        accelerometer_.Initialize();

        // The original art was designed for Zune HD, so we keep art & gameplay
        // at that native resolution and scale up for the (larger) window.
        //
        // The original queries GraphicsDevice.Viewport.Width/Height here, which
        // is safe on the phone (there's no windowing system to race against).
        // On this CNA/SDL3 port, getViewportProperty() hasn't synced to the
        // just-created window yet this early (Initialize() runs immediately
        // after GraphicsDeviceManager::CreateDevice(), before the first SDL
        // event pump) and returns a stale/wrong size; see missing.md. Since we
        // requested this exact back buffer size ourselves two lines above in
        // the constructor, use those known constants instead.
        worldRect_ = Rectangle(0, 0, WorldWidth, WorldHeight);

        worldToScreenMatrix_ = Matrix::CreateScale(
            (float)GraphicsWidth / (float)worldRect_.Width,
            (float)GraphicsHeight / (float)worldRect_.Height,
            1.0f);

        SetGameState(GameState::PreGame);

        Game::Initialize();
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        auto& content = getContentProperty();
        shovelTexture_.emplace(content.Load<Texture2D>("Images/shovel"));
        snowTexture_.emplace(content.Load<Texture2D>("Images/snowflakes"));

        titleFont_.emplace(content.Load<SpriteFont>("Fonts/TitleFont"));
        scoreFont_.emplace(content.Load<SpriteFont>("Fonts/ScoreFont"));

        sound_.emplace(content.Load<SoundEffect>("Sounds/plink"));

        // F1 help overlay (CNA addition beyond the XNA original).
        helpTexture_.emplace(content.Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        touchCollection_ = TouchPanel::GetState();
        gamepadState_ = GamePad::GetState(PlayerIndex::One);
        keyboardState_ = Keyboard::GetState();
        prevMouseState_ = mouseState_;
        mouseState_ = Mouse::GetState();

        // Allows the game to exit from any state
        if (gamepadState_.IsButtonDown(Buttons::Back) || keyboardState_.IsKeyDown(Keys::Escape)) {
            Exit();
        }

        if (timeRemaining_.getTotalMillisecondsProperty() > 0) {
            timeElapsed_ = timeElapsed_ + gameTime.getElapsedGameTimeProperty();
            timeRemaining_ = timeRemaining_ - gameTime.getElapsedGameTimeProperty();
        }

        // all the game states can have snowflakes, so do the update here
        for (Snowflake& snowflake : snowFlakes_) {
            snowflake.Update(worldRect_);
        }

        switch (gameState_) {
            case GameState::PreGame:
                UpdatePreGame();
                break;
            case GameState::PostGame:
                UpdatePostGame();
                break;
            case GameState::Game:
                UpdateGame(gameTime);
                break;
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color::Black);

        spriteBatch_->Begin(
            SpriteSortMode::Immediate,
            BlendState::NonPremultiplied,
            nullptr,
            nullptr,
            nullptr,
            nullptr,
            worldToScreenMatrix_);

        // draw my shovel
        spriteBatch_->Draw(*shovelTexture_,
            shovelPosition_,
            std::nullopt,
            Color::White,
            shovelRotation_,
            Vector2((float)shovelTexture_->getWidthProperty() / 2.0f, (float)shovelTexture_->getHeightProperty() / 2.0f),
            1.0f,
            SpriteEffects::None,
            0.0f);

        // and all the snow
        for (const Snowflake& snowflake : snowFlakes_) {
            int frameSize = snowTexture_->getHeightProperty();
            spriteBatch_->Draw(*snowTexture_,
                snowflake.Position,
                Rectangle(snowflake.TextureIndex * frameSize, 0, frameSize, frameSize),
                snowflake.Tint,
                snowflake.Rotation,
                Vector2((float)frameSize / 2.0f, (float)frameSize / 2.0f),
                snowflake.Scale,
                SpriteEffects::None,
                0.0f);
        }

        // draw the text
        DrawStringHelper(*titleFont_, "Snow Shovel", 0, 0, Color::Wheat);

        std::string timeRemainingString = "Time: " + FormatMinutesSeconds(timeRemaining_);

        // the time remaining text gets more red the closer to zero it is
        Color timeColor = Color::White;
        double totalMs = timeRemaining_.getTotalMillisecondsProperty();
        if (totalMs < 10000) {
            timeColor.setBProperty((SharpRuntime::bytecs)(255 * totalMs / 10000));
            timeColor.setGProperty((SharpRuntime::bytecs)(255 * totalMs / 10000));
        }

        DrawStringHelper(*scoreFont_, timeRemainingString, 0, 30, timeColor);

        std::string scoreString = "Score: " + std::to_string(score_);
        Vector2 stringDimensions = scoreFont_->MeasureString(scoreString);
        DrawStringHelper(*scoreFont_, scoreString, (int)(worldRect_.Width - stringDimensions.X), 30, Color::White);

        std::string timeElapsedString = "Elapsed Time: " + FormatMinutesSeconds(timeElapsed_);
        stringDimensions = scoreFont_->MeasureString(timeElapsedString);
        DrawStringHelper(*scoreFont_, timeElapsedString,
            (int)(worldRect_.Width - stringDimensions.X) / 2,
            worldRect_.Height - (int)stringDimensions.Y,
            Color::White);

        switch (gameState_) {
            case GameState::PreGame: {
                static const std::string instructionStrings[] = {
                    "Shovel snow before", "time runs out!", "Tap screen to start"};
                for (int i = 0; i < 3; ++i) {
                    stringDimensions = scoreFont_->MeasureString(instructionStrings[i]);
                    DrawStringHelper(*scoreFont_, instructionStrings[i],
                        (int)(worldRect_.Width - stringDimensions.X) / 2, 100 + i * 30, Color::White);
                }
                break;
            }
            case GameState::PostGame: {
                static const std::string instructionStrings[] = {
                    "GAME OVER", "Old Man Winter has Pwned you", "Tap screen to restart"};
                for (int i = 0; i < 3; ++i) {
                    stringDimensions = scoreFont_->MeasureString(instructionStrings[i]);
                    DrawStringHelper(*scoreFont_, instructionStrings[i],
                        (int)(worldRect_.Width - stringDimensions.X) / 2, 100 + i * 30, Color::Aqua);
                }
                break;
            }
            default:
                break;
        }

        spriteBatch_->End();

        // F1 help overlay (CNA addition beyond the XNA original): drawn in a
        // second, screen-space (identity transform) batch, since the overlay
        // is sized/positioned in real viewport pixels, not world units. The
        // default EasyGL backend composites multiple Begin/End per frame
        // correctly (a second Begin/End only loses data on the Vulkan
        // backend -- see NEXT.md section 5).
        if (helpTimer_ > 0.0f) {
            spriteBatch_->Begin();
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty() - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
            spriteBatch_->End();
        }

        Game::Draw(gameTime);
    }

private:
    enum class GameState { PreGame, Game, PostGame };

    static constexpr int GraphicsWidth = 480;
    static constexpr int GraphicsHeight = 800;
    static constexpr int WorldWidth = 272;
    static constexpr int WorldHeight = 480;
    static constexpr int PointsPerSnowflake = 100;

    // Move from one game state to the next.
    void SetGameState(GameState state) {
        gameState_ = state;
        snowFlakes_.clear();

        switch (state) {
            case GameState::PreGame:
                score_ = 0;
                timeElapsed_ = System::TimeSpan::Zero;
                nextWaveMilliseconds_ = 10000;
                nextWaveSnowflakeCount_ = 5;
                shovelPosition_.X = (float)worldRect_.getCenterProperty().X;
                shovelPosition_.Y = (float)worldRect_.getCenterProperty().Y;
                shovelVelocity_.X = 0.0f;
                shovelVelocity_.Y = 0.0f;
                break;

            case GameState::Game:
                timeRemaining_ = System::TimeSpan::FromSeconds(10.0);
                break;

            case GameState::PostGame:
                timeRemaining_ = System::TimeSpan::Zero;
                break;
        }

        // every game state change is followed by a wave of snowflakes
        Snow(nextWaveSnowflakeCount_);
    }

    // handle the pre-game menu and start
    void UpdatePreGame() {
        if (touchCollection_.getCountProperty() > 0 ||
            gamepadState_.IsButtonDown(Buttons::A) ||
            keyboardState_.IsKeyDown(Keys::Space) ||
            mouseState_.getLeftButtonProperty() == ButtonState::Pressed) {
            SetGameState(GameState::Game);
        }
    }

    // show the score, end the game
    void UpdatePostGame() {
        if ((touchCollection_.getCountProperty() > 0 &&
             touchCollection_[0].getStateProperty() == TouchLocationState::Pressed) ||
            gamepadState_.IsButtonDown(Buttons::A) ||
            keyboardState_.IsKeyDown(Keys::Space) ||
            MouseLeftJustPressed()) {
            SetGameState(GameState::PreGame);
        }
    }

    // A CNA addition (see missing.md): the original only reads Touch/Gamepad/Keyboard,
    // never Mouse. Mirrors TouchLocationState::Pressed's "just made contact" edge, so
    // holding the button from starting the game can't immediately re-trigger a restart.
    bool MouseLeftJustPressed() const {
        return mouseState_.getLeftButtonProperty() == ButtonState::Pressed &&
               prevMouseState_.getLeftButtonProperty() != ButtonState::Pressed;
    }

    // game play
    void UpdateGame(GameTime& gameTime) {
        if (timeRemaining_ < System::TimeSpan::Zero) {
            // clean up and exit
            SetGameState(GameState::PostGame);
            return;
        }

        if (snowFlakes_.empty()) {
            // next wave! each wave gets additional snowflakes and a shorter
            // bonus time, down to a minimum of half a second
            nextWaveSnowflakeCount_ += 5;

            if (nextWaveMilliseconds_ > 500) {
                nextWaveMilliseconds_ -= 500;
            }

            timeRemaining_ = timeRemaining_ + System::TimeSpan::FromMilliseconds(nextWaveMilliseconds_);

            Snow(nextWaveSnowflakeCount_);
        }

        // handle input for keyboard, accelerometer, or gamepad
        float pixelSeconds = 10.0f * (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        float accelX = gamepadState_.getThumbSticksProperty().getLeftProperty().X;
        float accelY = -gamepadState_.getThumbSticksProperty().getLeftProperty().Y;

        if (touchCollection_.getCountProperty() > 0) {
            Vector2 shovelPositionScreen = Vector2::Transform(shovelPosition_, worldToScreenMatrix_);
            Vector2 touchDifference = touchCollection_[0].getPositionProperty() - shovelPositionScreen;
            auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
            accelX = MathHelper::Clamp(touchDifference.X / (float)viewport.getWidthProperty() * 2.0f, -1.0f, 1.0f);
            accelY = MathHelper::Clamp(touchDifference.Y / (float)viewport.getHeightProperty() * 2.0f, -1.0f, 1.0f);
        } else if (mouseState_.getLeftButtonProperty() == ButtonState::Pressed) {
            // CNA addition (see missing.md): the original never reads Mouse, only
            // Touch/Gamepad/Keyboard. Click-and-drag mirrors "touch and drag" on a
            // machine with no touchscreen -- same relative-offset calc as touch above.
            Vector2 shovelPositionScreen = Vector2::Transform(shovelPosition_, worldToScreenMatrix_);
            Vector2 mousePosition((float)mouseState_.getXProperty(), (float)mouseState_.getYProperty());
            Vector2 mouseDifference = mousePosition - shovelPositionScreen;
            auto& viewport = getGraphicsDeviceProperty().getViewportProperty();
            accelX = MathHelper::Clamp(mouseDifference.X / (float)viewport.getWidthProperty() * 2.0f, -1.0f, 1.0f);
            accelY = MathHelper::Clamp(mouseDifference.Y / (float)viewport.getHeightProperty() * 2.0f, -1.0f, 1.0f);
        }

        if (keyboardState_.IsKeyDown(Keys::Left)) {
            accelX = -1.0f;
        }
        if (keyboardState_.IsKeyDown(Keys::Right)) {
            accelX = 1.0f;
        }
        if (keyboardState_.IsKeyDown(Keys::Up)) {
            accelY = -1.0f;
        }
        if (keyboardState_.IsKeyDown(Keys::Down)) {
            accelY = 1.0f;
        }

        // Real accelerometer, when present, overrides gamepad/touch/keyboard --
        // exactly like the original's `#if WINDOWS_PHONE` block did (see
        // Accelerometer.hpp).
        if (auto tilt = accelerometer_.GetTilt()) {
            accelX = tilt->X;
            accelY = -tilt->Y; // need to flip the y-value
        }

        shovelVelocity_.X += accelX * pixelSeconds;
        shovelVelocity_.Y += accelY * pixelSeconds;

        shovelPosition_.X += shovelVelocity_.X;
        shovelPosition_.Y += shovelVelocity_.Y;

        // make the shovel scoop face the direction of acceleration, or the
        // last known direction if acceleration is zero
        if (accelX != 0.0f || accelY != 0.0f) {
            shovelRotation_ = MathHelper::TwoPi - std::atan2(accelX, accelY);
        }

        // clamp X position and velocity to the screen bounds.
        if (shovelPosition_.X < 0) {
            shovelVelocity_.X = 0.0f;
            shovelPosition_.X = 0.0f;
        } else if (shovelPosition_.X > worldRect_.Width) {
            shovelVelocity_.X = 0.0f;
            shovelPosition_.X = (float)worldRect_.Width;
        }

        // clamp y position and velocity to the screen bounds
        if (shovelPosition_.Y < 0) {
            shovelVelocity_.Y = 0.0f;
            shovelPosition_.Y = 0.0f;
        } else if (shovelPosition_.Y > worldRect_.Height) {
            shovelVelocity_.Y = 0.0f;
            shovelPosition_.Y = (float)worldRect_.Height;
        }

        // Ideally we'd take shovel rotation into account and remove a
        // snowflake when you hit it with the scoop end. But this is a quick
        // & cheap hit-test.
        Rectangle shovelRect(
            (int)shovelPosition_.X - shovelTexture_->getWidthProperty() / 2,
            (int)shovelPosition_.Y - shovelTexture_->getHeightProperty() / 2,
            shovelTexture_->getWidthProperty(),
            shovelTexture_->getHeightProperty());

        for (auto it = snowFlakes_.begin(); it != snowFlakes_.end();) {
            if (shovelRect.Contains((int)it->Position.X, (int)it->Position.Y)) {
                score_ += PointsPerSnowflake;
                sound_->Play();
                it = snowFlakes_.erase(it);
            } else {
                ++it;
            }
        }
    }

    // generate new snowflakes
    void Snow(int snowflakeCount) {
        for (int i = 0; i < snowflakeCount; i++) {
            snowFlakes_.emplace_back(
                rand_,
                // snow within bounds, please
                Vector2((float)rand_.NextDouble() * worldRect_.Width, (float)rand_.NextDouble() * worldRect_.Height),
                // start with a velocity of -1.0 to 1.0
                Vector2((float)rand_.NextDouble() * 2.0f - 1.0f, (float)rand_.NextDouble() * 2.0f - 1.0f),
                // MAGIC NUMBER there are 5 sprites in the snowflake sprite sheet
                rand_.Next(5));
        }
    }

    // draws text with 1-pixel drop shadow
    void DrawStringHelper(const SpriteFont& font, const std::string& text, int x, int y, Color color) {
        spriteBatch_->DrawString(font, text, Vector2((float)(x + 1), (float)(y + 1)), Color::Black);
        spriteBatch_->DrawString(font, text, Vector2((float)x, (float)y), color);
    }

    // Formats a TimeSpan as "MM:SS.T" -- e.g. String::Format's custom picture
    // format "{0:00}:{1:00.0}" in the original, which sharp-runtime's
    // String::Format doesn't support (only standard .NET specifiers like D/F/X
    // are implemented, not zero-padded custom patterns). Same workaround as
    // Platformer's `pad2` helper.
    static std::string FormatMinutesSeconds(const System::TimeSpan& span) {
        int minutes = span.getMinutesProperty();
        float seconds = (float)span.getSecondsProperty() + (float)span.getMillisecondsProperty() / 1000.0f;

        std::string m = (minutes < 10 ? "0" : "") + std::to_string(minutes);

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << seconds;
        std::string s = oss.str();
        if (s.find('.') < 2) s = "0" + s;

        return m + ":" + s;
    }

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;

    std::optional<Texture2D> shovelTexture_;
    std::optional<Texture2D> snowTexture_;
    std::optional<SpriteFont> titleFont_;
    std::optional<SpriteFont> scoreFont_;
    std::optional<SoundEffect> sound_;

    Rectangle worldRect_;
    Matrix worldToScreenMatrix_;

    Accelerometer accelerometer_;

    System::Random rand_;
    std::vector<Snowflake> snowFlakes_;

    System::TimeSpan timeRemaining_;
    System::TimeSpan timeElapsed_;

    int score_ = 0;
    int nextWaveSnowflakeCount_ = 5;
    int nextWaveMilliseconds_ = 10000;

    Vector2 shovelPosition_;
    Vector2 shovelVelocity_;
    float shovelRotation_ = 0.0f;

    GameState gameState_ = GameState::PreGame;

    TouchCollection touchCollection_;
    GamePadState gamepadState_;
    KeyboardState keyboardState_;
    MouseState mouseState_;
    MouseState prevMouseState_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

} // namespace SnowShovel

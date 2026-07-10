#pragma once

// GameplayScreen.hpp — C++ port of Screens/GameplayScreen.cs (XNA 4.0 MarbleMaze
// sample). See missing.md for the full account of deviations:
//   - No CalibrationScreen / double-tap accelerometer calibration: the C# original
//     only reaches that branch when `Microsoft.Devices.Environment.DeviceType ==
//     DeviceType.Device` (a real phone) -- dead code on this desktop port, so it
//     (and CalibrationScreen.cs itself) is not ported at all, not merely stubbed.
//   - Maze tilt input: Accelerometer.cs's own emulator fallback (arrow keys ->
//     a synthesized accelerometer vector) plus this screen's own
//     `DeviceType.Emulator` branch (accelerometer vector -> Rotation delta) are
//     algebraically combined into one direct keyboard -> Rotation mapping below --
//     provably the same net behavior, not an invented new control scheme (the
//     established DEFERRED.md item #15 pattern: un-#if the existing non-phone
//     fallback).
//   - 3D draw order: the original interleaves SpriteBatch.Begin()/text/3D-draws/
//     End() in one block (re-enabling DepthStencilState after SpriteBatch.Begin()
//     disables it). This port instead draws the 3D scene first, then opens one
//     SpriteBatch block for all HUD text after -- this repo's established
//     convention (see e.g. ChaseCameraGame.hpp) -- which sidesteps needing the
//     DepthStencilState dance at all, with an identical visual result (text still
//     draws on top of the 3D scene).
//   - FinishCurrentGame(): the original calls Guide.BeginShowKeyboardInput to let
//     the player type their name for a new high score; this port always names the
//     entry "Player" (documented simplification -- see missing.md).
//   - `public new bool IsActive` (C#) hiding the base GameScreen.IsActive()
//     property is ported as a same-named public field on GameplayScreen, which
//     hides GameScreen::IsActive() for ordinary (non-virtual) lookup exactly like
//     C#'s `new` -- PauseScreen reads/writes it the same way the original does.

#include <cstdio>
#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "System/TimeSpan.hpp"

#include "../ScreenManager/GameScreen.hpp"
#include "../ScreenManager/ScreenManager.hpp"
#include "../Objects/Camera.hpp"
#include "../Objects/Marble.hpp"
#include "../Objects/Maze.hpp"
#include "../Misc/AudioManager.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::DepthStencilState;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;
using System::TimeSpan;

class BackgroundScreen; // forward declaration
class PauseScreen;      // forward declaration -- PauseCurrentGame's body constructs
                         // one (deferred to Screens/ScreensGlue.hpp).
class HighScoreScreen;  // forward declaration -- FinishCurrentGame's body constructs
                         // one (deferred).

class GameplayScreen : public GameScreen {
public:
    // Hides GameScreen::IsActive() -- see file header comment.
    bool IsActive = true;

    GameplayScreen() {
        setTransitionOnTime(TimeSpan::FromSeconds(0.0));
        setTransitionOffTime(TimeSpan::FromSeconds(0.0));
    }

    void LoadContent() override { timeFont_ = Load<SpriteFont>("Fonts/MenuFont"); }

    // Loads the 3D scene assets. Called once, synchronously, right before this
    // screen becomes active (see LoadingAndInstructionScreen.hpp) -- the C#
    // original runs this on a background System.Threading.Thread while showing a
    // "Loading..." message; this port runs it synchronously instead (see
    // missing.md: CNA's EasyGL graphics resources are not safe to create off the
    // main/GL thread).
    void LoadAssets() {
        InitializeCamera();
        InitializeMaze();
        InitializeMarble();
    }

    void HandleInput(InputState& input) override {
        if (input.IsPauseGame(std::nullopt)) {
            if (!gameOver_)
                PauseCurrentGame();
            else
                FinishCurrentGame();
        }

        if (IsActive && !startScreen_) {
            if (!input.Gestures.empty()) {
                if (input.Gestures[0].getGestureTypeProperty() == GestureType::Tap) {
                    if (gameOver_) FinishCurrentGame();
                }
            }

            if (!gameOver_) {
                // Direct keyboard tilt -- see file header comment for the
                // derivation from Accelerometer.cs's emulator fallback.
                KeyboardState kb = Keyboard::GetState();
                float dx = 0.0f, dz = 0.0f;
                if (kb.IsKeyDown(Keys::Right)) dz -= angularVelocity_;
                if (kb.IsKeyDown(Keys::Left)) dz += angularVelocity_;
                if (kb.IsKeyDown(Keys::Up)) dx -= angularVelocity_;
                if (kb.IsKeyDown(Keys::Down)) dx += angularVelocity_;

                maze_->Rotation.X = MathHelper::Clamp(maze_->Rotation.X + dx, MathHelper::ToRadians(-30.0f),
                                                       MathHelper::ToRadians(30.0f));
                maze_->Rotation.Z = MathHelper::Clamp(maze_->Rotation.Z + dz, MathHelper::ToRadians(-30.0f),
                                                       MathHelper::ToRadians(30.0f));
            }
        }
    }

    void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override {
        if (IsActive && !gameOver_) {
            if (!startScreen_) {
                elapsedGameTime_ = elapsedGameTime_ + gameTime.getElapsedGameTimeProperty();
                CheckFallInPit();
                UpdateLastCheckpoint();
            }

            maze_->Update(gameTime);
            marble_->Update(gameTime);
            camera_->Update(gameTime);

            CheckGameFinish();

            GameScreen::Update(gameTime, otherScreenHasFocus, coveredByOtherScreen);
        }
        if (startScreen_) {
            if (startScreenTime_.getTicksProperty() > 0)
                startScreenTime_ = startScreenTime_ - gameTime.getElapsedGameTimeProperty();
            else
                startScreen_ = false;
        }
    }

    void Draw(const GameTime& gameTime) override {
        auto& device = GetScreenManager()->getGraphicsDeviceProperty();
        device.Clear(Color::Black);

        if (IsActive) {
            maze_->Draw(gameTime);
            marble_->Draw(gameTime);
        }

        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();
        spriteBatch.Begin();

        if (startScreen_) DrawStartGame();

        if (IsActive) {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%02d:%02d", (int)elapsedGameTime_.getMinutesProperty(),
                          (int)elapsedGameTime_.getSecondsProperty());
            spriteBatch.DrawString(*timeFont_, buf, Vector2(20, 20), Color::YellowGreen);
        }

        if (gameOver_) {
            AudioManager::StopSounds();
            DrawEndGame();
        }

        spriteBatch.End();
    }

    // Restart the game.
    void Restart() {
        marble_->Position = maze_->StartPosition;
        marble_->Velocity = Vector3::Zero;
        marble_->Acceleration = Vector3::Zero;
        maze_->Rotation = Vector3::Zero;
        IsActive = true;
        gameOver_ = false;
        elapsedGameTime_ = TimeSpan::Zero;
        startScreen_ = true;
        startScreenTime_ = TimeSpan::FromSeconds(4);
        lastCheckpointIndex_ = 0;
    }

private:
    void InitializeCamera() {
        camera_ = std::make_unique<Camera>(GetScreenManager()->getGameProperty(),
                                            GetScreenManager()->getGraphicsDeviceProperty());
        camera_->Initialize();
    }

    void InitializeMaze() {
        maze_ = std::make_unique<Maze>(GetScreenManager()->getGameProperty());
        maze_->Position = Vector3::Zero;
        maze_->CameraPtr = camera_.get();
        maze_->Initialize();

        lastCheckpointIndex_ = 0;
    }

    void InitializeMarble() {
        marble_ = std::make_unique<Marble>(GetScreenManager()->getGameProperty());
        marble_->Position = maze_->StartPosition;
        marble_->CameraPtr = camera_.get();
        marble_->MazePtr = maze_.get();
        marble_->Initialize();
    }

    // Update the last checkpoint to return to after falling in a pit. Mirrors the
    // C# original's linked-list walk from the current checkpoint forward,
    // stopping at the first one within range.
    void UpdateLastCheckpoint() {
        BoundingSphere marblePosition = marble_->BoundingSphereTransformed();

        for (std::size_t i = lastCheckpointIndex_ + 1; i < maze_->Checkpoints.size(); ++i) {
            if (std::abs(Vector3::Distance(marblePosition.Center, maze_->Checkpoints[i])) <= marblePosition.Radius * 3) {
                AudioManager::PlaySound("checkpoint");
                lastCheckpointIndex_ = i;
                return;
            }
        }
    }

    // If the marble falls in a pit, return it to the last checkpoint it passed.
    void CheckFallInPit() {
        if (marble_->Position.Y < -150) {
            marble_->Position = maze_->Checkpoints[lastCheckpointIndex_];
            maze_->Rotation = Vector3::Zero;
            marble_->Acceleration = Vector3::Zero;
            marble_->Velocity = Vector3::Zero;
        }
    }

    // Check if the game has ended.
    void CheckGameFinish() {
        BoundingSphere marblePosition = marble_->BoundingSphereTransformed();
        if (std::abs(Vector3::Distance(marblePosition.Center, maze_->End)) <= marblePosition.Radius * 3) {
            gameOver_ = true;
        }
    }

    // Deferred to Screens/ScreensGlue.hpp -- constructs HighScoreScreen (this
    // sample skips Guide.BeginShowKeyboardInput -- see missing.md -- and just
    // names the entry "Player").
    void FinishCurrentGame();

    // Deferred to Screens/ScreensGlue.hpp -- constructs BackgroundScreen+PauseScreen.
    void PauseCurrentGame();

    void DrawEndGame() {
        bool highScore = HighScoreIsInHighscores();
        std::string text = highScore ? "    You got a High Score!" : "          Game Over";
        text += "\nTouch the screen to continue";

        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        Vector2 size = timeFont_->MeasureString(text);
        Vector2 textPosition((float)((viewport.getWidthProperty() - size.X) / 2.0f),
                              (float)((viewport.getHeightProperty() - size.Y) / 2.0f));

        spriteBatch.DrawString(*timeFont_, text, textPosition, Color::White);
    }

    void DrawStartGame() {
        std::string text = (startScreenTime_.getSecondsProperty() == 0) ? "Go!"
                                                                         : std::to_string((int)startScreenTime_.getSecondsProperty());
        SpriteBatch& spriteBatch = GetScreenManager()->getSpriteBatch();
        auto& viewport = GetScreenManager()->getGraphicsDeviceProperty().getViewportProperty();
        Vector2 size = timeFont_->MeasureString(text);
        Vector2 textPosition((float)((viewport.getWidthProperty() - size.X) / 2.0f),
                              (float)((viewport.getHeightProperty() - size.Y) / 2.0f));

        spriteBatch.DrawString(*timeFont_, text, textPosition, Color::White);
    }

    // Small indirection so this header doesn't need HighScoreScreen's full type
    // (only forward-declared here) -- defined in Screens/ScreensGlue.hpp.
    bool HighScoreIsInHighscores() const;

    bool gameOver_ = false;
    bool startScreen_ = true;
    TimeSpan startScreenTime_ = TimeSpan::FromSeconds(4);
    TimeSpan elapsedGameTime_ = TimeSpan::Zero;
    const float angularVelocity_ = MathHelper::ToRadians(1.5f);
    std::size_t lastCheckpointIndex_ = 0;

    std::unique_ptr<Maze> maze_;
    std::unique_ptr<Marble> marble_;
    std::unique_ptr<Camera> camera_;
    std::optional<SpriteFont> timeFont_;
};

} // namespace MarbleMazeSample

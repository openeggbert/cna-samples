#pragma once

// Direct port of SimpleAnimation.cs -- sample showing how to apply simple
// program-controlled, rigid-body animation to a model rendered using CNA.

#include "Tank.hpp"

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"

#include <cmath>
#include <memory>
#include <optional>
#include <string>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace SimpleAnimation {

class SimpleAnimationGame : public Game {
public:
    SimpleAnimationGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(1280);
        graphics_->setPreferredBackBufferHeightProperty(720);
        getContentProperty().setRootDirectoryProperty("Content");
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "SimpleAnimationGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        tank_.Load(getContentProperty());

        // F1 help overlay (CNA addition, see CLAUDE.md).
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        float time = (float)gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();

        // Update the animation properties on the tank object. In a real game
        // you would probably take this data from user inputs or the physics
        // system, rather than just making everything rotate like this!

        tank_.WheelRotation  = time * 5;
        tank_.SteerRotation  = (float)std::sin(time * 0.75f) * 0.5f;
        tank_.TurretRotation = (float)std::sin(time * 0.333f) * 1.25f;
        tank_.CannonRotation = (float)std::sin(time * 0.25f) * 0.333f - 0.333f;
        tank_.HatchRotation  = MathHelper::Clamp((float)std::sin(time * 2) * 2, -1.0f, 0.0f);

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        GraphicsDevice& device = getGraphicsDeviceProperty();

        device.Clear(Color(169, 169, 169, 255)); // DarkGray

        // Calculate the camera matrices.
        float time = (float)gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();

        Matrix rotation = Matrix::CreateRotationY(time * 0.1f);

        Matrix view = Matrix::CreateLookAt(Vector3(1000, 500, 0),
                                            Vector3(0, 150, 0),
                                            Vector3::Up);

        Matrix projection = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::PiOver4,
            device.getViewportProperty().getAspectRatioProperty(),
            10.0f,
            10000.0f);

        // Draw the tank model.
        tank_.Draw(rotation, view, projection);

        DrawHelpOverlay();

        Game::Draw(gameTime);
    }

private:
    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::unique_ptr<SpriteBatch>           spriteBatch_;

    Tank tank_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    // Handles input for quitting the game.
    void HandleInput() {
        KeyboardState currentKeyboardState = Keyboard::GetState();
        GamePadState  currentGamePadState  = GamePad::GetState(PlayerIndex::One);

        // Check for exit.
        if (currentKeyboardState.IsKeyDown(Keys::Escape) ||
            currentGamePadState.IsButtonDown(Buttons::Back)) {
            Exit();
        }
    }

    void DrawHelpOverlay() {
        if (helpTimer_ <= 0.0f || !helpTexture_.has_value()) return;

        int hw = helpTexture_->getWidthProperty();
        int hh = helpTexture_->getHeightProperty();
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        float sx = (float)((vp.getWidthProperty()  - hw) / 2);
        float sy = (float)((vp.getHeightProperty() - hh) / 2);

        spriteBatch_->Begin();
        spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        spriteBatch_->End();
    }
};

} // namespace SimpleAnimation

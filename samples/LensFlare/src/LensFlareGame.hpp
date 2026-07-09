#pragma once

#include <memory>
#include <optional>
#include <string>

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/Model.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelMesh.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelEffectCollection.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/DirectionalLight.hpp>
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/GamePadState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/Buttons.hpp>
#include <Microsoft/Xna/Framework/PlayerIndex.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>

#include "LensFlareComponent.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace LensFlareSample {

// Sample showing how to implement a lensflare effect, using occlusion queries to
// hide the flares when the sun is hidden behind the landscape.
class LensFlareGame : public Game {
public:
    LensFlareGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);

        getContentProperty().setRootDirectoryProperty("Content");

        // Create and add the lensflare component.
        lensFlare_ = std::make_unique<LensFlareComponent>(*this);
        getComponentsProperty().Add(lensFlare_.get());
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "LensFlareGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        terrain_.emplace(getContentProperty().Load<Model>("terrain"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        UpdateCamera(gameTime);

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255)); // CornflowerBlue

        // Compute camera matrices.
        Matrix view = Matrix::CreateLookAt(cameraPosition_, cameraPosition_ + cameraFront_, Vector3::Up);

        float aspectRatio = getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty();

        Matrix projection = Matrix::CreatePerspectiveFieldOfView(MathHelper::PiOver4, aspectRatio,
                                                                  0.1f, 500.0f);

        // Draw the terrain.
        getGraphicsDeviceProperty().setRasterizerStateProperty(RasterizerState::CullNone);

        for (ModelMesh* mesh : terrain_->getMeshesProperty()) {
            for (Effect* effect : mesh->getEffectsPropertyMutable()) {
                auto* basicEffect = static_cast<BasicEffect*>(effect);

                basicEffect->setWorldProperty(Matrix::getIdentityProperty());
                basicEffect->setViewProperty(view);
                basicEffect->setProjectionProperty(projection);

                basicEffect->setLightingEnabledProperty(true);
                basicEffect->setDiffuseColorProperty(Vector3::One);
                basicEffect->setAmbientLightColorProperty(Vector3(0.5f, 0.5f, 0.5f));

                DirectionalLight& light0 = basicEffect->getDirectionalLight0Property();
                light0.setEnabledProperty(true);
                light0.setDiffuseColorProperty(Vector3::One);
                light0.setDirectionProperty(lensFlare_->LightDirection);

                basicEffect->setFogEnabledProperty(true);
                basicEffect->setFogStartProperty(200.0f);
                basicEffect->setFogEndProperty(500.0f);
                basicEffect->setFogColorProperty(Color::CornflowerBlue.ToVector3());
            }

            mesh->Draw();
        }

        // Tell the lensflare component where our camera is positioned.
        lensFlare_->View = view;
        lensFlare_->Projection = projection;

        Game::Draw(gameTime);

        DrawHelpOverlay();
    }

private:
    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::optional<SpriteBatch> spriteBatch_;
    std::optional<Model> terrain_;

    std::unique_ptr<LensFlareComponent> lensFlare_;

    KeyboardState currentKeyboardState_;
    GamePadState  currentGamePadState_;

    Vector3 cameraPosition_ = Vector3(-200, 30, 30);
    Vector3 cameraFront_    = Vector3(1, 0, 0);

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    // Handles input for quitting the game.
    void HandleInput() {
        currentKeyboardState_ = Keyboard::GetState();
        currentGamePadState_  = GamePad::GetState(PlayerIndex::One);

        if (currentKeyboardState_.IsKeyDown(Keys::Escape) ||
            currentGamePadState_.getButtonsProperty().getBackProperty() == ButtonState::Pressed) {
            Exit();
        }
    }

    // Handles camera input.
    void UpdateCamera(GameTime& gameTime) {
        float time = (float)gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty();

        // Check for input to rotate the camera.
        float pitch = -currentGamePadState_.getThumbSticksProperty().getRightProperty().Y * time * 0.001f;
        float turn  = -currentGamePadState_.getThumbSticksProperty().getRightProperty().X * time * 0.001f;

        if (currentKeyboardState_.IsKeyDown(Keys::Up))    pitch += time * 0.001f;
        if (currentKeyboardState_.IsKeyDown(Keys::Down))  pitch -= time * 0.001f;
        if (currentKeyboardState_.IsKeyDown(Keys::Left))  turn  += time * 0.001f;
        if (currentKeyboardState_.IsKeyDown(Keys::Right)) turn  -= time * 0.001f;

        Vector3 cameraRight = Vector3::Cross(Vector3::Up, cameraFront_);
        Vector3 flatFront   = Vector3::Cross(cameraRight, Vector3::Up);

        Matrix pitchMatrix = Matrix::CreateFromAxisAngle(cameraRight, pitch);
        Matrix turnMatrix  = Matrix::CreateFromAxisAngle(Vector3::Up, turn);

        Vector3 tiltedFront = Vector3::TransformNormal(cameraFront_, pitchMatrix * turnMatrix);

        // Check angle so we can't flip over.
        if (Vector3::Dot(tiltedFront, flatFront) > 0.001f) {
            cameraFront_ = Vector3::Normalize(tiltedFront);
        }

        // Check for input to move the camera around.
        if (currentKeyboardState_.IsKeyDown(Keys::W)) cameraPosition_ += cameraFront_ * time * 0.1f;
        if (currentKeyboardState_.IsKeyDown(Keys::S)) cameraPosition_ -= cameraFront_ * time * 0.1f;
        if (currentKeyboardState_.IsKeyDown(Keys::A)) cameraPosition_ += cameraRight * time * 0.1f;
        if (currentKeyboardState_.IsKeyDown(Keys::D)) cameraPosition_ -= cameraRight * time * 0.1f;

        cameraPosition_ += cameraFront_ *
                            currentGamePadState_.getThumbSticksProperty().getLeftProperty().Y * time * 0.1f;

        cameraPosition_ -= cameraRight *
                            currentGamePadState_.getThumbSticksProperty().getLeftProperty().X * time * 0.1f;

        if (currentGamePadState_.getButtonsProperty().getRightStickProperty() == ButtonState::Pressed ||
            currentKeyboardState_.IsKeyDown(Keys::R)) {
            cameraPosition_ = Vector3(-200, 30, 30);
            cameraFront_    = Vector3(1, 0, 0);
        }
    }

    void DrawHelpOverlay() {
        if (helpTimer_ <= 0.0f || !helpTexture_.has_value()) return;
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int hw = helpTexture_->getWidthProperty();
        int hh = helpTexture_->getHeightProperty();
        float sx = (float)((vp.getWidthProperty()  - hw) / 2);
        float sy = (float)((vp.getHeightProperty() - hh) / 2);
        spriteBatch_->Begin();
        spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        spriteBatch_->End();
    }
};

} // namespace LensFlareSample

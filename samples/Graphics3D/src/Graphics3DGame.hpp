#pragma once

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/GamePadState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>
#include <Microsoft/Xna/Framework/Input/MouseState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
#include <Microsoft/Xna/Framework/PlayerIndex.hpp>
#include <Microsoft/Xna/Framework/DisplayOrientation.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>
#include <Microsoft/Xna/Framework/Point.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/MathHelper.hpp>
#include <System/TimeSpan.hpp>

#include "Checkbox.hpp"
#include "Animation.hpp"
#include "Spaceship.hpp"

#include <array>
#include <memory>
#include <optional>
#include <string>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace Graphics3DSample {

// Ported from the original's touch-driven phone sample: FreeDrag/Pinch gestures
// and on-screen touch buttons. This desktop port substitutes mouse input
// throughout (left-drag rotates, wheel zooms, click toggles buttons) since
// there is no touchscreen — see missing.md.
class Graphics3DGame : public Game {
public:
    Graphics3DGame() {
        getContentProperty().setRootDirectoryProperty("Content");

        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setIsFullScreenProperty(false);
        graphics_->setSupportedOrientationsProperty(DisplayOrientation::LandscapeLeft |
                                                     DisplayOrientation::LandscapeRight);
        graphics_->ApplyChanges();
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "Graphics3DGame";
        return name;
    }

protected:
    void Initialize() override {
        Game::Initialize();

        DepthStencilState depthStencilState;
        depthStencilState.setDepthBufferEnableProperty(true);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(depthStencilState);

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();

        for (int n = 0; n < 3; ++n) {
            Rectangle rect(vp.getWidthProperty() - (n + 1) * (buttonWidth + buttonMargin),
                           buttonMargin, buttonWidth, buttonHeight);
            lightEnablingButtons_[n] = std::make_unique<Checkbox>(*this, "Buttons/lamp_60x60", rect, true);
            AddComponent(lightEnablingButtons_[n].get());
        }

        perpixelLightingButton_ = std::make_unique<Checkbox>(*this, "Buttons/perPixelLight_60x60",
            Rectangle(vp.getWidthProperty() - (buttonWidth + buttonMargin),
                      vp.getHeightProperty() - (buttonHeight + buttonMargin), buttonWidth, buttonHeight),
            false);
        AddComponent(perpixelLightingButton_.get());

        animationButton_ = std::make_unique<Checkbox>(*this, "Buttons/animation_60x60",
            Rectangle(buttonMargin, vp.getHeightProperty() - (buttonHeight + buttonMargin),
                      buttonWidth, buttonHeight),
            false);
        AddComponent(animationButton_.get());

        backgroundTextureEnablingButton_ = std::make_unique<Checkbox>(*this, "Buttons/textureOnOff",
            Rectangle(buttonMargin, buttonMargin, buttonWidth, buttonHeight), false);
        AddComponent(backgroundTextureEnablingButton_.get());

        spaceship_.Projection = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::ToRadians(cameraFOV_), vp.getAspectRatioProperty(), 10.0f, 20000.0f);
    }

    void LoadContent() override {
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        background_.emplace(getContentProperty().Load<Texture2D>("Textures/spaceBG"));

        Texture2D explosionTexture = getContentProperty().Load<Texture2D>("Textures/explosionStrip");
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        Point frameSize(128, 128);
        animationPosition_ = Vector2((float)(vp.getWidthProperty() / 2 - frameSize.X),
                                     (float)(vp.getHeightProperty() / 2 - frameSize.Y));
        animation_.emplace(std::move(explosionTexture), frameSize, Point(6, 1),
                           System::TimeSpan::FromSeconds(1.0 / 10.0));

        spaceship_.Load(getContentProperty());
        spaceship_.SetTexture(getContentProperty().Load<Texture2D>("Models/enemy"));

        Game::LoadContent();
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        if (Keyboard::GetState().IsKeyDown(Keys::Escape) ||
            GamePad::GetState(PlayerIndex::One).getButtonsProperty().getBackProperty() == ButtonState::Pressed) {
            Exit();
        }

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();

        spaceship_.Rotation = GetRotationMatrix();
        spaceship_.View = GetViewMatrix();
        spaceship_.Lights = { lightEnablingButtons_[0]->IsChecked(), lightEnablingButtons_[1]->IsChecked(),
                              lightEnablingButtons_[2]->IsChecked() };
        spaceship_.IsTextureEnabled = true;
        spaceship_.IsPerPixelLightingEnabled = perpixelLightingButton_->IsChecked();
        spaceship_.Projection = Matrix::CreatePerspectiveFieldOfView(
            MathHelper::ToRadians(cameraFOV_), vp.getAspectRatioProperty(), 10.0f, 20000.0f);

        if (animationButton_->IsChecked()) {
            animation_->Update(gameTime);
        }

        // Matches the original's own ordering: the checkboxes' own Update()
        // (which reads this frame's click) runs via base Game::Update() last,
        // so IsChecked() above reflects the previous frame's click state.
        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255)); // CornflowerBlue

        if (backgroundTextureEnablingButton_->IsChecked()) {
            spriteBatch_->Begin();
            spriteBatch_->Draw(*background_, Vector2::Zero, Color::White);
            spriteBatch_->End();
        }

        getGraphicsDeviceProperty().setBlendStateProperty(BlendState::Opaque);
        getGraphicsDeviceProperty().setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
        getGraphicsDeviceProperty().setDepthStencilStateProperty(DepthStencilState::Default);
        getGraphicsDeviceProperty().getSamplerStatesProperty()[0] = SamplerState::LinearWrap;

        spaceship_.Draw();

        if (animationButton_->IsChecked()) {
            spriteBatch_->Begin();
            animation_->Draw(*spriteBatch_, animationPosition_, 2.0f, SpriteEffects::None);
            spriteBatch_->End();
        }

        Game::Draw(gameTime);

        DrawHelpOverlay();
    }

private:
    static constexpr int buttonHeight = 70;
    static constexpr int buttonWidth = 70;
    static constexpr int buttonMargin = 15;

    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::optional<SpriteBatch> spriteBatch_;

    Spaceship spaceship_;

    std::array<std::unique_ptr<Checkbox>, 3> lightEnablingButtons_;
    std::unique_ptr<Checkbox> perpixelLightingButton_;
    std::unique_ptr<Checkbox> animationButton_;
    std::unique_ptr<Checkbox> backgroundTextureEnablingButton_;

    float cameraFOV_ = 45.0f; // Initial camera FOV (serves as a zoom level)
    float rotationXAmount_ = 0.0f;
    float rotationYAmount_ = 0.0f;

    // NOXNA — mouse-drag/wheel state used to substitute for the original's
    // FreeDrag/Pinch touch gestures (no touchscreen on this desktop).
    bool prevMouseDown_ = false;
    int prevMouseX_ = 0;
    int prevMouseY_ = 0;
    int prevScrollWheel_ = 0;

    std::optional<Texture2D> background_;

    std::optional<Animation> animation_;
    Vector2 animationPosition_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;

    // NOXNA workaround for a CNA lifecycle gap: Game::DoInitialize() only wires
    // up Components_'s ComponentAdded event *after* calling the user's own
    // Initialize() override (see cna's Game.cpp), so a component added to
    // Components from within Initialize() — same pattern this sample's C#
    // original uses in its own Initialize() — never gets its Initialize()/
    // LoadContent() called via that event, leaving its optionals unset and
    // crashing on first Draw(). Real XNA/FNA wires this subscription in the
    // Game constructor itself, before any Initialize() override can run, so
    // this pattern works there. Calling Initialize() explicitly here is safe
    // even if the framework is later fixed to call it too, since
    // DrawableGameComponent::Initialize() is itself guarded against
    // double-initialization. See missing.md.
    void AddComponent(Checkbox* component) {
        getComponentsProperty().Add(component);
        component->Initialize();
    }

    void HandleInput() {
        MouseState mouse = Mouse::GetState();

        bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        if (mouseDown && prevMouseDown_) {
            rotationXAmount_ += (float)(mouse.getXProperty() - prevMouseX_);
            rotationYAmount_ -= (float)(mouse.getYProperty() - prevMouseY_);
        }
        prevMouseDown_ = mouseDown;
        prevMouseX_ = mouse.getXProperty();
        prevMouseY_ = mouse.getYProperty();

        int scroll = mouse.getScrollWheelValueProperty();
        int wheelDelta = scroll - prevScrollWheel_;
        prevScrollWheel_ = scroll;
        if (wheelDelta != 0) {
            cameraFOV_ -= wheelDelta * 0.02f;
            cameraFOV_ = MathHelper::Clamp(cameraFOV_, 30.0f, 60.0f);
        }
    }

    Matrix GetRotationMatrix() {
        Matrix matrix = Matrix::CreateWorld(Vector3(0, 250, 0), Vector3::Forward, Vector3::Up) *
            Matrix::CreateFromYawPitchRoll(MathHelper::Pi + MathHelper::PiOver2 + rotationXAmount_ / 100.0f,
                                            rotationYAmount_ / 100.0f, 0.0f);
        return matrix;
    }

    Matrix GetViewMatrix() {
        return Matrix::CreateLookAt(Vector3(3500, 400, 0) + Vector3(0, 250, 0),
                                     Vector3(0, 250, 0), Vector3::Up);
    }

    void DrawHelpOverlay() {
        if (helpTimer_ <= 0.0f || !helpTexture_.has_value()) return;
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int hw = helpTexture_->getWidthProperty();
        int hh = helpTexture_->getHeightProperty();
        float sx = (float)((vp.getWidthProperty() - hw) / 2);
        float sy = (float)((vp.getHeightProperty() - hh) / 2);
        spriteBatch_->Begin();
        spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        spriteBatch_->End();
    }
};

} // namespace Graphics3DSample

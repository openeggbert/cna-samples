#pragma once

// Ported from ChaseCameraSample.ChaseCameraGame (ChaseCameraGame.cs). Demonstrates a
// spring-physics chase camera (ChaseCamera.hpp) following a ship (Ship.hpp) flying over a
// checkered ground plane. See missing.md for the full port account, including a third/fourth
// independent confirmation of DEFERRED.md item #26's ModelTypeReader vertex-corruption bug
// (both the ship and ground models, converted from Ship.fbx/Ground.x, are stride-32
// .model.json files and hit the exact same "renders nothing" symptom Content.Load<Model>
// already produces for every other affected sample in this repo -- worked around here with
// RawModel.hpp, the same NOXNA bypass shape InverseKinematics' CylinderModel.hpp and
// HeightmapCollision/GeneratedGeometry's Terrain.hpp already established).

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/DisplayOrientation.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteFont.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>
#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/KeyboardState.hpp>
#include <Microsoft/Xna/Framework/Input/GamePad.hpp>
#include <Microsoft/Xna/Framework/Input/GamePadState.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>
#include <Microsoft/Xna/Framework/Input/MouseState.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Input/Buttons.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>
#include <Microsoft/Xna/Framework/PlayerIndex.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>

#include "ChaseCamera.hpp"
#include "Ship.hpp"
#include "RawModel.hpp"

#include <memory>
#include <optional>
#include <string>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace ChaseCameraSample {

class ChaseCameraGame : public Game {
public:
    ChaseCameraGame() {
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        // NOXNA note: the C# original sets SupportedOrientations = Portrait here too, even
        // though its actual back buffer (below) is a landscape 853x480 -- this only matters
        // on Windows Phone (the #if WINDOWS_PHONE branch this sample also has, not ported --
        // desktop only, matching every other port in this repo); kept for source fidelity, it
        // has no effect on this desktop build's window.
        graphics_->setSupportedOrientationsProperty(DisplayOrientation::Portrait);

        getContentProperty().setRootDirectoryProperty("Content");
        setIsMouseVisibleProperty(true);

        graphics_->setPreferredBackBufferWidthProperty(853);
        graphics_->setPreferredBackBufferHeightProperty(480);

        // Create the chase camera
        camera_ = std::make_unique<ChaseCamera>();

        // Set the camera offsets
        camera_->DesiredPositionOffset = Vector3(0.0f, 2000.0f, 3500.0f);
        camera_->LookAtOffset = Vector3(0.0f, 150.0f, 0.0f);

        // Set camera perspective
        camera_->NearPlaneDistance = 10.0f;
        camera_->FarPlaneDistance = 100000.0f;

        // TODO: Set any other camera invariants here such as field of view
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "ChaseCameraGame";
        return name;
    }

protected:
    void Initialize() override {
        Game::Initialize();

        ship_ = std::make_unique<Ship>(getGraphicsDeviceProperty());

        // Set the camera aspect ratio. This must be done after the call to base.Initialize()
        // which will initialize the graphics device.
        camera_->AspectRatio = getGraphicsDeviceProperty().getViewportProperty().getAspectRatioProperty();

        // Perform an initial reset on the camera so that it starts at the resting position.
        // If we don't do this, the camera will start at the origin and race across the world
        // to get behind the chased object. This is performed here because the aspect ratio is
        // needed by Reset.
        UpdateCameraChaseTarget();
        camera_->Reset();
    }

    void LoadContent() override {
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        spriteFont_.emplace(getContentProperty().Load<SpriteFont>("gameFont"));

        shipModel_.emplace();
        shipModel_->Load(getContentProperty(), getGraphicsDeviceProperty(), "Ship_p1_wedge_geo1",
                          "ShipDiffuse");

        groundModel_.emplace();
        groundModel_->Load(getContentProperty(), getGraphicsDeviceProperty(), "Ground", "Checker");

        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        // F1 help overlay
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        lastKeyboardState_ = currentKeyboardState_;
        lastGamePadState_ = currentGamePadState_;
        lastMouseState_ = currentMouseState_;

        currentKeyboardState_ = Keyboard::GetState();
        currentGamePadState_ = GamePad::GetState(PlayerIndex::One);
        currentMouseState_ = Mouse::GetState();

        // Exit when the Escape key or Back button is pressed
        if (currentKeyboardState_.IsKeyDown(Keys::Escape) ||
            currentGamePadState_.getButtonsProperty().getBackProperty() == ButtonState::Pressed) {
            Exit();
        }

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        bool touchTopLeft = currentMouseState_.getLeftButtonProperty() == ButtonState::Pressed &&
            lastMouseState_.getLeftButtonProperty() != ButtonState::Pressed &&
            currentMouseState_.getXProperty() < vp.getWidthProperty() / 10 &&
            currentMouseState_.getYProperty() < vp.getHeightProperty() / 10;

        // Pressing the A button or key toggles the spring behavior on and off
        if ((lastKeyboardState_.IsKeyUp(Keys::A) && currentKeyboardState_.IsKeyDown(Keys::A)) ||
            (lastGamePadState_.getButtonsProperty().getAProperty() == ButtonState::Released &&
             currentGamePadState_.getButtonsProperty().getAProperty() == ButtonState::Pressed) ||
            touchTopLeft) {
            cameraSpringEnabled_ = !cameraSpringEnabled_;
        }

        // Reset the ship on R key or right thumb stick clicked
        if (currentKeyboardState_.IsKeyDown(Keys::R) ||
            currentGamePadState_.getButtonsProperty().getRightStickProperty() == ButtonState::Pressed) {
            ship_->Reset();
            camera_->Reset();
        }

        // Update the ship
        ship_->Update(gameTime);

        // Update the camera to chase the new target
        UpdateCameraChaseTarget();

        // The chase camera's update behavior is the springs, but we can use the Reset method
        // to have a locked, spring-less camera
        if (cameraSpringEnabled_)
            camera_->Update(gameTime);
        else
            camera_->Reset();

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        GraphicsDevice& device = getGraphicsDeviceProperty();

        device.Clear(Color::CornflowerBlue);

        device.setBlendStateProperty(BlendState::Opaque);
        device.setDepthStencilStateProperty(DepthStencilState::Default);
        device.getSamplerStatesProperty()[0] = SamplerState::LinearWrap;

        shipModel_->Draw(ship_->World(), camera_->View(), camera_->Projection());

        // NOXNA: Ground.x's two triangles come out wound the opposite way from what CNA's
        // default RasterizerState::CullCounterClockwise expects once round-tripped through
        // `assimp export` + tools/obj2model.py (confirmed live: with the default cull state
        // the ground never appeared at all, even though its vertex/index data loads and
        // uploads correctly; switching only the ground's draw to CullNone made it appear,
        // fully textured/shaded, with no other change) -- the same per-asset winding
        // adjustment HeightmapCollision's/GeneratedGeometry's own Terrain.hpp already needed
        // for their own runtime-built meshes. The ship (Ship.fbx, converted by
        // tools/fbx_ascii2model.py) needs no such adjustment; its winding survives conversion
        // correctly, matching every other FBX-sourced model in this repo.
        device.setRasterizerStateProperty(RasterizerState::CullNone);
        groundModel_->Draw(Matrix::getIdentityProperty(), camera_->View(), camera_->Projection());
        device.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);

        DrawOverlayText();

        Game::Draw(gameTime);
    }

private:
    std::unique_ptr<GraphicsDeviceManager> graphics_;

    std::optional<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> spriteFont_;

    KeyboardState lastKeyboardState_;
    GamePadState lastGamePadState_;
    MouseState lastMouseState_;
    KeyboardState currentKeyboardState_;
    GamePadState currentGamePadState_;
    MouseState currentMouseState_;

    std::unique_ptr<Ship> ship_;
    std::unique_ptr<ChaseCamera> camera_;

    // NOXNA: RawModel (see RawModel.hpp) instead of Model -- both Ship.fbx/Ground.x convert to
    // stride-32 .model.json files, which hit DEFERRED.md item #26's ModelTypeReader
    // vertex-corruption bug just like every other affected sample in this repo. See missing.md.
    std::optional<RawModel> shipModel_;
    std::optional<RawModel> groundModel_;

    bool cameraSpringEnabled_ = true;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;

    // Update the values to be chased by the camera
    void UpdateCameraChaseTarget() {
        camera_->ChasePosition = ship_->Position;
        camera_->ChaseDirection = ship_->Direction;
        camera_->Up = ship_->Up;
    }

    // Displays an overlay showing what the controls are, and which settings are currently
    // selected -- plus (CNA addition beyond the XNA original) the F1 help overlay, drawn last
    // in the same SpriteBatch Begin()/End() block.
    void DrawOverlayText() {
        spriteBatch_->Begin();

        std::string text = "-Touch, Right Trigger, or Spacebar = thrust\n"
                            "-Screen edges, Left Thumb Stick,\n  or Arrow keys = steer\n"
                            "-Press A or touch the top left corner\n  to toggle camera spring (" +
                            std::string(cameraSpringEnabled_ ? "on" : "off") + ")";

        // Draw the string twice to create a drop shadow, first colored black and offset one
        // pixel to the bottom right, then again in white at the intended position. This makes
        // text easier to read over the background.
        spriteBatch_->DrawString(*spriteFont_, text, Vector2(65.0f, 65.0f), Color::Black);
        spriteBatch_->DrawString(*spriteFont_, text, Vector2(64.0f, 64.0f), Color::White);

        if (helpTimer_ > 0.0f && helpTexture_.has_value()) {
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            float sx = (float)((vp.getWidthProperty() - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();
    }
};

} // namespace ChaseCameraSample

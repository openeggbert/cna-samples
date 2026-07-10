#pragma once

// Port of TiltPerspectiveSample.cs's `ParallaxSample` class (XNA 4.0
// TiltPerspective sample). Renders a small textured box (DebugDraw) viewed
// through a perspective matrix that shifts based on the estimated tilt of
// the device, plus a physics simulation of balls rolling around inside that
// same box (BallSimulation), gravity-driven by the same tilt reading.
//
// See AccelerometerHelper.hpp for the one genuinely invented piece of this
// port (a keyboard-tilt control scheme -- the original has no interactive
// fallback of any kind, only a non-interactive automatic wobble) and
// missing.md for the full account of every other difference from the C#
// original.

#include <cmath>
#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/BoundingBox.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Quaternion.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
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
#include "System/TimeSpan.hpp"

#include "AccelerometerHelper.hpp"
#include "BallSimulation.hpp"
#include "DebugDraw.hpp"

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace TiltPerspectiveSample {

class TiltPerspectiveGame : public Game {
public:
    TiltPerspectiveGame() {
        getContentProperty().setRootDirectoryProperty("Content");
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);

        // Request portrait mode (matches the original's own 480x800 request).
        graphics_->setPreferredBackBufferWidthProperty(480);
        graphics_->setPreferredBackBufferHeightProperty(800);

        // Frame rate is 30 fps by default (matches the original exactly --
        // this is one Windows-Phone default that carries over cleanly).
        setIsFixedTimeStepProperty(true);
        setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(333333));

        accelerometer_ = std::make_unique<AccelerometerHelper>(*this);
        getComponentsProperty().Add(accelerometer_.get());

        ballSimulation_ = std::make_unique<BallSimulation>(*this);
        ballSimulation_->AddWalls(worldBox_);
        ballSimulation_->AddBalls(25, 25.0f, 75.0f, worldBox_);
        getComponentsProperty().Add(ballSimulation_.get());
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "TiltPerspectiveGame";
        return name;
    }

protected:
    void LoadContent() override {
        boxTexture_.emplace(getContentProperty().Load<Texture2D>("stone4"));

        worldGeometry_.emplace(DebugDraw::CreateBoxInterior(getGraphicsDeviceProperty(), worldBox_));

        spriteBatch_.emplace(getGraphicsDeviceProperty());
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

        // F1 help overlay (CNA addition, see CLAUDE.md).
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        // Matches the original's own explicit `accelerometer.Update(gameTime);`
        // call here -- note this means the accelerometer component's Update()
        // runs *twice* this frame (once here, once again below via
        // Game::Update()'s normal Components iteration, since it's also
        // registered in Components/Enabled==true, exactly like the C#
        // original's own AccelerometerHelper). This double-update is a real
        // quirk of the original sample itself (not introduced by this port);
        // it's harmless for our keyboard-driven substitute, which just
        // re-samples the current KeyboardState idempotently each call --
        // unlike the original's own time-accumulating fake wobble
        // (FakeRollTheta += elapsed * FakeRollSpeed), which this same quirk
        // would silently make spin twice as fast. See missing.md.
        accelerometer_->Update(gameTime);

        // Allows the game to exit.
        GamePadState gamePadState = GamePad::GetState(PlayerIndex::One);
        if (gamePadState.IsButtonDown(Buttons::Back) || Keyboard::GetState().IsKeyDown(Keys::Escape))
            Exit();

        // NOXNA: the original recalibrates `referenceDown` from
        // `TouchPanel.GetState().Count > 0` (any active touch). This desktop
        // build has no touchscreen and CNA's TouchPanel is fed only from real
        // SDL finger events (confirmed by reading
        // cna/src/CNA/Internal/Input/SdlInputBridge.cpp -- mouse clicks are
        // never forwarded into TouchPanel), so a literal port would leave
        // this feature permanently unreachable. Substituted with "hold the
        // left mouse button," matching this repo's established touch-to-
        // mouse substitution pattern (see NEXT.md section 6) -- like the
        // original, this re-calibrates continuously every frame the button
        // stays held, not just on the initial click.
        if (Mouse::GetState().getLeftButtonProperty() == ButtonState::Pressed) {
            referenceDown_ = Vector3::Normalize(accelerometer_->getSmoothAccelerationProperty());
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        constexpr float zThreshold = 0.4f;

        getGraphicsDeviceProperty().Clear(Color::CornflowerBlue);

        // Make the light always come from the actual ceiling.
        Vector3 lightDirection = Vector3::Normalize(accelerometer_->getSmoothAccelerationProperty());
        Vector3 eyeDirection = ComputeEyeVector();

        if (eyeDirection.Z < zThreshold) {
            // Limit how far we distort the perspective.
            eyeDirection.Z = zThreshold;
            eyeDirection.Normalize();
        }
        Vector3 eyePosition = eyeDirection * eyeDistance_;

        Matrix world = Matrix::getIdentityProperty();

        Matrix view = Matrix::CreateLookAt(Vector3(eyePosition.X, eyePosition.Y, eyePosition.Z),
                                            Vector3(eyePosition.X, eyePosition.Y, 0.0f), Vector3(0.0f, 1.0f, 0.0f));

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        Matrix projection = Matrix::CreatePerspectiveOffCenter(
            (-eyePosition.X - vp.getWidthProperty() * 0.5f) * nearPlane_,
            (-eyePosition.X + vp.getWidthProperty() * 0.5f) * nearPlane_,
            (-eyePosition.Y - vp.getHeightProperty() * 0.5f) * nearPlane_,
            (-eyePosition.Y + vp.getHeightProperty() * 0.5f) * nearPlane_, eyePosition.Z * nearPlane_,
            farPlaneDistance_);

        worldGeometry_->getBasicEffectProperty().getDirectionalLight0Property().setDirectionProperty(lightDirection);
        worldGeometry_->Draw(world, view, projection, *boxTexture_);

        ballSimulation_->Draw(view, projection, lightDirection);

        DrawHelpOverlay();

        Game::Draw(gameTime);
    }

private:
    // distance from eye to screen, in pixel units.
    static constexpr float eyeDistance_ = 2000.0f;

    // distance from eye to near clip plane, as a fraction of the Z distance
    // to the screen. We do it this way because we can end up with some
    // highly skewed projection matrices if the device is tilted enough.
    static constexpr float nearPlane_ = 0.5f;

    // distance from the eye to the far clip plane, in pixel units.
    static constexpr float farPlaneDistance_ = 4000.0f;

    // size of the box that everything takes place in, measured in pixel units.
    BoundingBox worldBox_{Vector3(-400.0f, -400.0f, -400.0f), Vector3(400.0f, 400.0f, 0.0f)};

    // "down" direction (smoothed accelerometer reading) to use as our
    // reference position. The user can reset this by touching the screen
    // (here: holding the left mouse button -- see Update()).
    Vector3 referenceDown_ = -Vector3::UnitZ;

    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::unique_ptr<AccelerometerHelper> accelerometer_;
    std::unique_ptr<BallSimulation> ballSimulation_;
    std::optional<DebugDraw> worldGeometry_;

    std::optional<Texture2D> boxTexture_;

    std::optional<SpriteBatch> spriteBatch_;
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;

    // Compute (guess) the user's eye direction, given a reference 'down'
    // direction and the current 'down' direction.
    //
    // 'world' vectors in this function refer to the actual real-world coords
    // from the (estimated) user's perspective. We don't really know where the
    // user is relative to the screen, so we have to make an assumption about
    // how they hold and tilt the device.
    [[nodiscard]] Vector3 ComputeEyeVector() const {
        float referencePitch = std::asin(referenceDown_.Y);
        constexpr float rollEpsilon = 0.1f;

        Vector3 worldDown = -accelerometer_->getSmoothAccelerationProperty();
        worldDown.Normalize();

        Vector3 worldRight = Vector3::Cross(Vector3::UnitY, worldDown);

        if (worldRight.LengthSquared() < rollEpsilon) {
            // The device is held nearly vertically, so the worldRight vector
            // isn't well-defined (its length is close to zero). Just use our
            // local right vector as the world right vector, which means we
            // generate an orientation with no roll.
            worldRight = Vector3::Right;
        } else {
            // We have a good 'right' vector; normalize it for
            // CreateFromAxisAngle() below.
            worldRight.Normalize();
        }
        Quaternion rot = Quaternion::CreateFromAxisAngle(worldRight, -referencePitch);
        return Vector3::Transform(worldDown, rot);
    }

    void DrawHelpOverlay() {
        if (helpTimer_ <= 0.0f || !helpTexture_.has_value()) return;
        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int hw = helpTexture_->getWidthProperty();
        int hh = helpTexture_->getHeightProperty();
        float sx = static_cast<float>((vp.getWidthProperty() - hw) / 2);
        float sy = static_cast<float>((vp.getHeightProperty() - hh) / 2);
        spriteBatch_->Begin();
        spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        spriteBatch_->End();
    }
};

} // namespace TiltPerspectiveSample

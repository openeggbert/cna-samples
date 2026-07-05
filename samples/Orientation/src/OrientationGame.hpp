#pragma once

// OrientationGame.hpp — C++ port of OrientationSample.cs (XNA 4.0
// Orientation sample). The shipped original documents four orientation-
// handling scenarios as alternate, mostly-commented-out configurations in
// the same file (see the sample's own .htm: "In order to see all four
// approaches, change the sample's code as instructed"); the live default
// (full-resolution, landscape-locked, no interactivity) is one line away
// from demonstrating nothing at all. This port enables Scenario #4 (full
// resolution, both orientations supported, dynamic tap-to-lock/unlock) —
// real, complete code already present in the original, not invented — see
// missing.md. `LayoutSample.cs`, present in the original directory but not
// referenced by `Program.cs` nor included in the shipped .csproj, is dead
// code and was not ported.

#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/DisplayOrientation.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"

namespace OrientationSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::DisplayOrientation;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// Port of OrientationSample.cs, Scenario #4 enabled (see file header).
class OrientationGame : public Microsoft::Xna::Framework::Game {
public:
    const std::string& GetTypeName() const override {
        static const std::string name = "OrientationGame";
        return name;
    }

    OrientationGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");

        // Scenario #4: both orientations supported, dynamic lock/unlock.
        graphics_.setSupportedOrientationsProperty(
            DisplayOrientation::Portrait | DisplayOrientation::LandscapeLeft | DisplayOrientation::LandscapeRight);
        enableOrientationLocking_ = true;

        graphics_.setIsFullScreenProperty(false);
    }

protected:
    void Initialize() override {
        // For scenario #4, lock/unlock is triggered by the tap gesture.
        TouchPanel::setEnabledGesturesProperty(GestureType::Tap);

        Game::Initialize();
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        directions_.emplace(getContentProperty().Load<Texture2D>("directions"));
        font_.emplace(getContentProperty().Load<SpriteFont>("Font"));
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        if (GamePad::GetState(PlayerIndex::One).IsButtonDown(Buttons::Back) ||
            Keyboard::GetState().IsKeyDown(Keys::Escape))
            Exit();

        // CNA addition (see missing.md): this desktop has no physical
        // rotation sensor, so 'O' stands in for physically rotating the
        // device -- same established pattern as DynamicMenu/#077. Only
        // takes effect while unlocked, matching how a real locked phone
        // ignores physical rotation.
        bool curO = Keyboard::GetState().IsKeyDown(Keys::O);
        if (curO && !prevO_ && !orientationLocked_)
            CycleOrientation();
        prevO_ = curO;

        // CNA addition (see missing.md): a left-click is turned into a
        // synthetic Tap gesture (CNA does not synthesize touch/gesture
        // events from mouse input), so the original's own TouchPanel-based
        // lock/unlock logic below runs completely unmodified.
        MouseState mouse = Mouse::GetState();
        bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        if (mouseDown && !prevMouseDown_) {
            Vector2 mousePos((float)mouse.getXProperty(), (float)mouse.getYProperty());
            TouchPanel::EnqueueGesture(GestureSample(GestureType::Tap, gameTime.getTotalGameTimeProperty(), mousePos,
                                                     Vector2::Zero, Vector2::Zero, Vector2::Zero));
        }
        prevMouseDown_ = mouseDown;

        if (enableOrientationLocking_) {
            while (TouchPanel::getIsGestureAvailableProperty()) {
                GestureSample gesture = TouchPanel::ReadGesture();

                if (gesture.getGestureTypeProperty() == GestureType::Tap) {
                    orientationLocked_ = !orientationLocked_;

                    if (orientationLocked_) {
                        // Lock to whatever orientation is current, and keep
                        // the current back-buffer size exactly as it is.
                        graphics_.setSupportedOrientationsProperty(currentOrientation_);
                    } else {
                        graphics_.setSupportedOrientationsProperty(DisplayOrientation::LandscapeLeft |
                                                                   DisplayOrientation::LandscapeRight |
                                                                   DisplayOrientation::Portrait);
                    }

                    graphics_.ApplyChanges();
                }
            }
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255));

        spriteBatch_->Begin();

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        Vector2 position((float)(vp.getWidthProperty() / 2 - directions_->getWidthProperty() / 2),
                          (float)(vp.getHeightProperty() / 2 - directions_->getHeightProperty() / 2));
        spriteBatch_->Draw(*directions_, position, Color(255, 255, 255, 255));

        if (enableOrientationLocking_) {
            std::string currentState = orientationLocked_ ? "Orientation: Locked" : "Orientation: Unlocked";
            std::string instructions =
                orientationLocked_ ? "Tap to unlock orientation." : "Tap to lock orientation.";
            std::string rotateHint = "('O' simulates physically rotating the device.)";

            spriteBatch_->DrawString(*font_, currentState, Vector2(10.0f, 10.0f), Color(255, 255, 255, 255));
            spriteBatch_->DrawString(*font_, instructions, Vector2(10.0f, 25.0f), Color(255, 255, 255, 255));
            spriteBatch_->DrawString(*font_, rotateHint, Vector2(10.0f, 40.0f), Color(255, 255, 255, 255));
        }

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            float sx = (float)((vp.getWidthProperty() - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    static constexpr int LandscapeWidth = 800;
    static constexpr int LandscapeHeight = 480;
    static constexpr int PortraitWidth = 480;
    static constexpr int PortraitHeight = 800;

    // CNA addition: simulates a physical device rotation event by cycling
    // through the currently-supported orientations and resizing the back
    // buffer to match, since this desktop has no rotation sensor to raise
    // Window.OrientationChanged from.
    //
    // Only toggles Landscape <-> Portrait (not also LandscapeLeft <->
    // LandscapeRight): the "directions" texture is drawn without any
    // rotation transform in the original (see file header comment on why),
    // so the two landscape variants are visually identical here -- cycling
    // through both would make every other 'O' press look like it did
    // nothing. Matches DynamicMenu's (#077) own two-state toggle.
    void CycleOrientation() {
        bool togglingToPortrait = currentOrientation_ != DisplayOrientation::Portrait;
        currentOrientation_ = togglingToPortrait ? DisplayOrientation::Portrait : DisplayOrientation::LandscapeLeft;

        graphics_.setPreferredBackBufferWidthProperty(togglingToPortrait ? PortraitWidth : LandscapeWidth);
        graphics_.setPreferredBackBufferHeightProperty(togglingToPortrait ? PortraitHeight : LandscapeHeight);
        graphics_.ApplyChanges();
    }

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;

    std::optional<Texture2D> directions_;
    std::optional<SpriteFont> font_;
    std::optional<Texture2D> helpTexture_;

    bool orientationLocked_ = false;
    bool enableOrientationLocking_ = false;
    DisplayOrientation currentOrientation_ = DisplayOrientation::LandscapeLeft;

    bool prevO_ = false;
    bool prevMouseDown_ = false;

    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

} // namespace OrientationSample

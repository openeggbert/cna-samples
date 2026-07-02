#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchCollection.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocationState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"

#include "Sprite.hpp"

namespace GesturesSample {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchCollection;
using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// Port of the XNA 4.0 "TouchGestureSample" (Windows Phone). Demonstrates the
// Hold, Tap, Drag, Flick, and Pinch gesture APIs: hold empty space to create a
// cat sprite, hold a sprite to remove it, tap to change its color, drag to
// move it, flick to throw it, and pinch to scale it.
//
// The original targets a touch screen exclusively. This desktop port adds a
// parallel mouse input path (left-drag = drag, quick click = tap, press-and-
// hold = hold, release-while-moving = flick, scroll wheel = pinch/scale) so
// the sample is playable without touch hardware; see missing.md.
class GesturesGame : public Game {
public:
    GesturesGame() : graphics_(this) {
        graphics_.setPreferredBackBufferWidthProperty(ScreenWidth);
        graphics_.setPreferredBackBufferHeightProperty(ScreenHeight);
        getContentProperty().setRootDirectoryProperty("Content");

        // Frame rate is 30 fps by default for Windows Phone.
        setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(333333));
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "GesturesGame";
        return name;
    }

protected:
    void Initialize() override {
        // Gotcha: TouchPanel does not learn the resolution from the back
        // buffer automatically; without this, gesture positions come out at
        // (0, 0). See NEXT.md.
        TouchPanel::setDisplayWidthProperty(ScreenWidth);
        TouchPanel::setDisplayHeightProperty(ScreenHeight);

        // Enable the gestures we care about. We use both Tap and DoubleTap to
        // work around a bug in the XNA GS 4.0 Beta where some taps were
        // missed if only Tap was specified (kept for fidelity to the original).
        TouchPanel::setEnabledGesturesProperty(
            GestureType::Hold | GestureType::Tap | GestureType::DoubleTap |
            GestureType::FreeDrag | GestureType::Flick | GestureType::Pinch);

        Game::Initialize();
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        catTexture_ = getContentProperty().Load<Texture2D>("Images/cat");
        font_.emplace(getContentProperty().Load<SpriteFont>("Fonts/Font"));
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());

        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        if (GamePad::GetState(PlayerIndex::One).IsButtonDown(Buttons::Back) ||
            Keyboard::GetState().IsKeyDown(Keys::Escape)) {
            Exit();
        }

        HandleMouseInput(elapsed);
        HandleTouchInput();

        Rectangle bounds = getGraphicsDeviceProperty().getViewportProperty().getBoundsProperty();
        for (auto& sprite : sprites_) {
            sprite->Update(gameTime, bounds);
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color::CornflowerBlue);

        spriteBatch_->Begin();

        for (auto& sprite : sprites_) {
            sprite->Draw(*spriteBatch_);
        }

        spriteBatch_->DrawString(*font_, HelpText, Vector2(10.0f, 32.0f), Color::White);

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = static_cast<float>((vp.getWidthProperty() - hw) / 2);
            float sy = static_cast<float>((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    static constexpr int ScreenWidth = 800;
    static constexpr int ScreenHeight = 480;

    // Time a mouse button must be held with no movement to count as a Hold
    // gesture; touch hardware uses TouchPanel's own gesture recognizer, this
    // constant only governs the desktop mouse fallback.
    static constexpr float MouseHoldSeconds = 0.75f;

    static constexpr const char* HelpText =
        "Hold (in empty space) - Create sprite\n"
        "Hold (on sprite) - Remove sprite\n"
        "Tap - Change sprite color\n"
        "Drag - Move sprite\n"
        "Flick - Throws sprite\n"
        "Pinch - Scale sprite (mouse: scroll wheel)";

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<SpriteFont> font_;
    Texture2D catTexture_;

    std::vector<std::unique_ptr<Sprite>> sprites_;
    Sprite* selectedSprite_ = nullptr;

    // --- Mouse fallback state (CNA addition; see class comment) ---
    MouseState prevMouseState_;
    Vector2 lastMousePos_;
    float mouseHoldTimer_ = 0.0f;
    bool mouseDragged_ = false;
    bool mouseHoldFired_ = false;
    bool mouseButtonActive_ = false;
    Vector2 mouseVelocity_;

    // --- F1 help overlay ---
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;

    void BringToFront(Sprite* sprite) {
        for (std::size_t i = 0; i < sprites_.size(); ++i) {
            if (sprites_[i].get() == sprite) {
                auto owned = std::move(sprites_[i]);
                sprites_.erase(sprites_.begin() + static_cast<std::ptrdiff_t>(i));
                sprites_.push_back(std::move(owned));
                break;
            }
        }
    }

    void RemoveSprite(Sprite* sprite) {
        sprites_.erase(
            std::remove_if(sprites_.begin(), sprites_.end(),
                            [sprite](const std::unique_ptr<Sprite>& s) { return s.get() == sprite; }),
            sprites_.end());
    }

    // Selects the topmost sprite hit by a press at the given point, matching
    // both the original's touch hit-testing and the mouse fallback's.
    void SelectSpriteAt(const Point& point) {
        selectedSprite_ = nullptr;
        for (auto it = sprites_.rbegin(); it != sprites_.rend(); ++it) {
            if ((*it)->HitBounds().Contains(point)) {
                selectedSprite_ = it->get();
                break;
            }
        }

        if (selectedSprite_ != nullptr) {
            selectedSprite_->Velocity = Vector2::Zero;
            BringToFront(selectedSprite_);
        }
    }

    // Shared Hold behavior: create a sprite in empty space, or remove the
    // currently selected one.
    void ToggleHoldAt(const Vector2& position) {
        if (selectedSprite_ == nullptr) {
            auto sprite = std::make_unique<Sprite>(catTexture_);
            sprite->Center = position;
            selectedSprite_ = sprite.get();
            sprites_.push_back(std::move(sprite));
        } else {
            RemoveSprite(selectedSprite_);
            selectedSprite_ = nullptr;
        }
    }

    void HandleTouchInput() {
        // We use raw touch points for selection, since they are more
        // appropriate for that use than gestures. So we need that raw data.
        TouchCollection touches = TouchPanel::GetState();

        // See if we have a new primary point down. When the first touch goes
        // down, hit-test to try and select one of our sprites.
        if (touches.getCountProperty() > 0 &&
            touches[0].getStateProperty() == TouchLocationState::Pressed) {
            const Vector2& pos = touches[0].getPositionProperty();
            SelectSpriteAt(Point(static_cast<int>(pos.X), static_cast<int>(pos.Y)));
        }

        // Read in all queued gestures so TouchPanel's queue doesn't back up.
        while (TouchPanel::getIsGestureAvailableProperty()) {
            GestureSample gesture = TouchPanel::ReadGesture();

            switch (gesture.getGestureTypeProperty()) {
                case GestureType::Tap:
                case GestureType::DoubleTap:
                    if (selectedSprite_ != nullptr) {
                        selectedSprite_->ChangeColor();
                    }
                    break;

                case GestureType::Hold:
                    ToggleHoldAt(gesture.getPositionProperty());
                    break;

                case GestureType::FreeDrag:
                    if (selectedSprite_ != nullptr) {
                        selectedSprite_->Center = selectedSprite_->Center + gesture.getDeltaProperty();
                    }
                    break;

                case GestureType::Flick:
                    if (selectedSprite_ != nullptr) {
                        selectedSprite_->Velocity = gesture.getDeltaProperty();
                    }
                    break;

                case GestureType::Pinch: {
                    if (selectedSprite_ != nullptr) {
                        Vector2 a = gesture.getPositionProperty();
                        Vector2 aOld = a - gesture.getDeltaProperty();
                        Vector2 b = gesture.getPosition2Property();
                        Vector2 bOld = b - gesture.getDelta2Property();

                        float d = Vector2::Distance(a, b);
                        float dOld = Vector2::Distance(aOld, bOld);

                        float scaleChange = (d - dOld) * 0.01f;
                        selectedSprite_->setScale(selectedSprite_->getScale() + scaleChange);
                    }
                    break;
                }

                default:
                    break;
            }
        }

        // If there are no raw touch points and the mouse isn't claiming the
        // selection either, deselect. Some gestures (taps, flicks) arrive on
        // the same frame the touch is released, so this runs last.
        if (touches.getCountProperty() == 0 && !mouseButtonActive_) {
            selectedSprite_ = nullptr;
        }
    }

    // Parallel desktop input path (see class comment): left-click drag moves
    // the selected sprite, a quick click changes its color, press-and-hold
    // creates/removes a sprite, releasing mid-drag throws it, and the scroll
    // wheel scales it in place of a two-finger pinch.
    void HandleMouseInput(float elapsed) {
        MouseState mouse = Mouse::GetState();
        bool down = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        bool wasDown = prevMouseState_.getLeftButtonProperty() == ButtonState::Pressed;
        Vector2 pos(static_cast<float>(mouse.getXProperty()), static_cast<float>(mouse.getYProperty()));

        if (down && !wasDown) {
            SelectSpriteAt(Point(mouse.getXProperty(), mouse.getYProperty()));
            mouseHoldTimer_ = 0.0f;
            mouseDragged_ = false;
            mouseHoldFired_ = false;
        } else if (down && wasDown) {
            Vector2 delta = pos - lastMousePos_;
            if (selectedSprite_ != nullptr && (delta.X != 0.0f || delta.Y != 0.0f)) {
                selectedSprite_->Center = selectedSprite_->Center + delta;
                mouseDragged_ = true;
                if (elapsed > 0.0f) {
                    mouseVelocity_ = delta * (1.0f / elapsed);
                }
            }

            mouseHoldTimer_ += elapsed;
            if (!mouseDragged_ && !mouseHoldFired_ && mouseHoldTimer_ >= MouseHoldSeconds) {
                mouseHoldFired_ = true;
                ToggleHoldAt(pos);
            }
        } else if (!down && wasDown) {
            if (selectedSprite_ != nullptr) {
                if (mouseDragged_) {
                    selectedSprite_->Velocity = mouseVelocity_;
                } else if (!mouseHoldFired_) {
                    selectedSprite_->ChangeColor();
                }
            }
        }

        int scrollDelta = mouse.getScrollWheelValueProperty() - prevMouseState_.getScrollWheelValueProperty();
        if (scrollDelta != 0 && selectedSprite_ != nullptr) {
            selectedSprite_->setScale(selectedSprite_->getScale() + static_cast<float>(scrollDelta) * 0.001f);
            // Scaling is active manipulation, like a drag: it must not also
            // let the hold timer fire and remove the sprite out from under it.
            mouseDragged_ = true;
        }

        mouseButtonActive_ = down;
        lastMousePos_ = pos;
        prevMouseState_ = mouse;
    }
};

} // namespace GesturesSample

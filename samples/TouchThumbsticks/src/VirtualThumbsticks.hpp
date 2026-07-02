#pragma once

#include <optional>

#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchCollection.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocation.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"

namespace TouchThumbsticks {

using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::Touch::TouchCollection;
using Microsoft::Xna::Framework::Input::Touch::TouchLocation;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// Represents virtual thumbsticks from touch input. Touching the left half of
// the screen places the center of the left thumbstick, the right half the
// right thumbstick; dragging away from that center simulates thumbstick
// input. Port of the XNA 4.0 "TouchThumbSticks" sample's VirtualThumbsticks.cs
// (a static class there, for parity with TouchPanel/GamePad/Keyboard).
//
// CNA addition: this sample has no touchscreen-free way to test on a desktop
// (unlike a gesture sample, two *simultaneous* contacts are fundamental here),
// so when a half isn't currently touched, the getters fall back to WASD/arrow
// keys for the left stick and mouse-position-relative-to-screen-center for
// the right stick (move+fire is mouse position; see missing.md).
class VirtualThumbsticks {
public:
    VirtualThumbsticks() = delete;

    static std::optional<Vector2> getLeftThumbstickCenter() { return leftCenter_; }
    static std::optional<Vector2> getRightThumbstickCenter() { return rightCenter_; }

    static Vector2 getLeftThumbstick() {
        if (!leftCenter_.has_value()) return getKeyboardLeftFallback();

        Vector2 l = (leftPosition_ - *leftCenter_) / MaxThumbstickDistance;
        if (l.LengthSquared() > 1.0f) l.Normalize();
        return l;
    }

    static Vector2 getRightThumbstick() {
        if (!rightCenter_.has_value()) return getMouseRightFallback();

        Vector2 r = (rightPosition_ - *rightCenter_) / MaxThumbstickDistance;
        if (r.LengthSquared() > 1.0f) r.Normalize();
        return r;
    }

    // Updates the virtual thumbsticks based on current touch state. Must be
    // called every frame.
    static void Update() {
        std::optional<TouchLocation> leftTouch, rightTouch;
        TouchCollection touches = TouchPanel::GetState();

        // Examine all the touches to convert them to virtual stick positions.
        // 'touches' is the set of all touches this instant, not a sequence of
        // events; the only sequential information available is each touch's
        // previous location.
        for (const auto& touch : touches) {
            if (touch.getIdProperty() == leftId_) {
                leftTouch = touch;
                continue;
            }

            if (touch.getIdProperty() == rightId_) {
                rightTouch = touch;
                continue;
            }

            // Didn't continue an existing thumbstick gesture; see if we can
            // start a new one. Use the previous touch position if possible,
            // to get as close as possible to where the gesture actually
            // began.
            TouchLocation earliestTouch;
            if (!touch.TryGetPreviousLocation(earliestTouch)) earliestTouch = touch;

            if (leftId_ == TouchPanel::NO_FINGER) {
                if (earliestTouch.getPositionProperty().X < TouchPanel::getDisplayWidthProperty() / 2.0f) {
                    leftTouch = earliestTouch;
                    continue;
                }
            }

            if (rightId_ == TouchPanel::NO_FINGER) {
                if (earliestTouch.getPositionProperty().X >= TouchPanel::getDisplayWidthProperty() / 2.0f) {
                    rightTouch = earliestTouch;
                    continue;
                }
            }
        }

        if (leftTouch.has_value()) {
            if (!leftCenter_.has_value()) leftCenter_ = leftTouch->getPositionProperty();
            leftPosition_ = leftTouch->getPositionProperty();
            leftId_ = leftTouch->getIdProperty();
        } else {
            leftCenter_.reset();
            leftId_ = TouchPanel::NO_FINGER;
        }

        if (rightTouch.has_value()) {
            if (!rightCenter_.has_value()) rightCenter_ = rightTouch->getPositionProperty();
            rightPosition_ = rightTouch->getPositionProperty();
            rightId_ = rightTouch->getIdProperty();
        } else {
            rightCenter_.reset();
            rightId_ = TouchPanel::NO_FINGER;
        }
    }

private:
    // Distance in screen pixels that represents a thumbstick value of 1.
    static constexpr float MaxThumbstickDistance = 60.0f;

    // Mouse-aim fallback: pixels from screen center before the right stick
    // starts registering any magnitude, and the pixel distance at which it
    // reaches full magnitude (1.0).
    static constexpr float MouseAimDeadzone = 20.0f;
    static constexpr float MouseAimFullThrow = 200.0f;

    static Vector2 getKeyboardLeftFallback() {
        KeyboardState kb = Keyboard::GetState();
        Vector2 dir;
        if (kb.IsKeyDown(Keys::A) || kb.IsKeyDown(Keys::Left)) dir.X -= 1.0f;
        if (kb.IsKeyDown(Keys::D) || kb.IsKeyDown(Keys::Right)) dir.X += 1.0f;
        if (kb.IsKeyDown(Keys::W) || kb.IsKeyDown(Keys::Up)) dir.Y -= 1.0f;
        if (kb.IsKeyDown(Keys::S) || kb.IsKeyDown(Keys::Down)) dir.Y += 1.0f;
        if (dir.LengthSquared() > 1.0f) dir.Normalize();
        return dir;
    }

    static Vector2 getMouseRightFallback() {
        MouseState mouse = Mouse::GetState();
        Vector2 center(static_cast<float>(TouchPanel::getDisplayWidthProperty()) / 2.0f,
                       static_cast<float>(TouchPanel::getDisplayHeightProperty()) / 2.0f);
        Vector2 offset = Vector2(static_cast<float>(mouse.getXProperty()), static_cast<float>(mouse.getYProperty())) -
                         center;

        float dist = offset.Length();
        if (dist < MouseAimDeadzone) return Vector2::Zero;

        Vector2 dir = offset / dist;
        float magnitude = MathHelper::Clamp((dist - MouseAimDeadzone) / (MouseAimFullThrow - MouseAimDeadzone), 0.0f, 1.0f);
        return dir * magnitude;
    }

    inline static Vector2 leftPosition_;
    inline static Vector2 rightPosition_;
    inline static int leftId_ = TouchPanel::NO_FINGER;
    inline static int rightId_ = TouchPanel::NO_FINGER;
    inline static std::optional<Vector2> leftCenter_;
    inline static std::optional<Vector2> rightCenter_;
};

} // namespace TouchThumbsticks

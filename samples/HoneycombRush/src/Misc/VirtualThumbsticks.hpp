#pragma once

// VirtualThumbsticks.hpp — C++ port of Misc/VirtualThumbsticks.cs (XNA 4.0
// HoneycombRush sample). Touching the left half of the screen places the
// center of the left thumbstick, the right half the right thumbstick;
// dragging away from that center simulates thumbstick input.
//
// CNA addition (see missing.md): this desktop has no touchscreen. When no
// real left-stick touch is active, Update() synthesizes one from WASD/arrow
// keys, anchored at a fixed point set via SetKeyboardAnchor() (GameplayScreen
// sets this to the on-screen joystick widget's center) — this way
// LeftThumbstick *and* LeftThumbstickCenter behave exactly as if a real touch
// were happening there, so BeeKeeper's direction-facing logic (which checks
// the center, not just the thumbstick vector) needs no changes at all. The
// right "thumbstick" is only ever used by GameplayScreen to hit-test a touch
// origin against the on-screen smoke button — the original already has an
// independent Space-key fallback for firing smoke, so RightThumbstick/
// RightThumbstickCenter are left touch-only.

#include <optional>

#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchCollection.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocation.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"

#include "../ScreenManager/InputState.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Touch::TouchCollection;
using Microsoft::Xna::Framework::Input::Touch::TouchLocation;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

class VirtualThumbsticks {
public:
    VirtualThumbsticks() = delete;

    static std::optional<Vector2> getLeftThumbstickCenter() { return leftThumbstickCenter_; }
    static std::optional<Vector2> getRightThumbstickCenter() { return rightThumbstickCenter_; }

    // The current raw left-stick position (touch position, or the
    // keyboard-synthesized equivalent). CNA addition: GameplayScreen uses this
    // in place of the original's own `lastTouchPosition` (tracked there from
    // raw TouchState iteration), so the keyboard fallback above also covers
    // GameplayScreen's "is the touch within the joystick's outer boundary"
    // sanity check with no extra plumbing — see missing.md.
    static Vector2 getLeftPosition() { return leftPosition_; }

    static Vector2 getLeftThumbstick() {
        if (!leftThumbstickCenter_.has_value()) return Vector2::Zero;

        Vector2 l = (leftPosition_ - *leftThumbstickCenter_) / MaxThumbstickDistance;
        if (l.LengthSquared() > 1.0f) l.Normalize();
        return l;
    }

    static Vector2 getRightThumbstick() {
        if (!rightThumbstickCenter_.has_value()) return Vector2::Zero;

        Vector2 r = (rightPosition_ - *rightThumbstickCenter_) / MaxThumbstickDistance;
        if (r.LengthSquared() > 1.0f) r.Normalize();
        return r;
    }

    // Sets the screen-space point that a synthesized keyboard "touch" is
    // anchored at (the on-screen virtual joystick widget's center).
    static void SetKeyboardAnchor(Vector2 anchor) { keyboardAnchor_ = anchor; }

    // Updates the virtual thumbsticks based on current touch state. Must be
    // called every frame.
    static void Update(InputState& input) {
        std::optional<TouchLocation> leftTouch, rightTouch;
        const TouchCollection& touches = input.TouchState;

        for (const TouchLocation& touch : touches) {
            if (touch.getIdProperty() == leftId_) {
                leftTouch = touch;
                continue;
            }

            if (touch.getIdProperty() == rightId_) {
                rightTouch = touch;
                continue;
            }

            TouchLocation earliestTouch = touch;
            TouchLocation previous;
            if (touch.TryGetPreviousLocation(previous)) {
                earliestTouch = previous;
            }

            if (leftId_ == -1) {
                if (earliestTouch.getPositionProperty().X < (float)TouchPanel::getDisplayWidthProperty() / 2.0f) {
                    leftTouch = earliestTouch;
                    continue;
                }
            }

            if (rightId_ == -1) {
                if (earliestTouch.getPositionProperty().X >= (float)TouchPanel::getDisplayWidthProperty() / 2.0f) {
                    rightTouch = earliestTouch;
                    continue;
                }
            }
        }

        if (leftTouch.has_value()) {
            if (!leftThumbstickCenter_.has_value()) {
                leftThumbstickCenter_ = leftTouch->getPositionProperty();
            }
            leftPosition_ = leftTouch->getPositionProperty();
            leftId_ = leftTouch->getIdProperty();
        } else {
            // No real left-stick touch — synthesize one from the keyboard,
            // anchored at the joystick widget's center, so downstream code
            // that checks LeftThumbstickCenter (not just LeftThumbstick) keeps
            // working unmodified.
            Vector2 keyboardVector = GetKeyboardFallback();
            if (keyboardVector != Vector2::Zero) {
                leftThumbstickCenter_ = keyboardAnchor_;
                leftPosition_ = keyboardAnchor_ + keyboardVector * MaxThumbstickDistance;
            } else {
                leftThumbstickCenter_.reset();
            }
            leftId_ = -1;
        }

        if (rightTouch.has_value()) {
            if (!rightThumbstickCenter_.has_value()) {
                rightThumbstickCenter_ = rightTouch->getPositionProperty();
            }
            rightPosition_ = rightTouch->getPositionProperty();
            rightId_ = rightTouch->getIdProperty();
        } else {
            rightThumbstickCenter_.reset();
            rightId_ = -1;
        }
    }

private:
    static constexpr float MaxThumbstickDistance = 60.0f;

    static Vector2 GetKeyboardFallback() {
        KeyboardState keyState = Keyboard::GetState();
        float x = 0.0f, y = 0.0f;
        if (keyState.IsKeyDown(Keys::Left) || keyState.IsKeyDown(Keys::A)) x -= 1.0f;
        if (keyState.IsKeyDown(Keys::Right) || keyState.IsKeyDown(Keys::D)) x += 1.0f;
        if (keyState.IsKeyDown(Keys::Up) || keyState.IsKeyDown(Keys::W)) y -= 1.0f;
        if (keyState.IsKeyDown(Keys::Down) || keyState.IsKeyDown(Keys::S)) y += 1.0f;

        Vector2 v(x, y);
        if (v.LengthSquared() > 1.0f) v.Normalize();
        return v;
    }

    static inline Vector2 leftPosition_;
    static inline Vector2 rightPosition_;

    static inline int leftId_ = -1;
    static inline int rightId_ = -1;

    static inline std::optional<Vector2> leftThumbstickCenter_;
    static inline std::optional<Vector2> rightThumbstickCenter_;

    static inline Vector2 keyboardAnchor_;
};

} // namespace HoneycombRush

#pragma once

#include <algorithm>
#include <cmath>

#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocationState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"

#include "../InputState.hpp"

namespace UISample::Controls {

using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// Watches the touch panel (and, via InputState's mouse fallback, the mouse --
// see missing.md) for drag and flick gestures, and computes the position of a
// viewport within a larger canvas, emulating Silverlight-style scrolling
// controls. This class only computes the view rectangle; rendering it is up
// to client code (see ScrollingPanelControl). Port of Controls/ScrollTracker.cs.
class ScrollTracker {
public:
    // This class watches for HorizontalDrag, DragComplete, and Flick gestures,
    // but doesn't set TouchPanel::EnabledGestures itself (that could interfere
    // with gestures needed elsewhere) -- client code must set it, using this
    // constant.
    static constexpr GestureType GesturesNeeded =
        GestureType::Flick | GestureType::VerticalDrag | GestureType::DragComplete;

    // A rectangle (set by client code) giving the area of the canvas to
    // scroll around in. Normally taller or wider than the viewport.
    Rectangle CanvasRect;

    // Describes the currently visible view area. Normally the caller sets
    // this once to establish the viewport size and initial position, and
    // from then on lets ScrollTracker move it around; it can be set at any
    // time to change the position or size of the viewport.
    Rectangle ViewRect;

    ScrollTracker() {
        ViewRect = Rectangle(0, 0, TouchPanel::getDisplayWidthProperty(), TouchPanel::getDisplayHeightProperty());
        CanvasRect = ViewRect;
    }

    // Same as CanvasRect, except extended to be at least as large as
    // ViewRect -- the true canvas area scrolled around in.
    Rectangle FullCanvasRect() const {
        Rectangle value = CanvasRect;
        if (value.Width < ViewRect.Width) value.Width = ViewRect.Width;
        if (value.Height < ViewRect.Height) value.Height = ViewRect.Height;
        return value;
    }

    bool IsTracking() const { return isTracking_; }

    bool IsMoving() const {
        return isTracking_ || velocity_.X != 0.0f || velocity_.Y != 0.0f || !FullCanvasRect().Contains(ViewRect);
    }

    // Must be called manually each tick that the ScrollTracker is active.
    void Update(const GameTime& gametime) {
        float dt = (float)gametime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        Vector2 viewMin(0.0f, 0.0f);
        Vector2 viewMax((float)(CanvasRect.Width - ViewRect.Width), (float)(CanvasRect.Height - ViewRect.Height));
        viewMax.X = std::max(viewMin.X, viewMax.X);
        viewMax.Y = std::max(viewMin.Y, viewMax.Y);

        if (isTracking_) {
            // ViewOrigin is a soft-clamped version of UnclampedViewOrigin
            viewOrigin_.X = SoftClamp(unclampedViewOrigin_.X, viewMin.X, viewMax.X);
            viewOrigin_.Y = SoftClamp(unclampedViewOrigin_.Y, viewMin.Y, viewMax.Y);
        } else {
            ApplyVelocity(dt, viewOrigin_.X, velocity_.X, viewMin.X, viewMax.X);
            ApplyVelocity(dt, viewOrigin_.Y, velocity_.Y, viewMin.Y, viewMax.Y);
        }
        ViewRect.X = (int)viewOrigin_.X;
        ViewRect.Y = (int)viewOrigin_.Y;
    }

    // Must be called manually each tick that the ScrollTracker is active.
    void HandleInput(InputState& input) {
        // Turn on tracking as soon as we see any kind of touch. We can't use
        // gestures for this because no gesture data is returned on the
        // initial touch. We have to be careful to pick out only 'Pressed'
        // locations, because TouchState can return other events a frame
        // *after* we've seen Flick or DragComplete.
        if (!isTracking_) {
            for (int i = 0; i < input.TouchState.getCountProperty(); i++) {
                if (input.TouchState[(std::size_t)i].getStateProperty() == TouchLocationState::Pressed) {
                    velocity_ = Vector2::Zero;
                    unclampedViewOrigin_ = viewOrigin_;
                    isTracking_ = true;
                    break;
                }
            }
        }

        for (const GestureSample& sample : input.Gestures) {
            switch (sample.getGestureTypeProperty()) {
                case GestureType::VerticalDrag:
                    unclampedViewOrigin_.Y -= sample.getDeltaProperty().Y;
                    break;

                case GestureType::Flick:
                    // Only respond to mostly-vertical flicks
                    if (std::abs(sample.getDeltaProperty().X) < std::abs(sample.getDeltaProperty().Y)) {
                        isTracking_ = false;
                        velocity_.X = -sample.getDeltaProperty().X;
                        velocity_.Y = -sample.getDeltaProperty().Y;
                    }
                    break;

                case GestureType::DragComplete:
                    isTracking_ = false;
                    break;

                default:
                    break;
            }
        }
    }

private:
    // How far the user is allowed to drag past the "real" border.
    static constexpr float SpringMaxDrag = 400.0f;
    // How far the display moves when dragged to SpringMaxDrag.
    static constexpr float SpringMaxOffset = SpringMaxDrag / 3.0f;
    static constexpr float SpringReturnRate = 0.1f;
    static constexpr float SpringReturnMin = 2.0f;
    static constexpr float Deceleration = 500.0f; // pixels/second^2
    static constexpr float MaxVelocity = 2000.0f; // pixels/second

    // If x is within (min,max), return x unchanged. Otherwise return a value
    // outside (min,max) but only partway to where x is -- the partial-overdrag
    // effect at the edges of the list.
    static float SoftClamp(float x, float min, float max) {
        if (x < min) {
            return std::max(x - min, -SpringMaxDrag) * SpringMaxOffset / SpringMaxDrag + min;
        }
        if (x > max) {
            return std::min(x - max, SpringMaxDrag) * SpringMaxOffset / SpringMaxDrag + max;
        }
        return x;
    }

    // Integrates the given position and velocity over a timespan. min/max
    // give the soft limits the position is allowed to move to.
    static void ApplyVelocity(float dt, float& x, float& v, float min, float max) {
        x += v * dt;

        v = MathHelper::Clamp(v, -MaxVelocity, MaxVelocity);
        v = std::max(std::abs(v) - dt * Deceleration, 0.0f) * (v < 0 ? -1.0f : (v > 0 ? 1.0f : 0.0f));

        if (x < min) {
            x = std::min(x + (min - x) * SpringReturnRate + SpringReturnMin, min);
            v = 0.0f;
        }
        if (x > max) {
            x = std::max(x - (x - max) * SpringReturnRate - SpringReturnMin, max);
            v = 0.0f;
        }
    }

    Vector2 velocity_;
    Vector2 viewOrigin_;
    Vector2 unclampedViewOrigin_;
    bool isTracking_ = false;
};

} // namespace UISample::Controls

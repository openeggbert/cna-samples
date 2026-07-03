#pragma once

#include <functional>
#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"

namespace DynamicMenu::Controls { class Control; }

namespace DynamicMenu::Transitions {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Point;

// Transitions apply an animated effect to a control: its size, position, and/or
// color can be changed smoothly over a given period of time. Port of
// Transitions/Transition.cs.
//
// StartTransition()/Update() and the CreateXxx() factory methods are defined
// out-of-line at the bottom of Control.hpp, once Controls::Control (forward-
// declared above) is a complete type -- the same forward-declare-then-define-
// out-of-line pattern GameStateManagement's Screens.hpp uses for its own
// mutually-referencing types.
class Transition {
public:
    // Event raised once a transition finishes (mirrors the original's
    // TransitionComplete C# event).
    std::function<void(Transition&)> TransitionComplete;

    Controls::Control* Control = nullptr;
    float TransitionLength = 1.0f;

    // Each parameter is optional, matching the original's nullable Point?/Color?
    // constructor parameters -- omitting one means that aspect of the control is
    // left unchanged by this transition.
    Transition(std::optional<Point> startPosition, std::optional<Point> endPosition,
               std::optional<Point> startSize, std::optional<Point> endSize,
               std::optional<Color> startColor, std::optional<Color> endColor) {
        if (startPosition) SetStartPosition(*startPosition);
        if (endPosition) SetEndPosition(*endPosition);
        if (startSize) SetStartSize(*startSize);
        if (endSize) SetEndSize(*endSize);
        if (startColor) SetStartColor(*startColor);
        if (endColor) SetEndColor(*endColor);
    }

    void SetStartPosition(Point value) { startPositionSet_ = true; startPosition_ = value; }
    void SetEndPosition(Point value) { endPositionSet_ = true; endPosition_ = value; }
    void SetStartSize(Point value) { startSizeSet_ = true; startSize_ = value; }
    void SetEndSize(Point value) { endSizeSet_ = true; endSize_ = value; }
    void SetStartColor(Color value) { startHueSet_ = true; startHue_ = value; }
    void SetEndColor(Color value) { endHueSet_ = true; endHue_ = value; }

    [[nodiscard]] bool IsTransitionActive() const { return transitionActive_; }

    // Begins applying the transition to Control. Any start value not explicitly
    // set is captured from the control's current state instead.
    void StartTransition();

    // Advances the transition; called once per frame while active.
    void Update(const GameTime& gameTime);

    // Transition creation methods, mirroring the originals in Transition.cs.
    static Transition CreateFadeIn(Controls::Control& control, std::optional<float> length);
    static Transition CreateFadeOut(Controls::Control& control, std::optional<float> length);
    static Transition CreateFlyIn(Controls::Control& control, Point startPos, std::optional<float> length);
    static Transition CreateFlyOut(Controls::Control& control, Point endPos, std::optional<float> length);

    bool startPositionSet_ = false;
    bool endPositionSet_ = false;
    bool startSizeSet_ = false;
    bool endSizeSet_ = false;
    bool startHueSet_ = false;
    bool endHueSet_ = false;

    Point startPosition_;
    Point endPosition_;
    Point startSize_;
    Point endSize_;
    Color startHue_{255, 255, 255, 255};
    Color endHue_{255, 255, 255, 255};

    bool transitionActive_ = false;
    double transitionStartTime_ = 0.0;
};

} // namespace DynamicMenu::Transitions

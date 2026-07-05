#pragma once

// AnimatedGameComponentAnimation.hpp -- C++ port of
// UI/AnimatedGameComponentAnimation.cs (XNA 4.0 CardsStarterKit sample).
// Represents an animation that can alter an animated component. Uses real
// wall-clock time (System::DateTime::getNowProperty()) exactly like the C#
// original, so animations can be scheduled to start at a future instant
// (used heavily to stagger dealt-card animations).

#include <functional>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "System/DateTime.hpp"
#include "System/TimeSpan.hpp"

namespace CardsFramework {

using Microsoft::Xna::Framework::GameTime;
using System::DateTime;
using System::TimeSpan;

class AnimatedGameComponent;  // forward declaration

class AnimatedGameComponentAnimation {
public:
    AnimatedGameComponent* Component = nullptr;

    // An action to perform before the animation begins / once it completes.
    std::function<void(void*)> PerformBeforeStart;
    void* PerformBeforeStartArgs = nullptr;
    std::function<void(void*)> PerformWhenDone;
    void* PerformWhenDoneArgs = nullptr;

    unsigned AnimationCycles() const { return animationCycles_; }
    void setAnimationCycles(unsigned value) {
        if (value > 0) animationCycles_ = value;
    }

    DateTime StartTime;
    TimeSpan Duration = TimeSpan::FromMilliseconds(150);
    bool IsLooped = false;

    AnimatedGameComponentAnimation() : StartTime(DateTime::getNowProperty()) {}
    virtual ~AnimatedGameComponentAnimation() = default;

    // Returns the time at which the animation is estimated to end.
    TimeSpan EstimatedTimeForAnimationCompletion() const {
        if (isStarted_)
            return Duration - Elapsed;
        return StartTime - DateTime::getNowProperty() + Duration;
    }

    // Whether the animation is done playing. Looped animations never finish.
    bool IsDone() {
        if (!isDone_) {
            isDone_ = !IsLooped && (Elapsed >= Duration);
            if (isDone_ && PerformWhenDone) {
                PerformWhenDone(PerformWhenDoneArgs);
                PerformWhenDone = nullptr;
            }
        }
        return isDone_;
    }

    // Whether the animation has started. As a side-effect, starts the
    // animation if it is not started and it is time for it to start.
    bool IsStarted() {
        if (!isStarted_) {
            if (StartTime <= DateTime::getNowProperty()) {
                if (PerformBeforeStart) {
                    PerformBeforeStart(PerformBeforeStartArgs);
                    PerformBeforeStart = nullptr;
                }
                StartTime = DateTime::getNowProperty();
                isStarted_ = true;
            }
        }
        return isStarted_;
    }

    // Increases the elapsed time seen by the animation, but only once started.
    void AccumulateElapsedTime(TimeSpan elapsedTime) {
        if (isStarted_)
            Elapsed = Elapsed + elapsedTime;
    }

    virtual void Run(GameTime& gameTime) { (void)gameTime; IsStarted(); }

protected:
    TimeSpan Elapsed = TimeSpan::Zero;

private:
    unsigned animationCycles_ = 1;
    bool isDone_ = false;
    bool isStarted_ = false;
};

} // namespace CardsFramework

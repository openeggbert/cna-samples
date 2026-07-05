#pragma once

// GameScreen.hpp -- C++ port of ScreenManager/GameScreen.cs. Unlike other
// ScreenManager ports in this repo (which route input through a per-frame
// InputState with a Gestures/mouse fallback), this sample has no InputState.cs
// at all in the original -- HandleInput() takes no parameters and reads global
// InputManager state directly (this is a Windows+Xbox sample; RPG movement/menu
// navigation goes entirely through InputManager's keyboard+gamepad Action map).

#include <functional>
#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "System/TimeSpan.hpp"

namespace RolePlaying {

using Microsoft::Xna::Framework::GameTime;
using System::TimeSpan;

class ScreenManager; // fwd decl, see ScreenManager.hpp

enum class ScreenState { TransitionOn, Active, TransitionOff, Hidden };

// A screen is a single layer that has update and draw logic, and which can be
// combined with other layers to build up a complex menu system.
class GameScreen {
public:
    virtual ~GameScreen() = default;

    bool IsPopup() const { return isPopup_; }

    TimeSpan TransitionOnTime() const { return transitionOnTime_; }
    TimeSpan TransitionOffTime() const { return transitionOffTime_; }
    float TransitionPosition() const { return transitionPosition_; }
    unsigned char TransitionAlpha() const { return (unsigned char)(255 - TransitionPosition() * 255); }
    ScreenState GetScreenState() const { return screenState_; }

    bool IsExiting() const { return isExiting_; }
    void SetIsExiting(bool v) {
        bool fireEvent = !isExiting_ && v;
        isExiting_ = v;
        if (fireEvent && Exiting) Exiting();
    }

    // Fired the moment IsExiting transitions false->true (matches the
    // original's Exiting event) -- not when the screen finally finishes its
    // transition-off and is actually removed.
    std::function<void()> Exiting;

    bool IsActive() const {
        return !otherScreenHasFocus_ && (screenState_ == ScreenState::TransitionOn || screenState_ == ScreenState::Active);
    }

    ScreenManager* GetScreenManager() const { return screenManager_; }
    void SetScreenManager(ScreenManager* v) { screenManager_ = v; }

    virtual void LoadContent() {}
    virtual void UnloadContent() {}

    // Unlike HandleInput, this is called regardless of screen state.
    virtual void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen) {
        otherScreenHasFocus_ = otherScreenHasFocus;

        if (IsExiting()) {
            screenState_ = ScreenState::TransitionOff;
            if (!UpdateTransition(gameTime, transitionOffTime_, 1)) RemoveFromScreenManager();
        } else if (coveredByOtherScreen) {
            screenState_ = UpdateTransition(gameTime, transitionOffTime_, 1) ? ScreenState::TransitionOff : ScreenState::Hidden;
        } else {
            screenState_ = UpdateTransition(gameTime, transitionOnTime_, -1) ? ScreenState::TransitionOn : ScreenState::Active;
        }
    }

    // Only called when the screen is active (not covered/hidden). Reads global
    // InputManager state directly -- see file header.
    virtual void HandleInput() {}

    virtual void Draw(const GameTime& gameTime) { (void)gameTime; }

    // Tells the screen to go away, respecting transition timings.
    void ExitScreen();

protected:
    void SetIsPopup(bool v) { isPopup_ = v; }
    void SetTransitionOnTime(TimeSpan v) { transitionOnTime_ = v; }
    void SetTransitionOffTime(TimeSpan v) { transitionOffTime_ = v; }

private:
    void RemoveFromScreenManager(); // defined in ScreenManager.hpp

    bool UpdateTransition(GameTime& gameTime, TimeSpan time, int direction) {
        float transitionDelta;
        if (time.getTotalMillisecondsProperty() == 0.0)
            transitionDelta = 1.0f;
        else
            transitionDelta = (float)(gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty() /
                                       time.getTotalMillisecondsProperty());

        transitionPosition_ += transitionDelta * direction;
        if (transitionPosition_ <= 0.0f || transitionPosition_ >= 1.0f) {
            transitionPosition_ = Microsoft::Xna::Framework::MathHelper::Clamp(transitionPosition_, 0.0f, 1.0f);
            return false;
        }
        return true;
    }

    bool isPopup_ = false;
    TimeSpan transitionOnTime_ = TimeSpan::Zero;
    TimeSpan transitionOffTime_ = TimeSpan::Zero;
    float transitionPosition_ = 1.0f;
    ScreenState screenState_ = ScreenState::TransitionOn;
    bool isExiting_ = false;
    bool otherScreenHasFocus_ = false;
    ScreenManager* screenManager_ = nullptr;
};

} // namespace RolePlaying

#pragma once

// GameScreen.hpp — C++ port of ScreenManager/GameScreen.cs (XNA 4.0
// HoneycombRush sample). Tombstoning (Serialize/Deserialize/IsSerializable)
// is dropped, matching this project's established precedent for the same
// ScreenManager framework in other samples (e.g. UISample) — see missing.md.

#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/TimeSpan.hpp"

#include "InputState.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;
using System::TimeSpan;

class ScreenManager; // forward declaration

// XNA's "Color * float" multiplies every channel by the scalar (premultiplied
// fade). CNA's Color has no operator*, so this helper reproduces it.
inline Color mul(const Color& c, float s) {
    auto ch = [](int v, float s) {
        int i = (int)((float)v * s + 0.5f);
        return i < 0 ? 0 : (i > 255 ? 255 : i);
    };
    return Color(ch(c.getRProperty(), s), ch(c.getGProperty(), s), ch(c.getBProperty(), s), ch(c.getAProperty(), s));
}

// Describes the screen transition state.
enum class ScreenState {
    TransitionOn,
    Active,
    TransitionOff,
    Hidden,
};

// A screen is a single layer that has update and draw logic, and which can be
// combined with other layers to build up a complex menu system. Port of
// ScreenManager/GameScreen.cs.
class GameScreen {
public:
    virtual ~GameScreen() = default;

    bool IsPopup() const { return isPopup_; }

    TimeSpan TransitionOnTime() const { return transitionOnTime_; }
    TimeSpan TransitionOffTime() const { return transitionOffTime_; }

    float TransitionPosition() const { return transitionPosition_; }
    float TransitionAlpha() const { return 1.0f - transitionPosition_; }

    ScreenState GetScreenState() const { return screenState_; }

    bool IsExiting() const { return isExiting_; }
    void setIsExiting(bool value) { isExiting_ = value; }

    // Checks whether this screen is active and can respond to user input.
    bool IsActive() const {
        return !otherScreenHasFocus_ && (screenState_ == ScreenState::Active);
    }

    ScreenManager* GetScreenManager() const { return screenManager_; }
    void setScreenManager(ScreenManager* value) { screenManager_ = value; }

    std::optional<PlayerIndex> ControllingPlayer() const { return controllingPlayer_; }
    void setControllingPlayer(std::optional<PlayerIndex> value) { controllingPlayer_ = value; }

    GestureType EnabledGestures() const { return enabledGestures_; }
    void setEnabledGestures(GestureType value) {
        enabledGestures_ = value;
        if (screenState_ == ScreenState::Active) {
            TouchPanel::setEnabledGesturesProperty(value);
        }
    }

    virtual void LoadContent() {}
    virtual void UnloadContent() {}

    // Allows the screen to run logic, such as updating the transition position.
    // Defined out-of-line in ScreenManager.hpp (it calls ScreenManager).
    virtual void Update(GameTime& gameTime, bool otherScreenHasFocus, bool coveredByOtherScreen);

    virtual void HandleInput(GameTime& gameTime, InputState& input) {
        (void)gameTime;
        (void)input;
    }

    virtual void Draw(const GameTime& gameTime) { (void)gameTime; }

    // Tells the screen to go away, respecting the transition timings.
    // Defined out-of-line in ScreenManager.hpp.
    void ExitScreen();

    // Loads an asset using the screen manager's content loader. Defined
    // out-of-line in ScreenManager.hpp.
    template <typename T>
    T Load(const std::string& assetName);

protected:
    void setIsPopup(bool value) { isPopup_ = value; }
    void setTransitionOnTime(TimeSpan value) { transitionOnTime_ = value; }
    void setTransitionOffTime(TimeSpan value) { transitionOffTime_ = value; }
    void setScreenState(ScreenState value) { screenState_ = value; }

    // Helper for updating the screen transition position. Self-contained.
    bool UpdateTransition(GameTime& gameTime, TimeSpan time, int direction) {
        float transitionDelta;
        if (time.getTotalMillisecondsProperty() <= 0.0)
            transitionDelta = 1.0f;
        else
            transitionDelta = (float)(gameTime.getElapsedGameTimeProperty().getTotalMillisecondsProperty() /
                                       time.getTotalMillisecondsProperty());

        transitionPosition_ += transitionDelta * direction;

        if (((direction < 0) && (transitionPosition_ <= 0)) || ((direction > 0) && (transitionPosition_ >= 1))) {
            transitionPosition_ = MathHelper::Clamp(transitionPosition_, 0.0f, 1.0f);
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
    std::optional<PlayerIndex> controllingPlayer_;
    GestureType enabledGestures_ = GestureType::None;
};

} // namespace HoneycombRush

#pragma once

#include <optional>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "System/TimeSpan.hpp"

#include "InputState.hpp"

namespace CatapultWars {

using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Color;
using System::TimeSpan;

class ScreenManager;  // forward declaration
} // namespace CatapultWars

namespace Microsoft::Xna::Framework::Content { class ContentManager; }

namespace CatapultWars {

// XNA's "Color * float" multiplies every channel by the scalar (premultiplied
// fade). CNA's Color has no operator*, so this helper reproduces it.
inline Color mul(const Color& c, float s) {
    auto ch = [](int v, float s) {
        int i = static_cast<int>(v * s + 0.5f);
        return i < 0 ? 0 : (i > 255 ? 255 : i);
    };
    return Color(ch(c.getRProperty(), s), ch(c.getGProperty(), s),
                 ch(c.getBProperty(), s), ch(c.getAProperty(), s));
}

// Describes the screen transition state.
enum class ScreenState {
    TransitionOn,
    Active,
    TransitionOff,
    Hidden,
};

// A screen is a single layer that has update and draw logic, and which can be
// combined with other layers to build up a complex menu system.
class GameScreen {
public:
    virtual ~GameScreen() = default;

    bool IsPopup() const { return isPopup_; }

    TimeSpan TransitionOnTime() const { return transitionOnTime_; }
    TimeSpan TransitionOffTime() const { return transitionOffTime_; }

    float TransitionPosition() const { return transitionPosition_; }
    float TransitionAlpha() const { return 1.0f - transitionPosition_; }

    // XNA exposes TransitionAlpha as a byte (0..255); some screens build a
    // Color(255, 255, 255, TransitionAlpha) directly.
    int TransitionAlphaByte() const {
        return static_cast<int>(255 - transitionPosition_ * 255);
    }

    ScreenState GetScreenState() const { return screenState_; }

    bool IsExiting() const { return isExiting_; }
    void setIsExiting(bool value) { isExiting_ = value; }

    // Checks whether this screen is active and can respond to user input.
    bool IsActive() const {
        return !otherScreenHasFocus_ &&
               (screenState_ == ScreenState::TransitionOn ||
                screenState_ == ScreenState::Active);
    }

    ScreenManager* GetScreenManager() const { return screenManager_; }
    void setScreenManager(ScreenManager* value) { screenManager_ = value; }

    std::optional<PlayerIndex> ControllingPlayer() const { return controllingPlayer_; }
    void setControllingPlayer(std::optional<PlayerIndex> value) { controllingPlayer_ = value; }

    virtual void LoadContent() {}
    virtual void UnloadContent() {}

    // Allows the screen to run logic, such as updating the transition position.
    // Defined out-of-line in ScreenManager.hpp (it calls ScreenManager).
    virtual void Update(GameTime& gameTime, bool otherScreenHasFocus,
                        bool coveredByOtherScreen);

    virtual void HandleInput(InputState& input) { (void)input; }

    virtual void Draw(const GameTime& gameTime) { (void)gameTime; }

    // Tells the screen to go away, respecting the transition timings.
    // Defined out-of-line in ScreenManager.hpp.
    void ExitScreen();

    // Convenience accessor for the game's ContentManager.
    // Defined out-of-line in ScreenManager.hpp.
    Microsoft::Xna::Framework::Content::ContentManager& Content();

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

        if (((direction < 0) && (transitionPosition_ <= 0)) ||
            ((direction > 0) && (transitionPosition_ >= 1))) {
            transitionPosition_ = MathHelper::Clamp(transitionPosition_, 0.0f, 1.0f);
            return false;
        }
        return true;
    }

    void setOtherScreenHasFocus(bool v) { otherScreenHasFocus_ = v; }

    bool isPopup_ = false;
    TimeSpan transitionOnTime_ = TimeSpan::Zero;
    TimeSpan transitionOffTime_ = TimeSpan::Zero;
    float transitionPosition_ = 1.0f;
    ScreenState screenState_ = ScreenState::TransitionOn;
    bool isExiting_ = false;
    bool otherScreenHasFocus_ = false;
    ScreenManager* screenManager_ = nullptr;
    std::optional<PlayerIndex> controllingPlayer_;
};

} // namespace CatapultWars

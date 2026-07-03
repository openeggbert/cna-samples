#pragma once

#include <array>
#include <optional>
#include <vector>

#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchCollection.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocation.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocationState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/TimeSpan.hpp"

namespace UISample {

using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchCollection;
using Microsoft::Xna::Framework::Input::Touch::TouchLocation;
using Microsoft::Xna::Framework::Input::Touch::TouchLocationState;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// Helper for reading input from keyboard, gamepad, and touch. Tracks both the
// current and previous state of the input devices, and implements query
// methods for high level input actions such as "menu select" or "pause the
// game". Port of ScreenManager/InputState.cs.
class InputState {
public:
    static const int MaxInputs = 4;

    std::array<KeyboardState, MaxInputs> CurrentKeyboardStates;
    std::array<GamePadState, MaxInputs> CurrentGamePadStates;

    std::array<KeyboardState, MaxInputs> LastKeyboardStates;
    std::array<GamePadState, MaxInputs> LastGamePadStates;

    std::array<bool, MaxInputs> GamePadWasConnected{};

    TouchCollection TouchState;
    std::vector<GestureSample> Gestures;

    InputState() = default;

    // Reads the latest state of the keyboard, gamepad, and touch panel.
    void Update() {
        for (int i = 0; i < MaxInputs; i++) {
            LastKeyboardStates[i] = CurrentKeyboardStates[i];
            LastGamePadStates[i] = CurrentGamePadStates[i];

            CurrentKeyboardStates[i] = Keyboard::GetState(static_cast<PlayerIndex>(i));
            CurrentGamePadStates[i] = GamePad::GetState(static_cast<PlayerIndex>(i));

            if (CurrentGamePadStates[i].getIsConnectedProperty())
                GamePadWasConnected[i] = true;
        }

        TouchState = TouchPanel::GetState();

        Gestures.clear();
        while (TouchPanel::getIsGestureAvailableProperty()) {
            Gestures.push_back(TouchPanel::ReadGesture());
        }

        UpdateMouseFallback();
    }

    // Helper for checking if a key was newly pressed during this update.
    bool IsNewKeyPress(Keys key, std::optional<PlayerIndex> controllingPlayer,
                       PlayerIndex& playerIndex) {
        if (controllingPlayer.has_value()) {
            playerIndex = controllingPlayer.value();
            int i = static_cast<int>(playerIndex);
            return CurrentKeyboardStates[i].IsKeyDown(key) &&
                   LastKeyboardStates[i].IsKeyUp(key);
        }
        return IsNewKeyPress(key, PlayerIndex::One, playerIndex) ||
               IsNewKeyPress(key, PlayerIndex::Two, playerIndex) ||
               IsNewKeyPress(key, PlayerIndex::Three, playerIndex) ||
               IsNewKeyPress(key, PlayerIndex::Four, playerIndex);
    }

    // Helper for checking if a button was newly pressed during this update.
    bool IsNewButtonPress(Buttons button, std::optional<PlayerIndex> controllingPlayer,
                          PlayerIndex& playerIndex) {
        if (controllingPlayer.has_value()) {
            playerIndex = controllingPlayer.value();
            int i = static_cast<int>(playerIndex);
            return CurrentGamePadStates[i].IsButtonDown(button) &&
                   LastGamePadStates[i].IsButtonUp(button);
        }
        return IsNewButtonPress(button, PlayerIndex::One, playerIndex) ||
               IsNewButtonPress(button, PlayerIndex::Two, playerIndex) ||
               IsNewButtonPress(button, PlayerIndex::Three, playerIndex) ||
               IsNewButtonPress(button, PlayerIndex::Four, playerIndex);
    }

    // Checks for a "menu select" input action.
    bool IsMenuSelect(std::optional<PlayerIndex> controllingPlayer, PlayerIndex& playerIndex) {
        return IsNewKeyPress(Keys::Space, controllingPlayer, playerIndex) ||
               IsNewKeyPress(Keys::Enter, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::A, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::Start, controllingPlayer, playerIndex);
    }

    // Checks for a "menu cancel" input action.
    bool IsMenuCancel(std::optional<PlayerIndex> controllingPlayer, PlayerIndex& playerIndex) {
        return IsNewKeyPress(Keys::Escape, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::B, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::Back, controllingPlayer, playerIndex);
    }

    // Checks for a "menu up" input action.
    bool IsMenuUp(std::optional<PlayerIndex> controllingPlayer) {
        PlayerIndex playerIndex;
        return IsNewKeyPress(Keys::Up, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::DPadUp, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::LeftThumbstickUp, controllingPlayer, playerIndex);
    }

    // Checks for a "menu down" input action.
    bool IsMenuDown(std::optional<PlayerIndex> controllingPlayer) {
        PlayerIndex playerIndex;
        return IsNewKeyPress(Keys::Down, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::DPadDown, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::LeftThumbstickDown, controllingPlayer, playerIndex);
    }

    // Checks for a "pause the game" input action.
    bool IsPauseGame(std::optional<PlayerIndex> controllingPlayer) {
        PlayerIndex playerIndex;
        return IsNewKeyPress(Keys::Escape, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::Back, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::Start, controllingPlayer, playerIndex);
    }

private:
    // CNA addition (see missing.md): this whole sample is touch/gesture-driven
    // (MenuScreen taps, ScrollTracker/PageFlipTracker drags), and CNA does not
    // synthesize touch or gestures from mouse input. Centralizing the mouse
    // fallback here, in the one shared InputState all screens/controls read
    // from, means MenuScreen, ScrollTracker, and PageFlipTracker all get mouse
    // support for free with no changes to their own ported logic.
    //
    // A press synthesizes a Tap (for MenuScreen) and a raw TouchLocation in
    // TouchState with state Pressed (the "just touched down" bootstrap signal
    // ScrollTracker's IsTracking check needs -- Gestures alone can't express a
    // zero-delta initial contact). While held, each frame's mouse movement
    // synthesizes HorizontalDrag/VerticalDrag gestures from the frame-to-frame
    // delta. Release synthesizes DragComplete. Flick (a fast release while
    // still moving) is deliberately not approximated -- only a deliberate
    // drag-then-release, which both trackers already settle/snap correctly
    // on without it.
    void UpdateMouseFallback() {
        MouseState mouse = Mouse::GetState();
        bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        Vector2 mousePos((float)mouse.getXProperty(), (float)mouse.getYProperty());

        if (mouseDown && !prevMouseDown_) {
            TouchState = TouchCollection(std::vector<TouchLocation>{
                TouchLocation(0, TouchLocationState::Pressed, mousePos)});
            Gestures.emplace_back(GestureType::Tap, System::TimeSpan::Zero, mousePos,
                                   Vector2::Zero, Vector2::Zero, Vector2::Zero);
        } else if (mouseDown) {
            Vector2 delta = mousePos - lastMousePos_;
            if (delta.X != 0.0f) {
                Gestures.emplace_back(GestureType::HorizontalDrag, System::TimeSpan::Zero, mousePos,
                                       Vector2::Zero, Vector2(delta.X, 0.0f), Vector2::Zero);
            }
            if (delta.Y != 0.0f) {
                Gestures.emplace_back(GestureType::VerticalDrag, System::TimeSpan::Zero, mousePos,
                                       Vector2::Zero, Vector2(0.0f, delta.Y), Vector2::Zero);
            }
        } else if (prevMouseDown_) {
            Gestures.emplace_back(GestureType::DragComplete, System::TimeSpan::Zero, mousePos,
                                   Vector2::Zero, Vector2::Zero, Vector2::Zero);
        }

        prevMouseDown_ = mouseDown;
        lastMousePos_ = mousePos;
    }

    bool prevMouseDown_ = false;
    Vector2 lastMousePos_;
};

} // namespace UISample

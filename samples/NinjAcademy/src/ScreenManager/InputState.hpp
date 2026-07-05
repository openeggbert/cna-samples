#pragma once

// InputState.hpp — C++ port of ScreenManager/InputState.cs (XNA 4.0
// NinjAcademy sample). Helper for reading input from keyboard, gamepad, and
// touch. CNA does not synthesize touch/gestures from mouse input on this
// desktop, so a left click is synthesized into a Tap gesture here -- this
// project's established "every interaction is a discrete tap" fallback (see
// NEXT.md section 6, pattern 3/4) -- giving every menu and the gameplay
// screen's throw/slash handling mouse support for free. See missing.md.

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
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/TimeSpan.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// Helper for reading input from keyboard, gamepad, and touch input. Port of
// ScreenManager/InputState.cs.
class InputState {
public:
    static const int MaxInputs = 4;

    std::array<KeyboardState, MaxInputs> CurrentKeyboardStates;
    std::array<GamePadState, MaxInputs> CurrentGamePadStates;

    std::array<KeyboardState, MaxInputs> LastKeyboardStates;
    std::array<GamePadState, MaxInputs> LastGamePadStates;

    std::array<bool, MaxInputs> GamePadWasConnected{};

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

        Gestures.clear();
        while (TouchPanel::getIsGestureAvailableProperty()) {
            Gestures.push_back(TouchPanel::ReadGesture());
        }

        UpdateMouseFallback();
    }

    // Helper for checking if a key was newly pressed during this update.
    bool IsNewKeyPress(Keys key, std::optional<PlayerIndex> controllingPlayer, PlayerIndex& playerIndex) {
        if (controllingPlayer.has_value()) {
            playerIndex = controllingPlayer.value();
            int i = static_cast<int>(playerIndex);
            return CurrentKeyboardStates[i].IsKeyDown(key) && LastKeyboardStates[i].IsKeyUp(key);
        }
        return IsNewKeyPress(key, PlayerIndex::One, playerIndex) ||
               IsNewKeyPress(key, PlayerIndex::Two, playerIndex) ||
               IsNewKeyPress(key, PlayerIndex::Three, playerIndex) ||
               IsNewKeyPress(key, PlayerIndex::Four, playerIndex);
    }

    // Helper for checking if a key is currently pressed down.
    bool IsKeyDown(Keys key, std::optional<PlayerIndex> controllingPlayer, PlayerIndex& playerIndex) {
        if (controllingPlayer.has_value()) {
            playerIndex = controllingPlayer.value();
            int i = static_cast<int>(playerIndex);
            return CurrentKeyboardStates[i].IsKeyDown(key);
        }
        return IsKeyDown(key, PlayerIndex::One, playerIndex) || IsKeyDown(key, PlayerIndex::Two, playerIndex) ||
               IsKeyDown(key, PlayerIndex::Three, playerIndex) || IsKeyDown(key, PlayerIndex::Four, playerIndex);
    }

    // Helper for checking if a button was newly pressed during this update.
    bool IsNewButtonPress(Buttons button, std::optional<PlayerIndex> controllingPlayer, PlayerIndex& playerIndex) {
        if (controllingPlayer.has_value()) {
            playerIndex = controllingPlayer.value();
            int i = static_cast<int>(playerIndex);
            return CurrentGamePadStates[i].IsButtonDown(button) && LastGamePadStates[i].IsButtonUp(button);
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
               IsNewKeyPress(Keys::Left, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::DPadLeft, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::LeftThumbstickLeft, controllingPlayer, playerIndex);
    }

    // Checks for a "menu down" input action.
    bool IsMenuDown(std::optional<PlayerIndex> controllingPlayer) {
        PlayerIndex playerIndex;
        return IsNewKeyPress(Keys::Down, controllingPlayer, playerIndex) ||
               IsNewKeyPress(Keys::Right, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::DPadRight, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::LeftThumbstickRight, controllingPlayer, playerIndex);
    }

    // Checks for a "pause the game" input action.
    bool IsPauseGame(std::optional<PlayerIndex> controllingPlayer) {
        PlayerIndex playerIndex;
        return IsNewKeyPress(Keys::Escape, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::Back, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::Start, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::BigButton, controllingPlayer, playerIndex);
    }

private:
    // CNA addition (see missing.md): synthesizes a Tap gesture on a mouse
    // left-click rising edge (for menu selection and throwing a shuriken),
    // and a FreeDrag gesture each frame the button is held (for sword
    // slashes, which GameplayScreen tracks via gesture.Position) -- matching
    // this project's established mouse fallback for tap/drag-driven samples
    // (DynamicMenu/UISample precedent).
    void UpdateMouseFallback() {
        MouseState mouse = Mouse::GetState();
        bool mouseDown = mouse.getLeftButtonProperty() == ButtonState::Pressed;
        Vector2 mousePos((float)mouse.getXProperty(), (float)mouse.getYProperty());

        if (mouseDown && !prevMouseDown_) {
            Gestures.emplace_back(GestureType::Tap, System::TimeSpan::Zero, mousePos, Vector2::Zero, Vector2::Zero,
                                   Vector2::Zero);
        } else if (mouseDown) {
            Vector2 delta = mousePos - lastMousePos_;
            Gestures.emplace_back(GestureType::FreeDrag, System::TimeSpan::Zero, mousePos, Vector2::Zero, delta,
                                   Vector2::Zero);
        }
        prevMouseDown_ = mouseDown;
        lastMousePos_ = mousePos;
    }

    bool prevMouseDown_ = false;
    Vector2 lastMousePos_;
};

} // namespace NinjAcademy

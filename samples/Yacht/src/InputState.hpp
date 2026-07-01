#pragma once

#include <array>
#include <optional>
#include <vector>

#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchCollection.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;
using Microsoft::Xna::Framework::Input::Touch::TouchCollection;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

// Helper for reading input from keyboard, gamepad, touch and mouse. Ported
// from the stock GameStateManagement-style InputState.cs that ships inside
// Yacht's own ScreenManager/ folder. The WINDOWS_PHONE-only Gestures/
// TouchState fields are un-#ifdef'd here since CNA's TouchPanel is a real,
// SDL3-backed implementation; mouse fields are added as an explicit
// parallel input source, matching the CatapultWars precedent (see
// missing.md).
class InputState {
public:
    static const int MaxInputs = 4;

    std::array<KeyboardState, MaxInputs> CurrentKeyboardStates;
    std::array<GamePadState,  MaxInputs> CurrentGamePadStates;

    std::array<KeyboardState, MaxInputs> LastKeyboardStates;
    std::array<GamePadState,  MaxInputs> LastGamePadStates;

    std::array<bool, MaxInputs> GamePadWasConnected{};

    TouchCollection TouchState;
    std::vector<GestureSample> Gestures;

    MouseState CurrentMouseState;
    MouseState LastMouseState;

    InputState() = default;

    // Reads the latest state of the keyboard, gamepad, touch panel and mouse.
    void Update() {
        for (int i = 0; i < MaxInputs; i++) {
            LastKeyboardStates[i] = CurrentKeyboardStates[i];
            LastGamePadStates[i]  = CurrentGamePadStates[i];

            CurrentKeyboardStates[i] = Keyboard::GetState(static_cast<PlayerIndex>(i));
            CurrentGamePadStates[i]  = GamePad::GetState(static_cast<PlayerIndex>(i));

            if (CurrentGamePadStates[i].getIsConnectedProperty())
                GamePadWasConnected[i] = true;
        }

        TouchState = TouchPanel::GetState();

        Gestures.clear();
        while (TouchPanel::getIsGestureAvailableProperty()) {
            Gestures.push_back(TouchPanel::ReadGesture());
        }

        LastMouseState = CurrentMouseState;
        CurrentMouseState = Mouse::GetState();
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

    bool IsMenuSelect(std::optional<PlayerIndex> controllingPlayer, PlayerIndex& playerIndex) {
        return IsNewKeyPress(Keys::Space, controllingPlayer, playerIndex) ||
               IsNewKeyPress(Keys::Enter, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::A, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::Start, controllingPlayer, playerIndex);
    }

    bool IsMenuCancel(std::optional<PlayerIndex> controllingPlayer, PlayerIndex& playerIndex) {
        return IsNewKeyPress(Keys::Escape, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::B, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::Back, controllingPlayer, playerIndex);
    }

    // NOTE: ported verbatim from Yacht's own ScreenManager/InputState.cs --
    // "up" checks both Up and Left (and "down" checks both Down and Right).
    // That is the original sample's own (seemingly copy-pasted from a
    // horizontal-menu template) behaviour; kept as-is for fidelity.
    bool IsMenuUp(std::optional<PlayerIndex> controllingPlayer) {
        PlayerIndex playerIndex;
        return IsNewKeyPress(Keys::Up, controllingPlayer, playerIndex) ||
               IsNewKeyPress(Keys::Left, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::DPadLeft, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::LeftThumbstickLeft, controllingPlayer, playerIndex);
    }

    bool IsMenuDown(std::optional<PlayerIndex> controllingPlayer) {
        PlayerIndex playerIndex;
        return IsNewKeyPress(Keys::Down, controllingPlayer, playerIndex) ||
               IsNewKeyPress(Keys::Right, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::DPadRight, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::LeftThumbstickRight, controllingPlayer, playerIndex);
    }

    bool IsPauseGame(std::optional<PlayerIndex> controllingPlayer) {
        PlayerIndex playerIndex;
        return IsNewKeyPress(Keys::Escape, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::Back, controllingPlayer, playerIndex) ||
               IsNewButtonPress(Buttons::Start, controllingPlayer, playerIndex);
    }

    // ---- Mouse helpers (desktop parallel to touch, per CatapultWars precedent) ----

    Vector2 MousePosition() const {
        return Vector2(static_cast<float>(CurrentMouseState.getXProperty()),
                       static_cast<float>(CurrentMouseState.getYProperty()));
    }

    bool IsLeftMouseDown() const {
        return CurrentMouseState.getLeftButtonProperty() == ButtonState::Pressed;
    }

    bool IsNewLeftMousePress() const {
        return CurrentMouseState.getLeftButtonProperty() == ButtonState::Pressed &&
               LastMouseState.getLeftButtonProperty() == ButtonState::Released;
    }

    bool IsNewLeftMouseRelease() const {
        return CurrentMouseState.getLeftButtonProperty() == ButtonState::Released &&
               LastMouseState.getLeftButtonProperty() == ButtonState::Pressed;
    }

    // Per-frame mouse movement delta -- the mouse-only analogue of a
    // GestureSample's Delta field, used for scrolling the scorecard.
    Vector2 MouseDelta() const {
        return Vector2(static_cast<float>(CurrentMouseState.getXProperty() - LastMouseState.getXProperty()),
                       static_cast<float>(CurrentMouseState.getYProperty() - LastMouseState.getYProperty()));
    }
};

} // namespace Yacht

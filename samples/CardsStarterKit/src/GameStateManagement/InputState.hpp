#pragma once

// InputState.hpp -- C++ port of ScreenManager/InputState.cs (XNA 4.0
// CardsStarterKit sample). Helper for reading keyboard/gamepad input.
// Touch/gestures from the XNA original are compiled out on Windows Phone
// only (`#if WINDOWS_PHONE`) even in the original -- the desktop path this
// port follows never used them, so nothing is omitted here.

#include <array>
#include <optional>

#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"

namespace GameStateManagement {

using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Buttons;

class InputState {
public:
    static const int MaxInputs = 4;

    std::array<KeyboardState, MaxInputs> CurrentKeyboardStates;
    std::array<GamePadState,  MaxInputs> CurrentGamePadStates;

    std::array<KeyboardState, MaxInputs> LastKeyboardStates;
    std::array<GamePadState,  MaxInputs> LastGamePadStates;

    std::array<bool, MaxInputs> GamePadWasConnected{};

    InputState() = default;

    void Update() {
        for (int i = 0; i < MaxInputs; i++) {
            LastKeyboardStates[i] = CurrentKeyboardStates[i];
            LastGamePadStates[i]  = CurrentGamePadStates[i];

            CurrentKeyboardStates[i] = Keyboard::GetState(static_cast<PlayerIndex>(i));
            CurrentGamePadStates[i]  = GamePad::GetState(static_cast<PlayerIndex>(i));

            if (CurrentGamePadStates[i].getIsConnectedProperty())
                GamePadWasConnected[i] = true;
        }
    }

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
};

} // namespace GameStateManagement

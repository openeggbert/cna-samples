#pragma once

// InputManager.hpp -- C++ port of InputManager.cs. This class handles all
// keyboard and gamepad actions in the game.
//
// CNA unifies GamePad face/shoulder buttons and D-pad directions into a
// single Buttons flags enum (checked via GamePadState::IsButtonDown(Buttons)),
// unlike the original's separate GamePadState.Buttons/.DPad structs -- this
// port's GamePadButtons enum below maps onto that single CNA enum instead of
// mirroring the original's two-struct split (see missing.md).

#include <array>
#include <cmath>
#include <string>

#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"

namespace RolePlaying {

using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::PlayerIndex;

class InputManager {
public:
    enum class Action {
        MainMenu, Ok, Back, CharacterManagement, ExitGame, TakeView, DropUnEquip,
        MoveCharacterUp, MoveCharacterDown, MoveCharacterLeft, MoveCharacterRight,
        CursorUp, CursorDown, DecreaseAmount, IncreaseAmount, PageLeft, PageRight,
        TargetUp, TargetDown, ActiveCharacterLeft, ActiveCharacterRight,
        TotalActionCount,
    };

    static std::string GetActionName(Action action) {
        static const char* names[] = {
            "Main Menu", "Ok", "Back", "Character Management", "Exit Game", "Take / View",
            "Drop / Unequip", "Move Character - Up", "Move Character - Down",
            "Move Character - Left", "Move Character - Right", "Move Cursor - Up",
            "Move Cursor - Down", "Decrease Amount", "Increase Amount", "Page Screen Left",
            "Page Screen Right", "Select Target -Up", "Select Target - Down",
            "Select Active Character - Left", "Select Active Character - Right",
        };
        return names[(int)action];
    }

    enum class GamePadButtons { Start, Back, A, B, X, Y, Up, Down, Left, Right, LeftShoulder, RightShoulder, LeftTrigger, RightTrigger };

    static bool IsKeyPressed(Keys key) { return currentKeyboardState_.IsKeyDown(key); }
    static bool IsKeyTriggered(Keys key) {
        return currentKeyboardState_.IsKeyDown(key) && !previousKeyboardState_.IsKeyDown(key);
    }

    static bool IsActionPressed(Action action) { return IsActionMapPressed(action); }
    static bool IsActionTriggered(Action action) { return IsActionMapTriggered(action); }

    static void Initialize() {}

    static void Update() {
        previousKeyboardState_ = currentKeyboardState_;
        currentKeyboardState_ = Keyboard::GetState();
        previousGamePadState_ = currentGamePadState_;
        currentGamePadState_ = GamePad::GetState(PlayerIndex::One);
    }

private:
    static constexpr float AnalogLimit = 0.5f;

    static bool IsGamePadButtonDown(GamePadButtons key, const GamePadState& state) {
        switch (key) {
            case GamePadButtons::Start: return state.IsButtonDown(Buttons::Start);
            case GamePadButtons::Back: return state.IsButtonDown(Buttons::Back);
            case GamePadButtons::A: return state.IsButtonDown(Buttons::A);
            case GamePadButtons::B: return state.IsButtonDown(Buttons::B);
            case GamePadButtons::X: return state.IsButtonDown(Buttons::X);
            case GamePadButtons::Y: return state.IsButtonDown(Buttons::Y);
            case GamePadButtons::LeftShoulder: return state.IsButtonDown(Buttons::LeftShoulder);
            case GamePadButtons::RightShoulder: return state.IsButtonDown(Buttons::RightShoulder);
            case GamePadButtons::LeftTrigger: return state.getTriggersProperty().getLeftProperty() > AnalogLimit;
            case GamePadButtons::RightTrigger: return state.getTriggersProperty().getRightProperty() > AnalogLimit;
            case GamePadButtons::Up:
                return state.IsButtonDown(Buttons::DPadUp) || state.getThumbSticksProperty().getLeftProperty().Y > AnalogLimit;
            case GamePadButtons::Down:
                return state.IsButtonDown(Buttons::DPadDown) || -state.getThumbSticksProperty().getLeftProperty().Y > AnalogLimit;
            case GamePadButtons::Left:
                return state.IsButtonDown(Buttons::DPadLeft) || -state.getThumbSticksProperty().getLeftProperty().X > AnalogLimit;
            case GamePadButtons::Right:
                return state.IsButtonDown(Buttons::DPadRight) || state.getThumbSticksProperty().getLeftProperty().X > AnalogLimit;
        }
        return false;
    }

    static bool IsGamePadButtonTriggered(GamePadButtons key) {
        return IsGamePadButtonDown(key, currentGamePadState_) && !IsGamePadButtonDown(key, previousGamePadState_);
    }

    struct ActionMapEntry {
        Keys key;
        GamePadButtons button;
    };

    static ActionMapEntry MapFor(Action action) {
        switch (action) {
            case Action::MainMenu: return {Keys::Tab, GamePadButtons::Start};
            case Action::Ok: return {Keys::Enter, GamePadButtons::A};
            case Action::Back: return {Keys::Escape, GamePadButtons::B};
            case Action::CharacterManagement: return {Keys::Space, GamePadButtons::Y};
            case Action::ExitGame: return {Keys::Escape, GamePadButtons::Back};
            case Action::TakeView: return {Keys::LeftControl, GamePadButtons::Y};
            case Action::DropUnEquip: return {Keys::D, GamePadButtons::X};
            case Action::MoveCharacterUp: return {Keys::Up, GamePadButtons::Up};
            case Action::MoveCharacterDown: return {Keys::Down, GamePadButtons::Down};
            case Action::MoveCharacterLeft: return {Keys::Left, GamePadButtons::Left};
            case Action::MoveCharacterRight: return {Keys::Right, GamePadButtons::Right};
            case Action::CursorUp: return {Keys::Up, GamePadButtons::Up};
            case Action::CursorDown: return {Keys::Down, GamePadButtons::Down};
            case Action::DecreaseAmount: return {Keys::Left, GamePadButtons::Left};
            case Action::IncreaseAmount: return {Keys::Right, GamePadButtons::Right};
            case Action::PageLeft: return {Keys::LeftShift, GamePadButtons::LeftTrigger};
            case Action::PageRight: return {Keys::RightShift, GamePadButtons::RightTrigger};
            case Action::TargetUp: return {Keys::Up, GamePadButtons::Up};
            case Action::TargetDown: return {Keys::Down, GamePadButtons::Down};
            case Action::ActiveCharacterLeft: return {Keys::Left, GamePadButtons::Left};
            case Action::ActiveCharacterRight: return {Keys::Right, GamePadButtons::Right};
            default: return {Keys::None, GamePadButtons::Start};
        }
    }

    static bool IsActionMapPressed(Action action) {
        auto map = MapFor(action);
        if (IsKeyPressed(map.key)) return true;
        if (currentGamePadState_.getIsConnectedProperty()) return IsGamePadButtonDown(map.button, currentGamePadState_);
        return false;
    }
    static bool IsActionMapTriggered(Action action) {
        auto map = MapFor(action);
        if (IsKeyTriggered(map.key)) return true;
        if (currentGamePadState_.getIsConnectedProperty()) return IsGamePadButtonTriggered(map.button);
        return false;
    }

    static inline KeyboardState currentKeyboardState_{};
    static inline KeyboardState previousKeyboardState_{};
    static inline GamePadState currentGamePadState_{};
    static inline GamePadState previousGamePadState_{};
};

} // namespace RolePlaying

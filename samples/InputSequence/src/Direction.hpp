#pragma once
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"

namespace InputSequenceSample::Direction {

using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePadState;
using Microsoft::Xna::Framework::Input::KeyboardState;
using Microsoft::Xna::Framework::Input::Keys;

constexpr Buttons None      = static_cast<Buttons>(0);
constexpr Buttons Up        = Buttons::DPadUp   | Buttons::LeftThumbstickUp;
constexpr Buttons Down      = Buttons::DPadDown | Buttons::LeftThumbstickDown;
constexpr Buttons Left      = Buttons::DPadLeft | Buttons::LeftThumbstickLeft;
constexpr Buttons Right     = Buttons::DPadRight | Buttons::LeftThumbstickRight;
constexpr Buttons UpLeft    = Up | Left;
constexpr Buttons UpRight   = Up | Right;
constexpr Buttons DownLeft  = Down | Left;
constexpr Buttons DownRight = Down | Right;
constexpr Buttons Any       = Up | Down | Left | Right;

inline Buttons FromInput(const GamePadState& gamePad, const KeyboardState& keyboard) {
    Buttons direction = None;

    if (gamePad.IsButtonDown(Buttons::DPadUp) ||
        gamePad.IsButtonDown(Buttons::LeftThumbstickUp) ||
        keyboard.IsKeyDown(Keys::Up))
    {
        direction |= Up;
    }
    else if (gamePad.IsButtonDown(Buttons::DPadDown) ||
             gamePad.IsButtonDown(Buttons::LeftThumbstickDown) ||
             keyboard.IsKeyDown(Keys::Down))
    {
        direction |= Down;
    }

    if (gamePad.IsButtonDown(Buttons::DPadLeft) ||
        gamePad.IsButtonDown(Buttons::LeftThumbstickLeft) ||
        keyboard.IsKeyDown(Keys::Left))
    {
        direction |= Left;
    }
    else if (gamePad.IsButtonDown(Buttons::DPadRight) ||
             gamePad.IsButtonDown(Buttons::LeftThumbstickRight) ||
             keyboard.IsKeyDown(Keys::Right))
    {
        direction |= Right;
    }

    return direction;
}

inline Buttons FromButtons(Buttons buttons) {
    return buttons & Any;
}

} // namespace InputSequenceSample::Direction

#pragma once
#include <array>
#include <utility>
#include <vector>
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/GamePadState.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/KeyboardState.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "System/TimeSpan.hpp"
#include "Direction.hpp"
#include "Move.hpp"

namespace InputSequenceSample {

class InputManager {
public:
    Microsoft::Xna::Framework::PlayerIndex PlayerIndex;
    Microsoft::Xna::Framework::Input::GamePadState  GamePadState;
    Microsoft::Xna::Framework::Input::KeyboardState KeyboardState;

    System::TimeSpan LastInputTime;
    std::vector<Microsoft::Xna::Framework::Input::Buttons> Buffer;

    const System::TimeSpan BufferTimeOut  = System::TimeSpan::FromMilliseconds(500);
    const System::TimeSpan MergeInputTime = System::TimeSpan::FromMilliseconds(100);

    static constexpr std::array<
        std::pair<Microsoft::Xna::Framework::Input::Buttons,
                  Microsoft::Xna::Framework::Input::Keys>, 4>
    NonDirectionButtons = {{
        { Microsoft::Xna::Framework::Input::Buttons::A, Microsoft::Xna::Framework::Input::Keys::A },
        { Microsoft::Xna::Framework::Input::Buttons::B, Microsoft::Xna::Framework::Input::Keys::B },
        { Microsoft::Xna::Framework::Input::Buttons::X, Microsoft::Xna::Framework::Input::Keys::X },
        { Microsoft::Xna::Framework::Input::Buttons::Y, Microsoft::Xna::Framework::Input::Keys::Y },
    }};

    InputManager(Microsoft::Xna::Framework::PlayerIndex playerIndex, int bufferCapacity)
        : PlayerIndex(playerIndex)
    {
        Buffer.reserve(bufferCapacity);
    }

    void Update(const Microsoft::Xna::Framework::GameTime& gameTime) {
        using namespace Microsoft::Xna::Framework::Input;
        using Microsoft::Xna::Framework::PlayerIndex;

        auto lastGamePad   = GamePadState;
        auto lastKeyboard  = KeyboardState;
        GamePadState  = GamePad::GetState(this->PlayerIndex);
        if (this->PlayerIndex == PlayerIndex::One)
            KeyboardState = Keyboard::GetState(this->PlayerIndex);

        // Expire old input.
        System::TimeSpan time          = gameTime.getTotalGameTimeProperty();
        System::TimeSpan timeSinceLast = time - LastInputTime;
        if (timeSinceLast > BufferTimeOut)
            Buffer.clear();

        // Accumulate non-direction button presses.
        Buttons buttons = Direction::None;
        for (auto& [btn, key] : NonDirectionButtons) {
            if ((lastGamePad.IsButtonUp(btn) && GamePadState.IsButtonDown(btn)) ||
                (lastKeyboard.IsKeyUp(key)   && KeyboardState.IsKeyDown(key)))
            {
                buttons |= btn;
            }
        }

        bool mergeInput = (!Buffer.empty() && timeSinceLast < MergeInputTime);

        // Detect direction change.
        auto direction     = Direction::FromInput(GamePadState, KeyboardState);
        auto lastDirection = Direction::FromInput(lastGamePad, lastKeyboard);
        if (lastDirection != direction) {
            buttons |= direction;
            mergeInput = false;
        }

        if (buttons != Direction::None) {
            if (mergeInput) {
                Buffer.back() = Buffer.back() | buttons;
            } else {
                if ((int)Buffer.size() == (int)Buffer.capacity())
                    Buffer.erase(Buffer.begin());
                Buffer.push_back(buttons);
                LastInputTime = time;
            }
        }
    }

    bool Matches(Move& move) {
        if ((int)Buffer.size() < (int)move.Sequence.size())
            return false;

        int bufSize = (int)Buffer.size();
        int seqSize = (int)move.Sequence.size();
        for (int i = 1; i <= seqSize; ++i) {
            if (Buffer[bufSize - i] != move.Sequence[seqSize - i])
                return false;
        }

        if (!move.IsSubMove)
            Buffer.clear();

        return true;
    }
};

} // namespace InputSequenceSample

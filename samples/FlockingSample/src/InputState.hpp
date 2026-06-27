#pragma once
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"

namespace Flocking {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Input;

class InputState {
public:
    KeyboardState currentKey;
    KeyboardState lastKey;
    GamePadState  currentPad;
    GamePadState  lastPad;

    void Update() {
        lastKey   = currentKey;
        lastPad   = currentPad;
        currentKey = Keyboard::GetState();
        currentPad = GamePad::GetState(PlayerIndex::One);
    }

    float MoveCatX() const {
        if (currentKey.IsKeyDown(Keys::A)) return -1.0f;
        if (currentKey.IsKeyDown(Keys::D)) return  1.0f;
        return currentPad.getThumbSticksProperty().getLeftProperty().X;
    }

    float MoveCatY() const {
        if (currentKey.IsKeyDown(Keys::W)) return -1.0f;
        if (currentKey.IsKeyDown(Keys::S)) return  1.0f;
        return -currentPad.getThumbSticksProperty().getLeftProperty().Y;
    }

    float SliderMove() const {
        if (currentKey.IsKeyDown(Keys::Left)  || currentPad.IsButtonDown(Buttons::DPadLeft))  return -1.0f;
        if (currentKey.IsKeyDown(Keys::Right) || currentPad.IsButtonDown(Buttons::DPadRight)) return  1.0f;
        return 0.0f;
    }

    bool Exit()           const { return IsNew(Keys::Escape) || IsNewPad(Buttons::Back); }
    bool ResetDistances() const { return IsNew(Keys::B) || IsNewPad(Buttons::B); }
    bool ResetFlock()     const { return IsNew(Keys::X) || IsNewPad(Buttons::X); }
    bool ToggleCat()      const { return IsNew(Keys::Y) || IsNewPad(Buttons::Y); }
    bool SelectUp()       const { return IsNew(Keys::Up)   || IsNewDPad(Buttons::DPadUp); }
    bool SelectDown()     const { return IsNew(Keys::Down) || IsNewDPad(Buttons::DPadDown); }

private:
    bool IsNew(Keys k)     const { return currentKey.IsKeyDown(k) && lastKey.IsKeyUp(k); }
    bool IsNewPad(Buttons b) const {
        return currentPad.IsButtonDown(b) && !lastPad.IsButtonDown(b);
    }
    bool IsNewDPad(Buttons b) const { return IsNewPad(b); }
};

} // namespace Flocking

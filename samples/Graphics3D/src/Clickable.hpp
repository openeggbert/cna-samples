#pragma once

#include <Microsoft/Xna/Framework/DrawableGameComponent.hpp>
#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>
#include <Microsoft/Xna/Framework/Input/MouseState.hpp>
#include <Microsoft/Xna/Framework/Input/ButtonState.hpp>

#include <string>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Input;

namespace Graphics3DSample {

// A game component with an associated rectangle that accepts mouse hover/click
// inside it. Has a state of IsTouching and IsClicked.
//
// Ported from the original's touch-based Clickable (TouchPanel.GetState(),
// checking the first active touch point against the rectangle) — this desktop
// port has no touchscreen, so mouse position + left button substitutes for
// finger contact (see missing.md).
class Clickable : public DrawableGameComponent {
public:
    Clickable(Game& game, const Rectangle& targetRectangle)
        : DrawableGameComponent(game), rectangle_(targetRectangle) {}

    const std::string& GetTypeName() const override {
        static const std::string name = "Clickable";
        return name;
    }

    bool IsTouching() const { return isTouching_; }
    bool IsClicked() const { return wasTouching_ && !isTouching_; }

protected:
    const Rectangle& GetRectangle() const { return rectangle_; }

    void HandleInput() {
        wasTouching_ = isTouching_;
        isTouching_ = false;

        MouseState mouse = Mouse::GetState();
        if (mouse.getLeftButtonProperty() == ButtonState::Pressed &&
            rectangle_.Contains(mouse.getXProperty(), mouse.getYProperty())) {
            isTouching_ = true;
        }
    }

private:
    Rectangle rectangle_;
    bool wasTouching_ = false;
    bool isTouching_ = false;
};

} // namespace Graphics3DSample

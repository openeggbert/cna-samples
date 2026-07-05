#pragma once

// MenuEntry.hpp — C++ port of ScreenManager/MenuEntry.cs (XNA 4.0
// NinjAcademy sample). Represents a single entry in a MenuScreen.

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

#include "GameScreen.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;

class MenuScreen; // forward declaration

// Represents a single entry in a MenuScreen. Draws the entry text string and
// provides an event raised when the entry is selected. Port of
// ScreenManager/MenuEntry.cs.
class MenuEntry {
public:
    explicit MenuEntry(const std::string& text) : text_(text) {}
    virtual ~MenuEntry() = default;

    const std::string& Text() const { return text_; }
    void setText(const std::string& value) { text_ = value; }

    Vector2 Position() const { return position_; }
    void setPosition(Vector2 value) { position_ = value; }

    // Event raised when the menu entry is selected.
    std::function<void(PlayerIndex)> Selected;

    void OnSelectEntry(PlayerIndex playerIndex) {
        if (Selected)
            Selected(playerIndex);
    }

    // Updates the menu entry's fading selection effect.
    virtual void Update(MenuScreen& screen, bool isSelected, GameTime& gameTime) {
        (void)screen;
        float fadeSpeed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty() * 4;
        if (isSelected)
            selectionFade_ = std::min(selectionFade_ + fadeSpeed, 1.0f);
        else
            selectionFade_ = std::max(selectionFade_ - fadeSpeed, 0.0f);
    }

    // The following need ScreenManager/MenuScreen internals; defined in MenuScreen.hpp.
    virtual void Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime);
    virtual int GetHeight(MenuScreen& screen);
    virtual int GetWidth(MenuScreen& screen);

protected:
    float selectionFade_ = 0.0f;

private:
    std::string text_;
    Vector2 position_;
};

} // namespace NinjAcademy

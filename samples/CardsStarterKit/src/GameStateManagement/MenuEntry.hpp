#pragma once

// MenuEntry.hpp -- C++ port of ScreenManager/MenuEntry.cs (XNA 4.0
// CardsStarterKit sample). Unlike the stock "Game State Management" template
// (see GameStateManagement/NinjAcademy's MenuEntry), this sample's entries
// are laid out in a horizontal row across the bottom of the screen and drawn
// over a button-background texture, with a `Destination` rectangle instead
// of a `Position` point -- ported to match, not the stock template.

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

#include "GameScreen.hpp"

namespace GameStateManagement {

using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;

class MenuScreen;  // forward declaration

// Helper class representing a single entry in a MenuScreen.
class MenuEntry {
public:
    // Event raised when the menu entry is selected. Simplified from the C#
    // original's `event EventHandler<PlayerIndexEventArgs>` to a plain
    // std::function, matching this project's established GameStateManagement/
    // NinjAcademy precedent.
    std::function<void(PlayerIndex)> Selected;

    Rectangle Destination;
    float Scale = 1.0f;
    float Rotation = 0.0f;

    explicit MenuEntry(const std::string& text) : text_(text) {}
    virtual ~MenuEntry() = default;

    const std::string& Text() const { return text_; }
    void setText(const std::string& value) { text_ = value; }

    void OnSelectEntry(PlayerIndex playerIndex) {
        if (Selected)
            Selected(playerIndex);
    }

    // Updates the fading selection effect.
    virtual void Update(MenuScreen& screen, bool isSelected, GameTime& gameTime) {
        (void)screen;
        float fadeSpeed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty() * 4;
        if (isSelected)
            selectionFade_ = std::min(selectionFade_ + fadeSpeed, 1.0f);
        else
            selectionFade_ = std::max(selectionFade_ - fadeSpeed, 0.0f);
    }

    // The following need ScreenManager internals; defined in MenuScreen.hpp.
    virtual void Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime);
    virtual int GetHeight(MenuScreen& screen) const;
    virtual int GetWidth(MenuScreen& screen) const;

private:
    Vector2 getTextPosition(MenuScreen& screen) const;

    std::string text_;
    float selectionFade_ = 0.0f;
};

} // namespace GameStateManagement

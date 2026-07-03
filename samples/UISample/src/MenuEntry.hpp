#pragma once

#include <cmath>
#include <functional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"

#include "GameScreen.hpp"
#include "ScreenManager.hpp"

namespace UISample {

using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;

class MenuScreen; // forward declaration (defined in MenuScreen.hpp)

// Represents a single entry in a MenuScreen. By default this just draws the
// entry text string, and provides an event raised when selected. Port of
// Screens/MenuEntry.cs.
//
// The original forces `isSelected` to always false on Windows Phone (there's
// no such thing as a keyboard-highlighted item on a tap-only device), and
// `selectedEntry` is never modified anywhere in the original codebase — this
// sample is tap-only, with no up/down keyboard navigation. The port keeps
// that same behavior: isSelected is always drawn as false. See missing.md.
class MenuEntry {
public:
    std::function<void(PlayerIndex)> Selected;

    explicit MenuEntry(std::string text) : text_(std::move(text)) {}

    const std::string& Text() const { return text_; }
    void setText(std::string value) { text_ = std::move(value); }

    Vector2 Position() const { return position_; }
    void setPosition(Vector2 value) { position_ = value; }

    // Raises the Selected event.
    void OnSelectEntry(PlayerIndex playerIndex) {
        if (Selected) Selected(playerIndex);
    }

    // Updates the menu entry's fading selection effect.
    virtual void Update(MenuScreen& screen, bool isSelected, const GameTime& gameTime) {
        (void)screen;
        // there is no such thing as a selected item on this touch-only sample
        isSelected = false;

        float fadeSpeed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty() * 4.0f;
        if (isSelected)
            selectionFade_ = std::min(selectionFade_ + fadeSpeed, 1.0f);
        else
            selectionFade_ = std::max(selectionFade_ - fadeSpeed, 0.0f);
    }

    // Draws the menu entry; can be overridden to customize the appearance.
    virtual void Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime);

    // Queries how much space this menu entry requires.
    virtual int GetHeight(MenuScreen& screen);

    // Queries how wide the entry is, used for centering on the screen.
    virtual int GetWidth(MenuScreen& screen);

private:
    std::string text_;
    float selectionFade_ = 0.0f;
    Vector2 position_;
};

} // namespace UISample

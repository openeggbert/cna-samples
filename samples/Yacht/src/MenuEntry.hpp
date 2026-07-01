#pragma once

#include <algorithm>
#include <functional>
#include <string>

#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"

#include "GameScreen.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;

class MenuScreen;  // forward declaration

// A single entry in a MenuScreen. Unlike the generic GameStateManagement
// MenuEntry (a centered vertical Position-based list), Yacht ships its own
// customized copy of the ScreenManager framework inside Yacht/ScreenManager/
// whose MenuEntry.cs draws each entry as a button over an explicit
// Destination rectangle that the owning screen sets itself -- that is what
// is ported here.
class MenuEntry {
public:
    explicit MenuEntry(const std::string& text) : text_(text) {}
    virtual ~MenuEntry() = default;

    const std::string& Text() const { return text_; }
    void setText(const std::string& value) { text_ = value; }

    Rectangle getDestination() const { return destination_; }
    void setDestination(Rectangle value) { destination_ = value; }

    float Scale = 1.0f;
    float Rotation = 0.0f;

    // Event raised when the menu entry is selected.
    std::function<void(PlayerIndex)> Selected;

    void OnSelectEntry(PlayerIndex playerIndex) {
        if (Selected)
            Selected(playerIndex);
    }

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

private:
    Vector2 GetTextPosition(MenuScreen& screen);

    std::string text_;
    float selectionFade_ = 0.0f;
    Rectangle destination_;
};

} // namespace Yacht

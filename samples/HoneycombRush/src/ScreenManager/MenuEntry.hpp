#pragma once

// MenuEntry.hpp — C++ port of ScreenManager/MenuEntry.cs (XNA 4.0
// HoneycombRush sample). Unlike the plain-text MenuEntry in this project's
// other ScreenManager-derived ports, this one draws a button-background
// texture behind the text (from ScreenManager::getButtonBackground()).

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "GameScreen.hpp"

namespace HoneycombRush {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class MenuScreen; // forward declaration

// Represents a single entry in a MenuScreen. Draws a button background plus
// the entry's text, and provides an event raised when it is selected. Port of
// ScreenManager/MenuEntry.cs.
class MenuEntry {
public:
    explicit MenuEntry(const std::string& text) : text_(text) {}
    virtual ~MenuEntry() = default;

    const std::string& Text() const { return text_; }
    void setText(const std::string& value) { text_ = value; }

    Vector2 Position() const { return position_; }
    void setPosition(Vector2 value) { position_ = value; }

    Rectangle Bounds() const {
        return Rectangle((int)position_.X, (int)position_.Y, buttonTexture_ ? buttonTexture_->getWidthProperty() : 0,
                          buttonTexture_ ? buttonTexture_->getHeightProperty() : 0);
    }

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
    Vector2 getTextPosition(MenuScreen& screen);

    std::string text_;
    float selectionFade_ = 0.0f;
    Vector2 position_;
    Texture2D* buttonTexture_ = nullptr;
};

} // namespace HoneycombRush

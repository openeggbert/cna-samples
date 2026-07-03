#pragma once

#include "TextControl.hpp"

namespace DynamicMenu::Controls {

// A simple control that shows text centered in its bounds. Port of
// Controls/Label.cs.
class Label : public TextControl {
public:
    void Draw(const GameTime& gameTime, SpriteBatch& spriteBatch) override {
        Control::Draw(gameTime, spriteBatch);
        DrawCenteredText(spriteBatch, Font.has_value() ? &*Font : nullptr, GetAbsoluteRect(), Text, TextColor);
    }
};

} // namespace DynamicMenu::Controls

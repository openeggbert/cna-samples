#pragma once

#include <optional>
#include <string>

#include "Control.hpp"

namespace DynamicMenu::Controls {

// A control that owns a piece of text and the font to draw it with. Port of
// Controls/TextControl.cs, merged with its ITextControl.cs interface (nothing
// in this sample implements ITextControl independently of TextControl).
class TextControl : public Control {
public:
    std::string Text;
    std::string FontName;
    std::optional<SpriteFont> Font;
    Color TextColor = Color::Black;

    // Automatically sizes the control's width to fit Text, plus SPACE padding
    // on each side.
    void AutoPickWidth() {
        Vector2 dim = Font->MeasureString(Text);
        Width = Space * 2 + (int)dim.X;
    }

    void LoadContent(GraphicsDevice& graphics, ContentManager& content) override {
        Control::LoadContent(graphics, content);
        if (!FontName.empty()) {
            Font.emplace(content.Load<SpriteFont>(FontName));
        }
    }

private:
    // The amount of space to the left and right of the text when auto-sized
    // using AutoPickWidth.
    static constexpr int Space = 10;
};

} // namespace DynamicMenu::Controls

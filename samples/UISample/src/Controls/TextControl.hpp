#pragma once

#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"

#include "Control.hpp"

namespace UISample::Controls {

using Microsoft::Xna::Framework::Color;

// Displays a single string of text. By default, the size is computed from
// the given text and font. Port of Controls/TextControl.cs.
//
// The original's `Font` getter has a copy-paste bug (`get { return Font; }`,
// infinite recursion) -- unreachable in practice since nothing in this sample
// ever reads it, only writes it, but fixed here anyway.
class TextControl : public Control {
public:
    Color TextColor = Color::White;

    TextControl() = default;
    TextControl(std::string text, const SpriteFont* font)
        : text_(std::move(text)), font_(font) {}
    TextControl(std::string text, const SpriteFont* font, Color color)
        : text_(std::move(text)), font_(font), TextColor(color) {}
    TextControl(std::string text, const SpriteFont* font, Color color, Vector2 position)
        : text_(std::move(text)), font_(font), TextColor(color) {
        setPosition(position);
    }

    const std::string& Text() const { return text_; }
    void setText(std::string value) {
        if (text_ != value) {
            text_ = std::move(value);
            InvalidateAutoSize();
        }
    }

    const SpriteFont* Font() const { return font_; }
    void setFont(const SpriteFont* value) {
        if (font_ != value) {
            font_ = value;
            InvalidateAutoSize();
        }
    }

    void Draw(DrawContext context) override {
        Control::Draw(context);
        context.SpriteBatchValue->DrawString(*font_, text_, context.DrawOffset, TextColor);
    }

    Vector2 ComputeSize() override {
        return font_->MeasureString(text_);
    }

private:
    std::string text_;
    const SpriteFont* font_ = nullptr;
};

} // namespace UISample::Controls

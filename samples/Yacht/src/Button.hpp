#pragma once

#include <functional>
#include <string>

#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"

#include "InputState.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

// An on-screen button that the user can tap or click. Ported from
// Misc/Button.cs. Per the approved plan's input design, touch
// (GestureSample) and mouse each get their own HandleInput overload; both
// funnel into the same private hit-test helper -- only "where did this
// position come from" duplicates, never the hit-test geometry itself.
class Button {
public:
    Rectangle Position;
    bool Enabled = true;
    std::function<void()> Click;

    Button(const Texture2D& texture, Vector2 position, const SpriteFont* font, const std::string& text)
        : Button(texture,
                 Rectangle((int)position.X, (int)position.Y,
                          texture.getWidthProperty(), texture.getHeightProperty()),
                 font, text) {}

    Button(const Texture2D& texture, Rectangle position, const SpriteFont* font, const std::string& text)
        : Position(position), texture_(&texture), font_(font), text_(text) {}

    const Texture2D& getTexture() const { return *texture_; }

    // Touch: handle a single gesture sample.
    bool HandleInput(const GestureSample& sample) {
        if (Enabled && sample.getGestureTypeProperty() == GestureType::Tap)
            return HitTestAndClick(sample.getPositionProperty());
        return false;
    }

    // Mouse: handle the current frame's input state.
    bool HandleInput(InputState& input) {
        if (Enabled && input.IsNewLeftMousePress())
            return HitTestAndClick(input.MousePosition());
        return false;
    }

    void Draw(SpriteBatch& spriteBatch) {
        spriteBatch.Draw(*texture_, Position, Enabled ? Color::White : Color::Gray);
        if (!text_.empty() && font_ != nullptr)
            DrawString(spriteBatch);
    }

private:
    bool HitTestAndClick(Vector2 point) {
        Rectangle touchRect((int)point.X - 1, (int)point.Y - 1, 2, 2);

        Rectangle bounds = texture_->getBoundsProperty();
        bounds.X += Position.X;
        bounds.Y += Position.Y;

        if (bounds.Intersects(touchRect)) {
            if (Click)
                Click();
            return true;
        }
        return false;
    }

    void DrawString(SpriteBatch& spriteBatch) {
        Vector2 textSize = font_->MeasureString(text_);
        spriteBatch.DrawString(*font_, text_,
                               Vector2((float)Position.X + Position.Width / 2.0f - textSize.X / 2.0f,
                                       (float)Position.Y + Position.Height / 2.0f - textSize.Y / 2.0f - 5.0f),
                               Color::White);
    }

    const Texture2D* texture_;
    const SpriteFont* font_;
    std::string text_;
};

} // namespace Yacht

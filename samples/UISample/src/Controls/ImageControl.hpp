#pragma once

#include <optional>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"

#include "Control.hpp"

namespace UISample::Controls {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Rectangle;

// Displays a single sprite. By default it displays an entire texture. If no
// texture is given, this control uses DrawContext::BlankTexture, allowing it
// to draw solid-colored rectangles. Port of Controls/ImageControl.cs.
class ImageControl : public Control {
public:
    // Position within the source texture, in texels. Default (0,0), the
    // upper-left corner.
    Vector2 Origin;

    // Size in texels of the source rectangle. If not set (the default), the
    // size is the same as the control's Size. Only needed if you want texels
    // scaled to something other than 1-to-1; normally just set the size of
    // both the source and destination via Size.
    std::optional<Vector2> SourceSize;

    // Color to modulate the texture with. Default white (unmodified texture).
    Color TintColor = Color::White;

    ImageControl() = default;
    ImageControl(Texture2D* texture, Vector2 position) : texture_(texture) {
        setPosition(position);
    }

    Texture2D* Texture() const { return texture_; }
    void setTexture(Texture2D* value) {
        if (texture_ != value) {
            texture_ = value;
            InvalidateAutoSize();
        }
    }

    void Draw(DrawContext context) override {
        Control::Draw(context);
        Texture2D* drawTexture = texture_ != nullptr ? texture_ : context.BlankTexture;

        Vector2 actualSourceSize = SourceSize.value_or(Size());
        Rectangle sourceRectangle((int)Origin.X, (int)Origin.Y,
                                  (int)actualSourceSize.X, (int)actualSourceSize.Y);
        Rectangle destRectangle((int)context.DrawOffset.X, (int)context.DrawOffset.Y,
                                (int)Size().X, (int)Size().Y);

        context.SpriteBatchValue->Draw(*drawTexture, destRectangle, sourceRectangle, TintColor);
    }

    Vector2 ComputeSize() override {
        if (texture_ != nullptr) {
            return Vector2((float)texture_->getWidthProperty(), (float)texture_->getHeightProperty());
        }
        return Vector2::Zero;
    }

private:
    Texture2D* texture_ = nullptr;
};

} // namespace UISample::Controls

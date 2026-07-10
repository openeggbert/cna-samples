#pragma once

// Ported from RimLighting's Slidebar.cs (Microsoft Advanced Technology Group). A
// Slidebar is a UI Element (control) that has a string of text and a bar whose length
// can be changed by dragging on it.
//
// See UIElement.hpp's own header comment for why the C# original's Draw(SpriteBatch)
// (which opens/closes its own SpriteBatch.Begin()/End() block) is ported here as
// DrawText(SpriteBatch&), assuming an already-open Begin block owned by the game's own
// single per-frame SpriteBatch block, instead.
//
// NOXNA: the C# original lazily loads a *static*, class-shared "blankTex" texture the
// first time any Slidebar draws (`protected void LoadContent()`, called from Draw()).
// This port instead takes a reference to the already-loaded blank texture (the same
// Texture2D RimLightingGame's own LoadContent() loads for the model's default texture)
// via SetBlankTexture(), avoiding a second, redundant load of the same 4x4 asset and a
// static/shared-ownership pattern that doesn't fit this port's RAII-based asset
// ownership -- not a behavior change (real XNA's own ContentManager caches/dedupes
// repeated Content.Load<T> calls for the same asset name too).

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocation.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocationState.hpp"

#include "UIElement.hpp"

#include <functional>
#include <string>

namespace RimLightingSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input::Touch;

class Slidebar : public UIElement {
public:
    using ValueChangedHandler = std::function<void()>;

    Slidebar(SpriteFont& font, float min, float max) : spriteFont_(font), minValue_(min), maxValue_(max) {}

    // Color of the control.
    Color TintColor = Color::White;

    // The maximum and minimum possible values represented by this slidebar.
    float minValue_;
    float maxValue_;

    // Text of the control to show on screen.
    std::string GetText() const { return text_; }
    void SetText(const std::string& value) {
        text_ = value;
        textSize_ = spriteFont_.MeasureString(value);
        needsMeasure_ = true;
    }

    Vector2 GetTextSize() const { return textSize_; }

    // Current Value of the slidebar.
    float GetValue() const { return value_; }
    void SetValue(float value) {
        value_ = value;
        currentLength_ = (value_ - minValue_) / (maxValue_ - minValue_) * sizeBar_.X;

        if (OnValueChanged) {
            OnValueChanged();
        }
    }

    // Is the user currently dragging on this slidebar?
    bool IsDragging = false;

    // OnValueChanged event: triggered either by manually changing Value, or dragging on
    // the bar by the user.
    ValueChangedHandler OnValueChanged;

    // Sets position and size of the bar.
    //   offsetX/offsetY: the relative coordinate to the Text.
    //   maxWidth/height: the max width/height of the bar in pixels.
    void SetBarOffsetSize(float offsetX, float offsetY, float maxWidth, float height) {
        offsetBar_ = Vector2(offsetX, offsetY);
        sizeBar_ = Vector2(maxWidth, height);
        needsMeasure_ = true;
    }

    // NOXNA: see this file's own header comment for why this replaces the C#
    // original's own lazy static Content.Load<Texture2D>("blankTex").
    void SetBlankTexture(Texture2D& blank) { blankTexture_ = &blank; }

    // Handle the touch input and update the bar if necessary.
    void HandleTouch(const TouchLocation& loc) override {
        if (loc.getStateProperty() == TouchLocationState::Pressed && !IsDragging) {
            const Vector2& pos = loc.getPositionProperty();
            if (pos.Y >= position_.Y && pos.Y <= (position_.Y + offsetBar_.Y + sizeBar_.Y)) {
                IsDragging = true;
                lastPressPosition_ = pos;
            }
        } else {
            if (loc.getStateProperty() == TouchLocationState::Released) {
                IsDragging = false;
            }
        }

        if (IsDragging) {
            Vector2 delta = loc.getPositionProperty() - lastPressPosition_;
            lastPressPosition_ = loc.getPositionProperty();

            currentLength_ += delta.X;
            if (currentLength_ < 0) currentLength_ = 0;
            if (currentLength_ > sizeBar_.X) currentLength_ = sizeBar_.X;
            value_ = currentLength_ / sizeBar_.X * (maxValue_ - minValue_) + minValue_;
            if (OnValueChanged) {
                OnValueChanged();
            }
        }
    }

    // Renders the control. Must be called inside an already-open SpriteBatch
    // Begin()/End() block -- see this file's own header comment.
    void DrawText(SpriteBatch& spriteBatch) {
        if (!IsVisible) {
            return;
        }

        if (needsMeasure_) {
            Measure();
        }

        Rectangle rect;
        rect.X = static_cast<int>(position_.X + offsetBar_.X);
        rect.Y = static_cast<int>(position_.Y + offsetBar_.Y);
        rect.Width = static_cast<int>(currentLength_);
        rect.Height = static_cast<int>(sizeBar_.Y);

        spriteBatch.Draw(*blankTexture_, rect, TintColor);
        spriteBatch.DrawString(spriteFont_, text_, position_, TintColor);
    }

protected:
    void Measure() override {
        size_ = textSize_ + offsetBar_ + sizeBar_;
        currentLength_ = (value_ - minValue_) / (maxValue_ - minValue_) * sizeBar_.X;
        needsMeasure_ = false;
    }

private:
    SpriteFont& spriteFont_;
    std::string text_;
    Vector2 textSize_;

    // The relative position of the bar to the text.
    Vector2 offsetBar_;
    // The maximum extent of the bar.
    Vector2 sizeBar_;
    // The current length of the bar, computed from Value/MinValue/MaxValue.
    float currentLength_ = 0.0f;

    float value_ = 0.0f;

    Vector2 lastPressPosition_;

    Texture2D* blankTexture_ = nullptr;
};

} // namespace RimLightingSample

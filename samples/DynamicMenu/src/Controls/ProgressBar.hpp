#pragma once

#include <cmath>
#include <optional>
#include <string>

#include "Control.hpp"

namespace DynamicMenu::Controls {

// A control which shows a progress indicator, split into a filled "left"
// texture and an unfilled "right" texture. Port of Controls/ProgressBar.cs.
class ProgressBar : public Control {
public:
    std::string LeftTextureName;
    std::optional<Texture2D> LeftTexture;
    std::string RightTextureName;
    std::optional<Texture2D> RightTexture;

    int Position = 0;
    int MaxValue = 100;
    Color LeftColor = Color::White;
    Color RightColor = Color::White;
    int BorderWidth = 0;

    void LoadContent(GraphicsDevice& graphics, ContentManager& content) override {
        Control::LoadContent(graphics, content);

        if (!LeftTextureName.empty()) {
            LeftTexture.emplace(content.Load<Texture2D>(LeftTextureName));
        }
        if (!RightTextureName.empty()) {
            RightTexture.emplace(content.Load<Texture2D>(RightTextureName));
        }
    }

    void Draw(const GameTime& gameTime, SpriteBatch& spriteBatch) override {
        Control::Draw(gameTime, spriteBatch);

        int leftSideWidth = GetLeftSideWidth();

        Rectangle rect = GetAbsoluteRect();
        rect.Width = leftSideWidth;
        rect.X += BorderWidth;
        rect.Y += BorderWidth;
        rect.Height -= BorderWidth * 2;

        Rectangle srcRect;
        srcRect.Width = rect.Width;
        srcRect.Height = rect.Height;

        if (LeftTexture.has_value()) {
            spriteBatch.Draw(*LeftTexture, rect, srcRect, LeftColor);
        }

        rect = GetAbsoluteRect();
        rect.X += leftSideWidth + BorderWidth;
        rect.Width -= BorderWidth * 2 + leftSideWidth;
        rect.Y += BorderWidth;
        rect.Height -= BorderWidth * 2;

        srcRect = Rectangle();
        srcRect.Width = rect.Width;
        srcRect.Height = rect.Height;
        srcRect.X = leftSideWidth;

        if (RightTexture.has_value()) {
            spriteBatch.Draw(*RightTexture, rect, srcRect, RightColor);
        }
    }

private:
    // Gets the current width of the left side based on Position and MaxValue.
    [[nodiscard]] int GetLeftSideWidth() const {
        float balance = (float)Position / (float)MaxValue;
        int progressBarWidth = Width - BorderWidth * 2;
        return (int)std::floor((float)progressBarWidth * balance);
    }
};

} // namespace DynamicMenu::Controls

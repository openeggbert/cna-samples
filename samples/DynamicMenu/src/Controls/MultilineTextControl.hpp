#pragma once

#include <string>
#include <vector>

#include "TextControl.hpp"

namespace DynamicMenu::Controls {

// A text control that wraps its text across multiple lines to fit its width.
// Port of Controls/MultilineTextControl.cs.
class MultilineTextControl : public TextControl {
public:
    int TopSpace = 0;
    int LeftSpace = 0;

    [[nodiscard]] const std::vector<std::string>& Lines() const { return lines_; }

    void LoadContent(GraphicsDevice& graphics, ContentManager& content) override {
        TextControl::LoadContent(graphics, content);
        CalculateLines();
    }

    void Draw(const GameTime& gameTime, SpriteBatch& spriteBatch) override {
        Control::Draw(gameTime, spriteBatch);

        if (!Font.has_value()) {
            // No font was loaded, so we can't show text
            return;
        }

        Vector2 extents = Font->MeasureString("A");
        Point topLeft = GetAbsoluteTopLeft();
        int currY = topLeft.Y + TopSpace + VertSpace;
        int left = topLeft.X + LeftSpace + HorzSpace;

        for (const std::string& line : lines_) {
            if (line.empty()) continue;

            spriteBatch.DrawString(*Font, line, Vector2((float)left, (float)currY), TextColor, 0.0f,
                                    Vector2::Zero, 1.0f, SpriteEffects::None, 1.0f);

            currY += (int)extents.Y + VertSpace;
        }
    }

    // Determines how to wrap Text into Lines based on the control's Width.
    virtual void CalculateLines() {
        lines_.clear();

        if (Text.empty()) {
            return;
        }
        if (!Font.has_value()) {
            // No font - can't calculate the lines
            return;
        }

        int lineWidth = Width - HorzSpace * 2 - LeftSpace;

        // Divide the text into words
        std::vector<std::string> words;
        std::string word;
        for (char c : Text) {
            if (c == ' ' || c == '\n') {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            } else {
                word += c;
            }
        }
        if (!word.empty()) words.push_back(word);

        std::string lineStr;
        for (const std::string& str : words) {
            std::string tempStr = lineStr;
            if (!tempStr.empty()) {
                tempStr += " ";
            }
            tempStr += str;

            Vector2 extents = Font->MeasureString(tempStr);
            if (extents.X > (float)lineWidth) {
                // Reached the end of the line. End the current line and start
                // the next with the current word.
                lines_.push_back(lineStr);
                lineStr = str;
            } else {
                lineStr = tempStr;
            }
        }

        if (!lineStr.empty()) {
            lines_.push_back(lineStr);
        }
    }

protected:
    static constexpr int HorzSpace = 10;
    static constexpr int VertSpace = 5;

private:
    std::vector<std::string> lines_;
};

} // namespace DynamicMenu::Controls

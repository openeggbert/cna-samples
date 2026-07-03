#pragma once

#include <cmath>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace UISample::CommonGraphics {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// Draws a string centered in the given rectangle. Port of CommonGraphics.cs.
inline void DrawCenteredText(SpriteBatch& batch, const SpriteFont& font, const Rectangle& rectangle,
                             const std::string& text, Color color) {
    if (!text.empty()) {
        Vector2 size = font.MeasureString(text);
        Vector2 topLeft(rectangle.getCenterProperty().X - size.X * 0.5f,
                        rectangle.getCenterProperty().Y - size.Y * 0.5f);
        batch.DrawString(font, text, topLeft, color);
    }
}

inline void DrawSpriteLine(SpriteBatch& batch, Texture2D& blankTexture, Vector2 vector1, Vector2 vector2,
                          Color color) {
    float distance = Vector2::Distance(vector1, vector2);
    float angle = std::atan2(vector2.Y - vector1.Y, vector2.X - vector1.X);

    // stretch the pixel between the two vectors
    batch.Draw(blankTexture, vector1, std::nullopt, color, angle, Vector2::Zero,
              Vector2(distance, 1.0f), SpriteEffects::None, 0.0f);
}

// Draws the outline of a rectangle. The supplied texture should be a
// single-pixel blank white texture such as ScreenManager::getBlankTexture().
// Does not call Begin/End on the batch; do that outside this call.
//
// Neither this function nor DrawCenteredText above are actually called
// anywhere in this sample -- they're plain utility functions ported for
// completeness. The original's DrawRectangle takes a `color` parameter but
// never passes it to DrawSpriteLine (hardcodes Color.White instead) -- an
// unreachable no-op bug in the original, fixed here since it's a one-line fix.
inline void DrawRectangle(SpriteBatch& batch, Texture2D& blankTexture, const Rectangle& rectangle, Color color) {
    DrawSpriteLine(batch, blankTexture, Vector2((float)rectangle.getLeftProperty(), (float)rectangle.getTopProperty()),
                  Vector2((float)rectangle.getRightProperty(), (float)rectangle.getTopProperty()), color);
    DrawSpriteLine(batch, blankTexture, Vector2((float)rectangle.getLeftProperty(), (float)rectangle.getBottomProperty()),
                  Vector2((float)rectangle.getRightProperty(), (float)rectangle.getBottomProperty()), color);
    DrawSpriteLine(batch, blankTexture, Vector2((float)rectangle.getLeftProperty(), (float)rectangle.getTopProperty()),
                  Vector2((float)rectangle.getLeftProperty(), (float)rectangle.getBottomProperty()), color);
    DrawSpriteLine(batch, blankTexture, Vector2((float)rectangle.getRightProperty(), (float)rectangle.getTopProperty()),
                  Vector2((float)rectangle.getRightProperty(), (float)rectangle.getBottomProperty()), color);
}

} // namespace UISample::CommonGraphics

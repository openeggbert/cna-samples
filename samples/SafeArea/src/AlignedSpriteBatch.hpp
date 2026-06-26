#pragma once
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

namespace SafeArea {

enum class Alignment : int {
    Left             = 0,
    Right            = 1,
    HorizontalCenter = 2,
    Top              = 0,
    Bottom           = 4,
    VerticalCenter   = 8,

    TopLeft      = 0,        // Top | Left
    TopRight     = 1,        // Top | Right
    TopCenter    = 2,        // Top | HorizontalCenter
    BottomLeft   = 4,        // Bottom | Left
    BottomRight  = 5,        // Bottom | Right
    BottomCenter = 6,        // Bottom | HorizontalCenter
    CenterLeft   = 8,        // VerticalCenter | Left
    CenterRight  = 9,        // VerticalCenter | Right
    Center       = 10,       // VerticalCenter | HorizontalCenter
};

inline Alignment operator|(Alignment a, Alignment b) {
    return static_cast<Alignment>(static_cast<int>(a) | static_cast<int>(b));
}
inline bool operator&(Alignment a, Alignment b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

class AlignedSpriteBatch : public Microsoft::Xna::Framework::Graphics::SpriteBatch {
public:
    explicit AlignedSpriteBatch(Microsoft::Xna::Framework::Graphics::GraphicsDevice& graphicsDevice)
        : SpriteBatch(graphicsDevice) {}

    void DrawString(const Microsoft::Xna::Framework::Graphics::SpriteFont& font,
                    const std::string& text,
                    Microsoft::Xna::Framework::Vector2 position,
                    Microsoft::Xna::Framework::Color color,
                    Alignment alignment) {
        using Microsoft::Xna::Framework::Vector2;

        if (alignment & Alignment::Right)
            position.X -= font.MeasureString(text).X;
        else if (alignment & Alignment::HorizontalCenter)
            position.X -= font.MeasureString(text).X / 2.0f;

        if (alignment & Alignment::Bottom)
            position.Y -= (float)font.getLineSpacingProperty();
        else if (alignment & Alignment::VerticalCenter)
            position.Y -= (float)font.getLineSpacingProperty() / 2.0f;

        SpriteBatch::DrawString(font, text, position, color);
    }
};

} // namespace SafeArea

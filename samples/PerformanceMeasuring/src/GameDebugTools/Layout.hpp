#pragma once

// Layout.hpp — C++ port of GameDebugTools/Layout.cs (XNA 4.0 PerformanceMeasuring sample).

#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/Viewport.hpp"

namespace PerformanceMeasuring::GameDebugTools {

using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::Viewport;

// Alignment for layout. A bit-flag enum, matching the original's [Flags] enum.
enum class Alignment {
    None = 0,

    Left = 1,
    Right = 2,
    HorizontalCenter = 4,

    Top = 8,
    Bottom = 16,
    VerticalCenter = 32,

    TopLeft = Top | Left,
    TopRight = Top | Right,
    TopCenter = Top | HorizontalCenter,

    BottomLeft = Bottom | Left,
    BottomRight = Bottom | Right,
    BottomCenter = Bottom | HorizontalCenter,

    CenterLeft = VerticalCenter | Left,
    CenterRight = VerticalCenter | Right,
    Center = VerticalCenter | HorizontalCenter
};

constexpr Alignment operator|(Alignment a, Alignment b) {
    return static_cast<Alignment>(static_cast<int>(a) | static_cast<int>(b));
}

constexpr int operator&(Alignment a, Alignment b) {
    return static_cast<int>(a) & static_cast<int>(b);
}

// Layout class that supports title safe area. Places a rectangle with the
// specified alignment and margin (percentage of client area size) based on a
// client area, clamped to a safe area. Port of GameDebugTools/Layout.cs.
struct Layout {
    Rectangle ClientArea;
    Rectangle SafeArea;

    Layout() = default;

    Layout(Rectangle clientArea, Rectangle safeArea)
        : ClientArea(clientArea), SafeArea(safeArea) {}

    explicit Layout(Rectangle clientArea)
        : Layout(clientArea, clientArea) {}

    explicit Layout(const Viewport& viewport)
        : ClientArea(viewport.getBoundsProperty()),
          SafeArea(viewport.getTitleSafeAreaProperty()) {}

    Vector2 Place(Vector2 size, float horizontalMargin, float verticalMargin, Alignment alignment) const {
        Rectangle rc(0, 0, (int)size.X, (int)size.Y);
        rc = Place(rc, horizontalMargin, verticalMargin, alignment);
        return Vector2((float)rc.X, (float)rc.Y);
    }

    Rectangle Place(Rectangle region, float horizontalMargin, float verticalMargin, Alignment alignment) const {
        // Horizontal layout.
        if ((alignment & Alignment::Left) != 0) {
            region.X = ClientArea.X + (int)((float)ClientArea.Width * horizontalMargin);
        } else if ((alignment & Alignment::Right) != 0) {
            region.X = ClientArea.X + (int)((float)ClientArea.Width * (1.0f - horizontalMargin)) - region.Width;
        } else if ((alignment & Alignment::HorizontalCenter) != 0) {
            region.X = ClientArea.X + (ClientArea.Width - region.Width) / 2 +
                       (int)(horizontalMargin * (float)ClientArea.Width);
        }

        // Vertical layout.
        if ((alignment & Alignment::Top) != 0) {
            region.Y = ClientArea.Y + (int)((float)ClientArea.Height * verticalMargin);
        } else if ((alignment & Alignment::Bottom) != 0) {
            region.Y = ClientArea.Y + (int)((float)ClientArea.Height * (1.0f - verticalMargin)) - region.Height;
        } else if ((alignment & Alignment::VerticalCenter) != 0) {
            region.Y = ClientArea.Y + (ClientArea.Height - region.Height) / 2 +
                       (int)(verticalMargin * (float)ClientArea.Height);
        }

        // Make sure the layout region is in the safe area.
        if (region.getLeftProperty() < SafeArea.getLeftProperty())
            region.X = SafeArea.getLeftProperty();

        if (region.getRightProperty() > SafeArea.getRightProperty())
            region.X = SafeArea.getRightProperty() - region.Width;

        if (region.getTopProperty() < SafeArea.getTopProperty())
            region.Y = SafeArea.getTopProperty();

        if (region.getBottomProperty() > SafeArea.getBottomProperty())
            region.Y = SafeArea.getBottomProperty() - region.Height;

        return region;
    }
};

} // namespace PerformanceMeasuring::GameDebugTools

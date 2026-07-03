#pragma once

#include "Control.hpp"

namespace UISample::Controls {

// Groups a collection of child controls. Port of Controls/PanelControl.cs.
class PanelControl : public Control {
public:
    // Positions child components in a column, with the given spacing between
    // components.
    void LayoutColumn(float xMargin, float yMargin, float ySpacing) {
        float y = yMargin;
        for (int i = 0; i < ChildCount(); i++) {
            Control& child = (*this)[i];
            child.setPosition(Vector2(xMargin, y));
            y += child.Size().Y + ySpacing;
        }
        InvalidateAutoSize();
    }

    // Positions child components in a row, with the given spacing between
    // components.
    void LayoutRow(float xMargin, float yMargin, float xSpacing) {
        float x = xMargin;
        for (int i = 0; i < ChildCount(); i++) {
            Control& child = (*this)[i];
            child.setPosition(Vector2(x, yMargin));
            x += child.Size().X + xSpacing;
        }
        InvalidateAutoSize();
    }
};

} // namespace UISample::Controls

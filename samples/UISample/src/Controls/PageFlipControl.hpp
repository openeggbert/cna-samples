#pragma once

#include "PageFlipTracker.hpp"
#include "PanelControl.hpp"

namespace UISample::Controls {

// Aligns its child controls horizontally, and lets the user flick through
// them -- similar in look and feel to the Silverlight Panorama control. Port
// of Controls/PageFlipControl.cs.
class PageFlipControl : public PanelControl {
public:
    void Update(const GameTime& gametime) override {
        tracker_.Update();
        PanelControl::Update(gametime);
    }

    void HandleInput(InputState& input) override {
        tracker_.HandleInput(input);

        if (ChildCount() > 0) {
            // Only the child that currently has focus gets input
            int current = tracker_.CurrentPage();
            (*this)[current].HandleInput(input);
        }
    }

    void Draw(DrawContext context) override {
        int childCount = ChildCount();
        if (childCount < 2) {
            // Default rendering behavior if there aren't enough children to
            // flip through.
            PanelControl::Draw(context);
            return;
        }

        Vector2 origin = context.DrawOffset;
        int iCurrent = tracker_.CurrentPage();

        float horizontalOffset = tracker_.GetCurrentPageOffset();
        context.DrawOffset = origin + Vector2(horizontalOffset, 0.0f);
        (*this)[iCurrent].Draw(context);

        if (horizontalOffset > 0) {
            // The screen has been dragged to the right, so the edge of
            // another page is visible to the left.
            int iLeft = (iCurrent + childCount - 1) % childCount;
            context.DrawOffset.X = origin.X + horizontalOffset - (float)tracker_.EffectivePageWidth(iLeft);
            (*this)[iLeft].Draw(context);
        }

        if (horizontalOffset + (*this)[iCurrent].Size().X < (float)context.Device->getViewportProperty().getWidthProperty()) {
            // The edge of another page is visible to the right. Note that
            // with two pages, a page can be drawn twice, with parts visible
            // on each edge of the screen.
            int iRight = (iCurrent + 1) % childCount;
            context.DrawOffset.X = origin.X + horizontalOffset + (float)tracker_.EffectivePageWidth(iCurrent);
            (*this)[iRight].Draw(context);
        }
    }

protected:
    void OnChildAdded(int index, Control& child) override {
        tracker_.PageWidthList().insert(tracker_.PageWidthList().begin() + index, (int)child.Size().X);
    }

    void OnChildRemoved(int index, Control& child) override {
        (void)child;
        tracker_.PageWidthList().erase(tracker_.PageWidthList().begin() + index);
    }

private:
    PageFlipTracker tracker_;
};

} // namespace UISample::Controls

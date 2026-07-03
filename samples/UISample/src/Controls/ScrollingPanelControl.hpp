#pragma once

#include "PanelControl.hpp"
#include "ScrollTracker.hpp"

namespace UISample::Controls {

// A panel that lets the user scroll through content larger than the screen
// via drag/flick gestures. Port of Controls/ScrollingPanelControl.cs.
class ScrollingPanelControl : public PanelControl {
public:
    void Update(const GameTime& gametime) override {
        Vector2 size = ComputeSize();
        scrollTracker_.CanvasRect.Width = (int)size.X;
        scrollTracker_.CanvasRect.Height = (int)size.Y;
        scrollTracker_.Update(gametime);

        PanelControl::Update(gametime);
    }

    void HandleInput(InputState& input) override {
        scrollTracker_.HandleInput(input);
        PanelControl::HandleInput(input);
    }

    void Draw(DrawContext context) override {
        // To render the scrolled panel, adjust the draw offset before
        // rendering child controls as a normal PanelControl.
        context.DrawOffset.Y = -(float)scrollTracker_.ViewRect.Y;
        PanelControl::Draw(context);
    }

private:
    ScrollTracker scrollTracker_;
};

} // namespace UISample::Controls

#pragma once

// Ported from RimLighting's UIElement.cs (Microsoft Advanced Technology Group). Base
// class for a drawable UI control (Button/Slidebar both derive from this).
//
// NOXNA: WordWrap() (UIElement.cs's own static helper) is dead code in the original --
// confirmed via a full-repo grep of RimLighting_4_0, it is declared but never called
// anywhere (not by Button.cs, Slidebar.cs, or Game1.cs) -- so it is not ported here.
//
// NOXNA: the original's virtual Draw(SpriteBatch) mixes 3D (BasicEffect) and 2D
// (SpriteBatch) drawing in Button's override, with the class's own doc comment warning
// "should not be called from within a SpriteBatch.Begin/End block". This repo's
// established F1-help-overlay convention (see CLAUDE.md) consolidates all 2D drawing
// into a single SpriteBatch Begin()/End() block per frame -- so this port splits each
// UI element's rendering into a 3D part (DrawBox(), only Button has one) and a 2D part
// (DrawText(SpriteBatch&), assumes an already-open Begin block) instead of one Draw()
// that manages its own Begin/End. See missing.md for the full rationale.

#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocation.hpp"

namespace RimLightingSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Input::Touch;

// Defines the base class for a drawable UI control.
class UIElement {
public:
    virtual ~UIElement() = default;

    // Gets or sets the position of the UIElement.
    Vector2 GetPosition() const { return position_; }
    void SetPosition(const Vector2& value) { position_ = value; needsMeasure_ = true; }

    // Gets or sets the size of the UIElement.
    Vector2 GetSize() const { return size_; }
    void SetSize(const Vector2& value) { size_ = value; needsMeasure_ = true; }

    // Gets or sets the visibility of the UIElement.
    bool IsVisible = true;

    virtual void HandleTouch(const TouchLocation& /*loc*/) {}

protected:
    // Derived classes should implement this when they can be sized & formatted.
    virtual void Measure() = 0;

    bool needsMeasure_ = false;
    Vector2 position_;
    Vector2 size_;
};

} // namespace RimLightingSample

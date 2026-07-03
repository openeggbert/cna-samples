#pragma once

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "../InputState.hpp"
#include "DrawContext.hpp"

namespace UISample::Controls {

using Microsoft::Xna::Framework::Vector2;

// Control is the base class for a simple UI controls framework. Controls are
// grouped into a hierarchy: each control has a Parent and an optional list of
// Children. Port of Controls/Control.cs.
//
// Layout: controls have a Position and Size, which define a rectangle. By
// default Size is computed automatically via ComputeSize(); writing to Size
// disables auto-sizing until InvalidateAutoSize() is called again. There is
// no dynamic layout system -- container controls (PanelControl in
// particular) provide methods for arranging their children into rows,
// columns, etc.
class Control : public std::enable_shared_from_this<Control> {
public:
    // Draw() is not called unless Visible is true (the default).
    bool Visible = true;

    virtual ~Control() = default;

    // Position of this control within its parent control.
    Vector2 Position() const { return position_; }
    void setPosition(Vector2 value) {
        position_ = value;
        if (parent_ != nullptr) parent_->InvalidateAutoSize();
    }

    // Size of this control. See the class comment for a discussion of the
    // layout system.
    Vector2 Size() {
        if (!sizeValid_) {
            size_ = ComputeSize();
            sizeValid_ = true;
        }
        return size_;
    }
    // Setting the size overrides whatever ComputeSize() would return, and
    // disables auto-sizing.
    void setSize(Vector2 value) {
        size_ = value;
        sizeValid_ = true;
        autoSize_ = false;
        if (parent_ != nullptr) parent_->InvalidateAutoSize();
    }

    // The control containing this control, if any.
    Control* Parent() const { return parent_; }

    int ChildCount() const { return (int)children_.size(); }
    Control& operator[](int childIndex) { return *children_[childIndex]; }

    void AddChild(std::shared_ptr<Control> child) { AddChild(std::move(child), ChildCount()); }

    void AddChild(std::shared_ptr<Control> child, int index) {
        if (child->parent_ != nullptr) {
            child->parent_->RemoveChild(child);
        }
        child->parent_ = this;
        children_.insert(children_.begin() + index, child);
        OnChildAdded(index, *child);
    }

    void RemoveChildAt(int index) {
        std::shared_ptr<Control> child = children_[index];
        child->parent_ = nullptr;
        children_.erase(children_.begin() + index);
        OnChildRemoved(index, *child);
    }

    // Removes the given control from this control's list of children.
    void RemoveChild(const std::shared_ptr<Control>& child) {
        if (child->parent_ != this) {
            throw std::runtime_error("Control::RemoveChild: not a child of this control");
        }
        auto it = std::find(children_.begin(), children_.end(), child);
        RemoveChildAt((int)std::distance(children_.begin(), it));
    }

    virtual void Draw(DrawContext context) {
        Vector2 origin = context.DrawOffset;
        for (auto& child : children_) {
            if (child->Visible) {
                context.DrawOffset = origin + child->Position();
                child->Draw(context);
            }
        }
    }

    // Called once per frame to update the control; override to add custom
    // update logic, calling Control::Update() to update child controls.
    virtual void Update(const GameTime& gameTime) {
        for (auto& child : children_) {
            child->Update(gameTime);
        }
    }

    // Called once per frame to route input to the control; override to add
    // custom input handling, calling Control::HandleInput() to route input
    // to child controls.
    //
    // Note that Controls have no special system for setting
    // TouchPanel::EnabledGestures — if you're using gesture-sensitive
    // controls, set EnabledGestures as appropriate on each GameScreen.
    virtual void HandleInput(InputState& input) {
        for (auto& child : children_) {
            child->HandleInput(input);
        }
    }

    // Called when Size is read and sizeValid_ is false. Call
    // Control::ComputeSize() to compute the size (the lower-right corner) of
    // all child controls.
    virtual Vector2 ComputeSize() {
        if (children_.empty()) {
            return Vector2::Zero;
        }
        Vector2 bounds = children_[0]->Position() + children_[0]->Size();
        for (std::size_t i = 1; i < children_.size(); i++) {
            Vector2 corner = children_[i]->Position() + children_[i]->Size();
            bounds.X = std::max(bounds.X, corner.X);
            bounds.Y = std::max(bounds.Y, corner.Y);
        }
        return bounds;
    }

    // Call this when a control's content changes so its size needs to be
    // recomputed. Has no effect if auto-sizing has been disabled.
    void InvalidateAutoSize() {
        if (autoSize_) {
            sizeValid_ = false;
            if (parent_ != nullptr) parent_->InvalidateAutoSize();
        }
    }

    // Call this once per frame on the root of the control hierarchy to draw
    // all the controls. See SingleControlScreen for an example.
    static void BatchDraw(Control* control, GraphicsDevice& device, SpriteBatch& spriteBatch,
                          Texture2D& blankTexture, Vector2 offset, const GameTime& gameTime) {
        if (control != nullptr && control->Visible) {
            spriteBatch.Begin();
            DrawContext context;
            context.Device = &device;
            context.SpriteBatchValue = &spriteBatch;
            context.BlankTexture = &blankTexture;
            context.DrawOffset = offset + control->Position();
            context.GameTimeValue = &gameTime;
            control->Draw(context);
            spriteBatch.End();
        }
    }

protected:
    // Called after a child control is added to this control. Default
    // behavior is to call InvalidateAutoSize().
    virtual void OnChildAdded(int index, Control& child) {
        (void)index;
        (void)child;
        InvalidateAutoSize();
    }

    // Called after a child control is removed from this control. Default
    // behavior is to call InvalidateAutoSize().
    virtual void OnChildRemoved(int index, Control& child) {
        (void)index;
        (void)child;
        InvalidateAutoSize();
    }

private:
    Vector2 position_;
    Vector2 size_;
    bool sizeValid_ = false;
    bool autoSize_ = true;
    Control* parent_ = nullptr;
    std::vector<std::shared_ptr<Control>> children_;
};

} // namespace UISample::Controls

#pragma once

#include <cmath>
#include <vector>

#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/DateTime.hpp"
#include "System/TimeSpan.hpp"

#include "../InputState.hpp"

namespace UISample::Controls {

using Microsoft::Xna::Framework::Input::Touch::GestureType;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// Watches the touch panel (and, via InputState's mouse fallback, the mouse --
// see missing.md) for drag and flick gestures, and computes the offsets for
// flipping horizontally through a multi-page display. Used by PageFlipControl
// to handle the scroll logic; broken out into its own class so it can be
// reused without the Control classes or GameStateManagement. Uses
// TouchPanel::getDisplayWidthProperty() for the screen width, which updates
// automatically on orientation change. Port of Controls/PageFlipTracker.cs.
class PageFlipTracker {
public:
    // This class watches for HorizontalDrag, DragComplete, and Flick
    // gestures, but doesn't set TouchPanel::EnabledGestures itself (that
    // could interfere with gestures needed elsewhere) -- client code must
    // set it, using this constant.
    static constexpr GestureType GesturesNeeded =
        GestureType::Flick | GestureType::HorizontalDrag | GestureType::DragComplete;

    static System::TimeSpan FlipDuration;

    // Exponent on the curve that makes page flips and springbacks start
    // quickly and slow to a stop. Interpolation formula is
    // (1-TransitionAlpha)^FlipExponent, where TransitionAlpha animates
    // uniformly from 0 to 1 over FlipDuration.
    static constexpr double FlipExponent = 3.0;

    // By default, this many pixels of the next page are visible on the
    // right-hand edge of the screen, unless the current page's width is too
    // large.
    static constexpr int PreviewMargin = 20;

    // How far (as a fraction of total screen width) you have to drag a
    // screen past its edge to trigger a flip by dragging.
    static constexpr float DragToFlipThreshold = 1.0f / 3.0f;

    // Current active page. If in a transition, this is the page transitioning TO.
    int CurrentPage() const { return currentPage_; }

    // Offset in pixels to render the current page at, relative to the
    // current page. If positive, other pages may be visible to the left.
    float GetCurrentPageOffset() const { return currentPageOffset_; }

    bool IsLeftPageVisible() const {
        return pageWidthList_.size() >= 2 && currentPageOffset_ > 0.0f;
    }

    bool IsRightPageVisible() const {
        return pageWidthList_.size() >= 2 &&
               currentPageOffset_ + (float)EffectivePageWidth(currentPage_) <= (float)TouchPanel::getDisplayWidthProperty();
    }

    bool InFlip() const { return inFlip_; }

    // Alpha value that animates from 0 to 1 during a spring. 1 when not springing.
    float FlipAlpha() const { return flipAlpha_; }

    // Width in pixels of each page. Pages can be added/removed at any time.
    std::vector<int>& PageWidthList() { return pageWidthList_; }

    int EffectivePageWidth(int page) const {
        int displayWidth = TouchPanel::getDisplayWidthProperty() - PreviewMargin;
        return std::max(displayWidth, pageWidthList_[(std::size_t)page]);
    }

    // Called once per frame.
    void Update() {
        if (inFlip_) {
            System::TimeSpan transitionClock = System::DateTime::getNowProperty() - flipStartTime_;
            if (transitionClock >= FlipDuration) {
                EndFlip();
            } else {
                double f = transitionClock.getTotalSecondsProperty() / FlipDuration.getTotalSecondsProperty();
                f = std::max(f, 0.0); // this shouldn't happen, but just in case time goes crazy
                flipAlpha_ = (float)(1.0 - std::pow(1.0 - f, FlipExponent));
                currentPageOffset_ = flipStartOffset_ * (1.0f - flipAlpha_);
            }
        }
    }

    void HandleInput(InputState& input) {
        for (const GestureSample& sample : input.Gestures) {
            switch (sample.getGestureTypeProperty()) {
                case GestureType::HorizontalDrag:
                    currentPageOffset_ += sample.getDeltaProperty().X;
                    flipStartOffset_ = currentPageOffset_;
                    break;

                case GestureType::DragComplete:
                    if (!inFlip_) {
                        if (currentPageOffset_ < -(float)TouchPanel::getDisplayWidthProperty() * DragToFlipThreshold) {
                            BeginFlip(1); // flip to next page
                        } else if (currentPageOffset_ + (float)TouchPanel::getDisplayWidthProperty() * (1.0f - DragToFlipThreshold) >
                                   (float)EffectivePageWidth(currentPage_)) {
                            BeginFlip(-1); // flip to previous page
                        } else {
                            BeginFlip(0); // "snap back" effect
                        }
                    }
                    break;

                case GestureType::Flick:
                    // Only respond to mostly-horizontal flicks
                    if (std::abs(sample.getDeltaProperty().X) > std::abs(sample.getDeltaProperty().Y)) {
                        if (sample.getDeltaProperty().X > 0) {
                            BeginFlip(-1);
                        } else {
                            BeginFlip(1);
                        }
                    }
                    break;

                default:
                    break;
            }
        }
    }

private:
    void BeginFlip(int pageDelta) {
        if (pageWidthList_.empty()) return;

        int pageFrom = currentPage_;
        currentPage_ = (int)(((currentPage_ + pageDelta) % (int)pageWidthList_.size() + (int)pageWidthList_.size()) %
                              (int)pageWidthList_.size());

        if (pageDelta > 0) {
            // going to next page; offset starts out large
            currentPageOffset_ += (float)EffectivePageWidth(pageFrom);
        } else if (pageDelta < 0) {
            // going to previous page; offset starts out negative
            currentPageOffset_ -= (float)EffectivePageWidth(currentPage_);
        }
        inFlip_ = true;
        flipAlpha_ = 0.0f;
        flipStartOffset_ = currentPageOffset_;
        flipStartTime_ = System::DateTime::getNowProperty();
    }

    void EndFlip() {
        inFlip_ = false;
        flipAlpha_ = 1.0f;
        currentPageOffset_ = 0.0f;
    }

    System::DateTime flipStartTime_;
    float flipStartOffset_ = 0.0f;

    int currentPage_ = 0;
    float currentPageOffset_ = 0.0f;
    bool inFlip_ = false;
    float flipAlpha_ = 1.0f;
    std::vector<int> pageWidthList_;
};

inline System::TimeSpan PageFlipTracker::FlipDuration = System::TimeSpan::FromSeconds(0.3);

} // namespace UISample::Controls

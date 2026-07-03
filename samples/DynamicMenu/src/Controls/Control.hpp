#pragma once

#include <algorithm>
#include <any>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "SharpRuntime/SharpRuntimeHelper.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp"

#include "../Transitions/Transition.hpp"

namespace DynamicMenu::Controls {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Graphics::GraphicsDevice;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteEffects;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;

// The base class for all controls. Port of Controls/Control.cs, merged with
// its IControl.cs interface -- nothing in this sample implements IControl
// independently of Control, so the split serves no purpose in C++ (which has
// no separate "properties" mechanism for interfaces to declare anyway).
class Control {
public:
    int Left = 0;
    int Top = 0;
    int Width = 0;
    int Height = 0;
    std::string Name;
    std::string BackTextureName;
    std::optional<Texture2D> BackTexture;
    bool Visible = true;
    Color Hue = Color::White;
    Control* Parent = nullptr;

    // Mirrors the original's untyped `object Tag`: on the page-select buttons
    // this holds an EDynamicControlPage enum value; on Page 3's advance button
    // it holds a Control* pointing at the progress bar it advances.
    std::any Tag;

    virtual ~Control() = default;

    [[nodiscard]] int Bottom() const { return Top + Height; }
    [[nodiscard]] int Right() const { return Left + Width; }

    virtual void Initialize() {
        // Do nothing here
    }

    virtual void LoadContent(GraphicsDevice& graphics, ContentManager& content) {
        (void)graphics;
        if (!BackTextureName.empty()) {
            BackTexture.emplace(content.Load<Texture2D>(BackTextureName));
        }
    }

    virtual void Update(const GameTime& gameTime, const std::vector<GestureSample>& gestures) {
        (void)gestures;

        // Iterate a snapshot of the active-transitions list, not the live one:
        // a transition's TransitionComplete callback can itself call
        // ApplyTransition() (see DynamicMenuGame's GetBigTransitionComplete),
        // which would otherwise mutate activeTransitions_ while this loop is
        // walking it. The original C# does the same (copies into a
        // `curTransitions` list before iterating) -- there, Transition is a
        // reference type, so the copy only protects the *list*, not the
        // shared Transition objects; std::shared_ptr here gets the same effect.
        std::vector<std::shared_ptr<Transitions::Transition>> curTransitions = activeTransitions_;
        std::vector<std::shared_ptr<Transitions::Transition>> toRemove;

        for (auto& transition : curTransitions) {
            transition->Update(gameTime);
            if (!transition->IsTransitionActive()) {
                toRemove.push_back(transition);
            }
        }

        for (auto& done : toRemove) {
            auto it = std::find(activeTransitions_.begin(), activeTransitions_.end(), done);
            if (it != activeTransitions_.end()) {
                activeTransitions_.erase(it);
            }
        }
    }

    virtual void Draw(const GameTime& gameTime, SpriteBatch& spriteBatch) {
        (void)gameTime;
        Texture2D* currTexture = GetCurrTexture();
        if (currTexture != nullptr) {
            // The original's call also passes rotation=0, origin=(0,0),
            // effects=None, depth=0 -- all no-ops for an axis-aligned,
            // unflipped background sprite -- and CNA has no destRect+rotation
            // overload, so this drops straight to the destRect+source+color one.
            Rectangle rect = GetAbsoluteRect();
            spriteBatch.Draw(*currTexture, rect, std::nullopt, Hue);
        }
    }

    virtual Texture2D* GetCurrTexture() {
        return BackTexture.has_value() ? &*BackTexture : nullptr;
    }

    [[nodiscard]] Point GetAbsoluteTopLeft() const {
        Point absoluteTopLeft(Left, Top);
        if (Parent != nullptr) {
            Point parentTopLeft = Parent->GetAbsoluteTopLeft();
            absoluteTopLeft.X += parentTopLeft.X;
            absoluteTopLeft.Y += parentTopLeft.Y;
        }
        return absoluteTopLeft;
    }

    [[nodiscard]] Rectangle GetAbsoluteRect() const {
        Point topLeft = GetAbsoluteTopLeft();
        return Rectangle(topLeft.X, topLeft.Y, Width, Height);
    }

    // Starts the passed-in transition on this control.
    void ApplyTransition(Transitions::Transition transition) {
        auto applied = std::make_shared<Transitions::Transition>(std::move(transition));
        activeTransitions_.push_back(applied);
        applied->Control = this;
        applied->StartTransition();
    }

protected:
    void DrawCenteredText(SpriteBatch& spriteBatch, const SpriteFont* font, const Rectangle& rect,
                          const std::string& text, Color color) {
        if (font == nullptr || text.empty()) return;

        Vector2 midPoint((float)(rect.X + rect.Width / 2), (float)(rect.Y + rect.Height / 2));
        Vector2 stringSize = font->MeasureString(text);
        Vector2 fontPos(midPoint.X - stringSize.X * 0.5f, midPoint.Y - stringSize.Y * 0.5f);

        spriteBatch.DrawString(*font, text, fontPos, color);
    }

    [[nodiscard]] bool ContainsPos(const Vector2& pos) const {
        Rectangle rect = GetAbsoluteRect();
        return rect.Contains((int)pos.X, (int)pos.Y);
    }

private:
    std::vector<std::shared_ptr<Transitions::Transition>> activeTransitions_;
};

} // namespace DynamicMenu::Controls

// Out-of-line Transition method definitions -- Control must be a complete
// type first (see the forward-declaration note at the top of Transition.hpp).
namespace DynamicMenu::Transitions {

inline void Transition::StartTransition() {
    transitionActive_ = true;
    transitionStartTime_ = 0.0;

    if (startPositionSet_) {
        Control->Left = startPosition_.X;
        Control->Top = startPosition_.Y;
    } else {
        startPosition_.X = Control->Left;
        startPosition_.Y = Control->Top;
    }

    if (startSizeSet_) {
        Control->Width = startSize_.X;
        Control->Height = startSize_.Y;
    } else {
        startSize_.X = Control->Width;
        startSize_.Y = Control->Height;
    }

    if (startHueSet_) {
        Control->Hue = startHue_;
    } else {
        startHue_ = Control->Hue;
    }

    if (!endPositionSet_) {
        endPosition_.X = Control->Left;
        endPosition_.Y = Control->Top;
    }
    if (!endSizeSet_) {
        endSize_.X = Control->Width;
        endSize_.Y = Control->Height;
    }
    if (!endHueSet_) {
        endHue_ = Control->Hue;
    }
}

inline void Transition::Update(const GameTime& gameTime) {
    if (!transitionActive_) return;

    if (transitionStartTime_ == 0.0) {
        transitionStartTime_ = gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();
    }

    float timeSinceStart = (float)(gameTime.getTotalGameTimeProperty().getTotalSecondsProperty() - transitionStartTime_);
    float percentComplete = timeSinceStart / TransitionLength;

    if (percentComplete > 1.0f) {
        Control->Left = endPosition_.X;
        Control->Top = endPosition_.Y;
        Control->Width = endSize_.X;
        Control->Height = endSize_.Y;
        Control->Hue = endHue_;
        transitionStartTime_ = 0.0;
        transitionActive_ = false;
        if (TransitionComplete) {
            TransitionComplete(*this);
        }
    } else {
        Control->Left = (int)(startPosition_.X + (endPosition_.X - startPosition_.X) * percentComplete);
        Control->Top = (int)(startPosition_.Y + (endPosition_.Y - startPosition_.Y) * percentComplete);
        Control->Width = (int)(startSize_.X + (endSize_.X - startSize_.X) * percentComplete);
        Control->Height = (int)(startSize_.Y + (endSize_.Y - startSize_.Y) * percentComplete);

        float t = percentComplete;
        auto lerp = [t](SharpRuntime::bytecs a, SharpRuntime::bytecs b) {
            return (SharpRuntime::bytecs)(a + (b - a) * t);
        };
        Control->Hue = Color(lerp(startHue_.getRProperty(), endHue_.getRProperty()),
                              lerp(startHue_.getGProperty(), endHue_.getGProperty()),
                              lerp(startHue_.getBProperty(), endHue_.getBProperty()),
                              lerp(startHue_.getAProperty(), endHue_.getAProperty()));
    }
}

inline Transition Transition::CreateFadeIn(Controls::Control& control, std::optional<float> length) {
    Color startHue = control.Hue;
    Color endHue = control.Hue;
    startHue.setAProperty(0);

    Transition transition(std::nullopt, std::nullopt, std::nullopt, std::nullopt, startHue, endHue);
    transition.Control = &control;
    if (length) transition.TransitionLength = *length;
    return transition;
}

inline Transition Transition::CreateFadeOut(Controls::Control& control, std::optional<float> length) {
    Color startHue = control.Hue;
    Color endHue = control.Hue;
    endHue.setAProperty(0);

    Transition transition(std::nullopt, std::nullopt, std::nullopt, std::nullopt, startHue, endHue);
    transition.Control = &control;
    if (length) transition.TransitionLength = *length;
    return transition;
}

inline Transition Transition::CreateFlyIn(Controls::Control& control, Point startPos, std::optional<float> length) {
    Transition transition(startPos, std::nullopt, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    transition.Control = &control;
    if (length) transition.TransitionLength = *length;
    return transition;
}

inline Transition Transition::CreateFlyOut(Controls::Control& control, Point endPos, std::optional<float> length) {
    Transition transition(std::nullopt, endPos, std::nullopt, std::nullopt, std::nullopt, std::nullopt);
    transition.Control = &control;
    if (length) transition.TransitionLength = *length;
    return transition;
}

} // namespace DynamicMenu::Transitions

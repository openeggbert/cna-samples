#pragma once

// Ported from RimLighting's Button.cs (Microsoft Advanced Technology Group). A Button is
// a UI Element that has an up & down state, an event handler, and a string of text.
//
// See UIElement.hpp's own header comment for why Draw() is split into DrawBox() (3D,
// called before any SpriteBatch Begin()) and DrawText(SpriteBatch&) (2D, called inside
// this sample's single per-frame SpriteBatch Begin()/End() block) instead of one
// Draw(SpriteBatch) like the C# original.

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionColor.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocation.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchLocationState.hpp"

#include "UIElement.hpp"

#include <array>
#include <functional>
#include <memory>
#include <string>

namespace RimLightingSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input::Touch;

class Button : public UIElement {
public:
    using ClickEventHandler = std::function<void()>;

    // Creates a new button object.
    Button(GraphicsDevice& device, SpriteFont& font, const std::string& text) : buttonFont_(font) {
        SetText(text);
        SetSize(buttonFont_.MeasureString(text_));

        effect_ = std::make_unique<BasicEffect>(device);
        effect_->Projection = Matrix::CreateOrthographicOffCenter(
            0.0f, static_cast<float>(device.getViewportProperty().getWidthProperty()),
            static_cast<float>(device.getViewportProperty().getHeightProperty()), 0.0f, 1.0f, 1000.0f);
        effect_->View = Matrix::CreateLookAt(Vector3(0, 0, 5), Vector3::Zero, Vector3::Up);
        effect_->VertexColorEnabled = true;

        for (auto& v : verts_) {
            v.Color = Color(255, 255, 255, 255);
        }
    }

    // Gets or sets the text displayed on the button.
    std::string GetText() const { return text_; }
    void SetText(const std::string& value) {
        text_ = value;
        needsMeasure_ = true;
    }

    // OnClick fires when the button is pressed and released.
    ClickEventHandler OnClick;

    // Check to see if the button was pressed or released.
    void HandleTouch(const TouchLocation& loc) override {
        if (loc.getStateProperty() == TouchLocationState::Pressed) {
            if (pressId_ == kNoPress && HitTest(loc.getPositionProperty())) {
                if (IsVisible) {
                    pressId_ = loc.getIdProperty();
                }
            }
        } else if (loc.getStateProperty() == TouchLocationState::Released) {
            if (pressId_ == loc.getIdProperty()) {
                pressId_ = kNoPress;

                if (HitTest(loc.getPositionProperty()) && OnClick) {
                    OnClick();
                }
            }
        }
    }

    // Draws the button's 3D box (a BasicEffect-rendered outline or filled quad). Must be
    // called outside/before any SpriteBatch Begin()/End() block -- see this file's own
    // header comment.
    void DrawBox() {
        if (!IsVisible) {
            return;
        }

        if (needsMeasure_) {
            Measure();
        }

        auto& device = effect_->getGraphicsDeviceInternal();
        for (auto& pass : effect_->getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();

            if (pressId_ == kNoPress) {
                device.DrawUserPrimitives(PrimitiveType::LineStrip, verts_.data(), 0, 4);
            } else {
                device.DrawUserPrimitives(PrimitiveType::TriangleList, verts_.data(), 0, 2);
            }
        }
    }

    // Draws the button's text. Must be called inside an already-open SpriteBatch
    // Begin()/End() block -- see this file's own header comment.
    void DrawText(SpriteBatch& spriteBatch) {
        if (!IsVisible) {
            return;
        }

        spriteBatch.DrawString(buttonFont_, text_, GetPosition() + textOffset_,
                                pressId_ == kNoPress ? Color::White : Color::Black);
    }

protected:
    // Called when the button's attributes are 'dirty' and the visuals need to be
    // updated. NOXNA note: matching the C# original exactly, this never resets
    // needsMeasure_ back to false -- so it recomputes the (idempotent) box/text-offset
    // geometry on every Draw() call once the text has been set at least once. Harmless
    // (cheap, deterministic recomputation), kept faithfully rather than "fixed".
    void Measure() override {
        if (!text_.empty()) {
            Vector2 measured = buttonFont_.MeasureString(text_);
            textOffset_ = (GetSize() - measured) / 2.0f;
        }

        Vector2 pos = GetPosition();
        Vector2 size = GetSize();

        // This array will be used to draw either a line strip or a triangle list.
        verts_[0].Position = Vector3(pos.X, pos.Y, 0);
        verts_[1].Position = Vector3(pos.X + size.X, pos.Y, 0);
        verts_[2].Position = Vector3(pos.X + size.X, pos.Y + size.Y, 0);
        verts_[3].Position = Vector3(pos.X, pos.Y + size.Y, 0);
        verts_[4].Position = verts_[0].Position;
        verts_[5].Position = verts_[2].Position;
    }

private:
    static constexpr int kNoPress = 0;

    // Check to see if the button's rectangle contains the given point.
    bool HitTest(const Vector2& point) const {
        Vector2 pos = GetPosition();
        Vector2 size = GetSize();
        return point.X >= pos.X && point.X < pos.X + size.X && point.Y >= pos.Y && point.Y < pos.Y + size.Y;
    }

    SpriteFont& buttonFont_;
    std::unique_ptr<BasicEffect> effect_;
    std::array<VertexPositionColor, 6> verts_;
    Vector2 textOffset_;
    std::string text_;

    // NOXNA: the synthesized single mouse-touch this sample feeds (see
    // RimLightingGame.hpp) always uses a nonzero id, specifically to avoid colliding
    // with kNoPress (0) -- the same ambiguity the C# original's own `pressId == 0`
    // sentinel would have on real touch hardware if a device ever assigned touch id 0.
    int pressId_ = kNoPress;
};

} // namespace RimLightingSample

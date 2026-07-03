#pragma once

#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"

#include "TextControl.hpp"

namespace DynamicMenu::Controls {

using Microsoft::Xna::Framework::Input::Touch::GestureType;

// A control which reads touch/gesture input within its bounds and fires a
// Tapped event when touched. Port of Controls/Button.cs.
class Button : public TextControl {
public:
    // Fired when the button is tapped (mirrors the original's Tapped C# event).
    std::function<void(Button&)> Tapped;

    std::string PressedTextureName;
    std::optional<Texture2D> PressedTexture;
    bool Pressed = false;

    void LoadContent(GraphicsDevice& graphics, ContentManager& content) override {
        TextControl::LoadContent(graphics, content);
        if (!PressedTextureName.empty()) {
            PressedTexture.emplace(content.Load<Texture2D>(PressedTextureName));
        }
    }

    void Update(const GameTime& gameTime, const std::vector<GestureSample>& gestures) override {
        TextControl::Update(gameTime, gestures);

        for (const GestureSample& sample : gestures) {
            if (sample.getGestureTypeProperty() != GestureType::Tap) continue;

            if (ContainsPos(sample.getPositionProperty())) {
                Pressed = true;
                pressStartTime_ = gameTime.getTotalGameTimeProperty().getTotalSecondsProperty();
                HandlePressed();
                break;
            }
        }

        if (Pressed && gameTime.getTotalGameTimeProperty().getTotalSecondsProperty() > pressStartTime_ + PressTimeSeconds) {
            Pressed = false;
            if (Tapped) {
                Tapped(*this);
            }
        }
    }

    void Draw(const GameTime& gameTime, SpriteBatch& spriteBatch) override {
        Control::Draw(gameTime, spriteBatch);
        DrawCenteredText(spriteBatch, Font.has_value() ? &*Font : nullptr, GetAbsoluteRect(), Text, TextColor);
    }

    Texture2D* GetCurrTexture() override {
        if (Pressed && PressedTexture.has_value()) {
            return &*PressedTexture;
        }
        return Control::GetCurrTexture();
    }

protected:
    // Can be overridden to do special processing when this is clicked.
    virtual void HandlePressed() {}

private:
    // Keep a button pressed for this long in seconds before executing the event.
    static constexpr double PressTimeSeconds = 0.2;

    double pressStartTime_ = 0.0;
};

} // namespace DynamicMenu::Controls

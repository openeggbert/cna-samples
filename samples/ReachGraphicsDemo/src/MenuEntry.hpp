#pragma once

// Ported from XnaGraphicsDemo.MenuEntry (MenuEntry.cs). Base class for each entry in a
// MenuComponent's menu list, plus the BoolMenuEntry/FloatMenuEntry subclasses used by
// several of the demo screens.
//
// MenuEntry::OnClicked() is declared here but DEFINED out-of-line at the bottom of
// DemoGame.hpp (it calls the static DemoGame::SpawnZoomyText helper, and MenuEntry.hpp
// can only forward-declare DemoGame -- see DemoGame.hpp's own comment for the full
// reasoning, the same header-split shape this repo's GameStateManagement port already
// established for GameScreen.hpp/ScreenManager.hpp).

#include <functional>
#include <string>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

// Base class for each entry in a MenuComponent.
class MenuEntry {
public:
    // Constants.
    static constexpr int Height = 64;
    static constexpr int Border = 32;

    virtual ~MenuEntry() = default;

    // Properties.
    virtual std::string GetText() const { return text_; }
    virtual void SetText(const std::string& value) { text_ = value; }

    Vector2 Position;
    bool IsFocused = false;
    bool IsDraggable = false;
    std::function<void()> Clicked;

    Color GetColor() const { return IsFocused ? Color::Blue : Color::White; }

    // Draws the menu entry.
    virtual void Draw(SpriteBatch& spriteBatch, SpriteFont& font, Texture2D& /*blankTexture*/) {
        positionOffset_ = Vector2(0.0f, (Height - font.getLineSpacingProperty()) / 2.0f);

        spriteBatch.DrawString(font, GetText(), Vector2(Position.X + positionOffset_.X, Position.Y + positionOffset_.Y),
                                GetColor());
    }

    // Handles clicks on this menu entry. Defined out-of-line in DemoGame.hpp (calls
    // DemoGame::SpawnZoomyText).
    virtual void OnClicked();

    // Handles dragging this menu entry from left to right.
    virtual void OnDragged(float /*delta*/) {}

protected:
    Vector2 positionOffset_;

private:
    std::string text_;
};

// Menu entry subclass for boolean toggle values.
class BoolMenuEntry : public MenuEntry {
public:
    explicit BoolMenuEntry(std::string label) : label_(std::move(label)) {}

    bool Value = false;

    // Click handler toggles the boolean value.
    void OnClicked() override {
        Value = !Value;
        MenuEntry::OnClicked();
    }

    // Customize our text string.
    std::string GetText() const override { return label_ + " " + (Value ? "on" : "off"); }
    void SetText(const std::string&) override {}

private:
    std::string label_;
};

// Menu entry subclass for floating point slider values.
class FloatMenuEntry : public MenuEntry {
public:
    FloatMenuEntry() { IsDraggable = true; }

    float Value = 0.0f;

    // Drag handler changes the slider position.
    void OnDragged(float delta) override {
        constexpr float speed = 1.0f / 300.0f;
        Value = MathHelper::Clamp(Value + delta * speed, 0.0f, 1.0f);
    }

    // Custom draw function displays a slider bar in addition to the item text.
    void Draw(SpriteBatch& spriteBatch, SpriteFont& font, Texture2D& blankTexture) override {
        MenuEntry::Draw(spriteBatch, font, blankTexture);

        Vector2 size = font.MeasureString(GetText());
        size.Y /= 2.0f;

        Vector2 pos = Vector2(Position.X + size.X, Position.Y + size.Y);

        pos.X += 8.0f;
        pos.Y += (Height - font.getLineSpacingProperty()) / 2.0f;

        float w = 480.0f - Border - pos.X;

        spriteBatch.Draw(blankTexture,
                          Rectangle(static_cast<int>(pos.X), static_cast<int>(pos.Y) - 3,
                                    static_cast<int>(w * Value), 6),
                          GetColor());
    }
};

} // namespace ReachGraphicsDemoSample

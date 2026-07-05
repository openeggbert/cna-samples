#pragma once

// MenuEntry.hpp -- C++ port of ScreenManager/MenuEntry.cs.

#include <cmath>
#include <functional>
#include <memory>
#include <string>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "../Fonts.hpp"

namespace RolePlaying {

using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class MenuScreen; // fwd decl, see MenuScreen.hpp
class ScreenManager;

// Helper class represents a single entry in a MenuScreen.
class MenuEntry {
public:
    explicit MenuEntry(std::string text) : text_(std::move(text)) {}
    virtual ~MenuEntry() = default;

    const std::string& Text() const { return text_; }
    void SetText(std::string v) { text_ = std::move(v); }

    SpriteFont* Font = nullptr; // non-owning -- points at a Fonts:: static, which outlives all screens
    Vector2 Position;
    std::string Description;
    std::shared_ptr<Texture2D> EntryTexture;

    std::function<void()> Selected;

    virtual void OnSelectEntry() {
        if (Selected) Selected();
    }

    virtual void Update(MenuScreen& screen, bool isSelected, GameTime& gameTime) { (void)screen; (void)isSelected; (void)gameTime; }

    // Defined out-of-line in MenuScreen.hpp (needs ScreenManager fully defined).
    virtual void Draw(MenuScreen& screen, bool isSelected, const GameTime& gameTime);

    virtual int GetHeight(MenuScreen& screen) const {
        (void)screen;
        return Font ? Font->getLineSpacingProperty() : 0;
    }

private:
    std::string text_;
};

} // namespace RolePlaying

#pragma once

#include "Clickable.hpp"

#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>

#include <optional>
#include <string>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

namespace Graphics3DSample {

// A game component, inherits from Clickable. Has associated "on" content and an
// IsChecked state that's toggled by a click. Draws content tinted yellow when
// checked, white otherwise.
class Checkbox : public Clickable {
public:
    Checkbox(Game& game, std::string textureName, const Rectangle& targetRectangle, bool isChecked)
        : Clickable(game, targetRectangle), asset_(std::move(textureName)), isChecked_(isChecked) {}

    const std::string& GetTypeName() const override {
        static const std::string name = "Checkbox";
        return name;
    }

    bool IsChecked() const { return isChecked_; }

    void Update(GameTime& gameTime) override {
        HandleInput();
        if (IsClicked()) isChecked_ = !isChecked_;
        DrawableGameComponent::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        spriteBatch_->Begin();
        spriteBatch_->Draw(*textureOn_, GetRectangle(), IsChecked() ? Color::Yellow : Color::White);
        spriteBatch_->End();
        DrawableGameComponent::Draw(gameTime);
    }

protected:
    void LoadContent() override {
        spriteBatch_.emplace(getGraphicsDeviceProperty());
        textureOn_.emplace(getGameProperty().getContentProperty().Load<Texture2D>(asset_));
    }

private:
    std::string asset_;
    bool isChecked_;
    std::optional<Texture2D> textureOn_;
    std::optional<SpriteBatch> spriteBatch_;
};

} // namespace Graphics3DSample

#pragma once

// HitPointsComponent.hpp — C++ port of Elements/HUD/HitPointsComponent.cs
// (XNA 4.0 NinjAcademy sample). A component used to display and maintain
// the player's hit points.

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "../../GameConstants.hpp"
#include "../RestorableStateComponent.hpp"

namespace NinjAcademy {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// Port of Elements/HUD/HitPointsComponent.cs.
class HitPointsComponent : public RestorableStateComponent {
public:
    int TotalHitPoints = 0;
    int CurrentHitPoints = 0;

    HitPointsComponent(Game& game, Texture2D fullTexture, Texture2D emptyTexture)
        : RestorableStateComponent(game), fullTexture_(std::move(fullTexture)), emptyTexture_(std::move(emptyTexture)),
          spriteBatch_(getGameProperty().getServicesProperty().GetService<SpriteBatch>()) {}

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        int missingHP = TotalHitPoints - CurrentHitPoints;
        Vector2 drawingPosition = GameConstants::HitPointsOrigin;

        spriteBatch_->Begin();

        for (int i = 0; i < missingHP; i++) {
            spriteBatch_->Draw(emptyTexture_, drawingPosition, Color::White);
            drawingPosition.X -= (float)(emptyTexture_.getWidthProperty()) + GameConstants::HitPointsSpace;
        }
        for (int i = 0; i < CurrentHitPoints; i++) {
            spriteBatch_->Draw(fullTexture_, drawingPosition, Color::White);
            drawingPosition.X -= (float)(fullTexture_.getWidthProperty()) + GameConstants::HitPointsSpace;
        }

        spriteBatch_->End();
    }

private:
    Texture2D fullTexture_;
    Texture2D emptyTexture_;
    SpriteBatch* spriteBatch_;
};

} // namespace NinjAcademy

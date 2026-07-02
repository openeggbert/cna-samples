#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BlendState.hpp"
#include "Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteSortMode.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/TouchPanel.hpp"
#include "System/Random.hpp"
#include "System/TimeSpan.hpp"

#include "Bullet.hpp"
#include "EnemyShip.hpp"
#include "PlayerShip.hpp"
#include "VirtualThumbsticks.hpp"

namespace TouchThumbsticks {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::GraphicsDeviceManager;
using Microsoft::Xna::Framework::Matrix;
using Microsoft::Xna::Framework::PlayerIndex;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Graphics::BlendState;
using Microsoft::Xna::Framework::Graphics::DepthStencilState;
using Microsoft::Xna::Framework::Graphics::RasterizerState;
using Microsoft::Xna::Framework::Graphics::SamplerState;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteSortMode;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Buttons;
using Microsoft::Xna::Framework::Input::GamePad;
using Microsoft::Xna::Framework::Input::Keyboard;
using Microsoft::Xna::Framework::Input::Keys;
using Microsoft::Xna::Framework::Input::Touch::TouchPanel;

// This sample demonstrates using the touchscreen as a virtual thumbstick
// control. Each half of the screen behaves as a virtual thumbstick: the
// player's first point of contact defines the center, then they drag away
// from there to move the stick. Left stick flies the ship, right stick aims
// and fires at aliens closing in from off-screen. Port of the XNA 4.0
// "TouchThumbSticks" sample.
class TouchThumbsticksGame : public Game {
public:
    TouchThumbsticksGame() : graphics_(this) {
        graphics_.setPreferredBackBufferWidthProperty(GraphicsWidth);
        graphics_.setPreferredBackBufferHeightProperty(GraphicsHeight);
        getContentProperty().setRootDirectoryProperty("Content");
        setTargetElapsedTimeProperty(System::TimeSpan::FromTicks(333333));
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "TouchThumbsticksGame";
        return name;
    }

protected:
    void Initialize() override {
        TouchPanel::setDisplayWidthProperty(GraphicsWidth);
        TouchPanel::setDisplayHeightProperty(GraphicsHeight);
        Game::Initialize();
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        auto& content = getContentProperty();
        Bullet::SharedTexture.emplace(content.Load<Texture2D>("Images/bullet"));
        thumbstick_ = content.Load<Texture2D>("Images/thumbstick");
        helpTexture_.emplace(content.Load<Texture2D>("help"));

        player_.emplace(content.Load<Texture2D>("Images/player1"));
        player_->WorldWidth = WorldWidth;
        player_->WorldHeight = WorldHeight;

        blank_ = Texture2D(getGraphicsDeviceProperty(), 1, 1);
        Color white = Color::White;
        blank_->SetData(&white, 1);

        for (int i = 0; i < NumStars; ++i) {
            stars_.emplace_back(
                static_cast<float>(rand_.NextDouble()) * (WorldWidth + GraphicsWidth) -
                    (WorldWidth / 2.0f + GraphicsWidthHalf),
                static_cast<float>(rand_.NextDouble()) * (WorldWidth + GraphicsHeight) -
                    (WorldWidth / 2.0f + GraphicsHeightHalf),
                static_cast<float>(rand_.Next(1, 3)));
        }
    }

    void Update(GameTime& gameTime) override {
        float elapsed = static_cast<float>(gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty());
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        if (GamePad::GetState(PlayerIndex::One).IsButtonDown(Buttons::Back) ||
            Keyboard::GetState().IsKeyDown(Keys::Escape)) {
            Exit();
        }

        VirtualThumbsticks::Update();

        spawnTimer_ = spawnTimer_ - gameTime.getElapsedGameTimeProperty();
        if (spawnTimer_ <= System::TimeSpan::Zero) {
            int numToSpawn = rand_.Next(1, 3);
            for (int i = 0; i < numToSpawn; ++i) {
                EnemyShip enemy(getContentProperty().Load<Texture2D>("Images/alien"));
                enemy.Player = &*player_;

                enemy.Position.X = (rand_.Next() % 2 == 0) ? -WorldWidth / 2.0f - (GraphicsWidthHalf + 10)
                                                            : WorldWidth / 2.0f + (GraphicsWidthHalf + 10);
                enemy.Position.Y = (rand_.Next() % 2 == 0) ? -WorldHeight / 2.0f - (GraphicsHeightHalf + 10)
                                                            : WorldHeight / 2.0f + (GraphicsHeightHalf + 10);

                enemies_.push_back(std::move(enemy));
            }
            spawnTimer_ = SpawnRate;
        }

        player_->Update(gameTime);
        for (auto& enemy : enemies_) enemy.Update(gameTime);

        // Bullets only collide with one enemy each; mark both for removal.
        std::vector<std::size_t> enemiesToRemove;
        for (auto bulletIt = player_->Bullets.begin(); bulletIt != player_->Bullets.end();) {
            bool hit = false;
            for (std::size_t ei = 0; ei < enemies_.size(); ++ei) {
                if (enemies_[ei].ContainsPoint(bulletIt->Position)) {
                    enemiesToRemove.push_back(ei);
                    hit = true;
                    break;
                }
            }
            bulletIt = hit ? player_->Bullets.erase(bulletIt) : std::next(bulletIt);
        }

        std::sort(enemiesToRemove.begin(), enemiesToRemove.end());
        enemiesToRemove.erase(std::unique(enemiesToRemove.begin(), enemiesToRemove.end()), enemiesToRemove.end());
        for (auto it = enemiesToRemove.rbegin(); it != enemiesToRemove.rend(); ++it) {
            enemies_.erase(enemies_.begin() + static_cast<std::ptrdiff_t>(*it));
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color::Black);

        Matrix cameraTransform = Matrix::CreateTranslation(
            -player_->Position.X + GraphicsWidthHalf, -player_->Position.Y + GraphicsHeightHalf, 0.0f);

        spriteBatch_->Begin(SpriteSortMode::Deferred, BlendState::AlphaBlend,
                             const_cast<SamplerState*>(&SamplerState::LinearClamp),
                             const_cast<DepthStencilState*>(&DepthStencilState::Default),
                             const_cast<RasterizerState*>(&RasterizerState::CullNone), nullptr, cameraTransform);

        for (const auto& star : stars_) {
            spriteBatch_->Draw(*blank_,
                                Rectangle(static_cast<int>(star.X), static_cast<int>(star.Y),
                                          static_cast<int>(star.Z), static_cast<int>(star.Z)),
                                Color::White);
        }

        DrawWorldBorder();

        for (const auto& enemy : enemies_) enemy.Draw(*spriteBatch_);
        player_->Draw(*spriteBatch_);

        spriteBatch_->End();

        spriteBatch_->Begin();

        if (VirtualThumbsticks::getLeftThumbstickCenter().has_value()) {
            spriteBatch_->Draw(thumbstick_,
                                *VirtualThumbsticks::getLeftThumbstickCenter() -
                                    Vector2(static_cast<float>(thumbstick_.getWidthProperty()) / 2.0f,
                                            static_cast<float>(thumbstick_.getHeightProperty()) / 2.0f),
                                Color::Green);
        }

        if (VirtualThumbsticks::getRightThumbstickCenter().has_value()) {
            spriteBatch_->Draw(thumbstick_,
                                *VirtualThumbsticks::getRightThumbstickCenter() -
                                    Vector2(static_cast<float>(thumbstick_.getWidthProperty()) / 2.0f,
                                            static_cast<float>(thumbstick_.getHeightProperty()) / 2.0f),
                                Color::Blue);
        }

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = static_cast<float>((vp.getWidthProperty() - hw) / 2);
            float sy = static_cast<float>((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }

        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    static constexpr int GraphicsWidth = 800;
    static constexpr int GraphicsHeight = 480;
    static constexpr int GraphicsWidthHalf = GraphicsWidth / 2;
    static constexpr int GraphicsHeightHalf = GraphicsHeight / 2;
    static constexpr int WorldWidth = 1000;
    static constexpr int WorldHeight = 1000;
    static constexpr int NumStars = 1000;
    static constexpr int WorldBorderThickness = 4;
    static constexpr int WorldBorderThicknessDouble = WorldBorderThickness * 2;

    inline static const System::TimeSpan SpawnRate = System::TimeSpan::FromSeconds(2.0);

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;
    std::optional<Texture2D> blank_;
    Texture2D thumbstick_;

    System::Random rand_;
    System::TimeSpan spawnTimer_;
    std::vector<Vector3> stars_;
    std::optional<PlayerShip> player_;
    std::vector<EnemyShip> enemies_;

    Color worldBorderColor_ = Color::Red;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;

    void DrawWorldBorder() {
        Rectangle r(-WorldWidth / 2 - WorldBorderThickness, -WorldHeight / 2 - WorldBorderThickness,
                    WorldBorderThickness, WorldHeight + WorldBorderThicknessDouble);
        spriteBatch_->Draw(*blank_, r, worldBorderColor_);

        r = Rectangle(-WorldWidth / 2 - WorldBorderThickness, -WorldHeight / 2 - WorldBorderThickness,
                      WorldWidth + WorldBorderThicknessDouble, WorldBorderThickness);
        spriteBatch_->Draw(*blank_, r, worldBorderColor_);

        r = Rectangle(WorldWidth / 2, -WorldHeight / 2 - WorldBorderThickness, WorldBorderThickness,
                      WorldHeight + WorldBorderThicknessDouble);
        spriteBatch_->Draw(*blank_, r, worldBorderColor_);

        r = Rectangle(-WorldWidth / 2 - WorldBorderThickness, WorldHeight / 2,
                      WorldWidth + WorldBorderThicknessDouble, WorldBorderThickness);
        spriteBatch_->Draw(*blank_, r, worldBorderColor_);
    }
};

} // namespace TouchThumbsticks

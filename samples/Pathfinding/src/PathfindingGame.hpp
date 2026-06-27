#pragma once
#include <memory>
#include <optional>
#include <string>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Map.hpp"
#include "PathFinder.hpp"
#include "Tank.hpp"

namespace Pathfinding {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

class PathfindingGame : public Microsoft::Xna::Framework::Game {
    GraphicsDeviceManager graphics_;

    std::unique_ptr<SpriteBatch> spriteBatch_;

    // 1x1 white pixel texture for UI bars
    std::optional<Texture2D> onePixelWhite_;

    // Button icon textures
    std::optional<Texture2D> buttonA_, buttonB_, buttonX_, buttonY_;

    static constexpr int bottomUIHeight  = 80;
    Rectangle gameplayArea_;

    Map        map_;
    Tank       tank_;
    PathFinder pathFinder_;

    KeyboardState prevKeyboard_;
    KeyboardState curKeyboard_;
    GamePadState  prevGamePad_;
    GamePadState  curGamePad_;

    // --- F1 help overlay ---
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "PathfindingGame";
        return name;
    }

    PathfindingGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");
    }

protected:
    void Initialize() override {
        Game::Initialize();

        Viewport vp = getGraphicsDeviceProperty().getViewportProperty();
        gameplayArea_ = vp.getTitleSafeAreaProperty();
        gameplayArea_.Height -= bottomUIHeight;

        map_.UpdateMapViewport(gameplayArea_);
        tank_.Initialize(map_);
        pathFinder_.Initialize(map_);
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        // 1×1 white pixel
        onePixelWhite_.emplace(getGraphicsDeviceProperty(), 1, 1);
        Color white(255, 255, 255, 255);
        onePixelWhite_->SetData(&white, 1);

        buttonA_.emplace(getContentProperty().Load<Texture2D>("xboxControllerButtonA"));
        buttonB_.emplace(getContentProperty().Load<Texture2D>("xboxControllerButtonB"));
        buttonX_.emplace(getContentProperty().Load<Texture2D>("xboxControllerButtonX"));
        buttonY_.emplace(getContentProperty().Load<Texture2D>("xboxControllerButtonY"));

        map_.LoadContent(getContentProperty());
        tank_.LoadContent(getContentProperty());
        pathFinder_.LoadContent(getContentProperty());
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        HandleInput();

        if (map_.MapReload()) {
            map_.ReloadMap();
            map_.UpdateMapViewport(gameplayArea_);
            tank_.Reset();
            pathFinder_.Reset();
        }

        if (pathFinder_.GetSearchStatus() == SearchStatus::PathFound && !tank_.Moving()) {
            for (const Point& pt : pathFinder_.FinalPath())
                tank_.Waypoints().Enqueue(map_.MapToWorld(pt, true));
            tank_.SetMoving(true);
        }

        pathFinder_.Update(gameTime);
        tank_.Update(gameTime);

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(0, 0, 0, 255));

        // Single Begin/End for the entire frame (CNA Vulkan bug: multiple pairs discard earlier draws)
        spriteBatch_->Begin();
        map_.Draw(*spriteBatch_);
        pathFinder_.Draw(*spriteBatch_);
        tank_.Draw(*spriteBatch_);
        DrawHUD();
        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty()  - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }
        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    void HandleInput() {
        prevKeyboard_ = curKeyboard_;
        prevGamePad_  = curGamePad_;
        curGamePad_   = GamePad::GetState(PlayerIndex::One);
        curKeyboard_  = Keyboard::GetState();

        if (curGamePad_.IsButtonDown(Buttons::Back) || curKeyboard_.IsKeyDown(Keys::Escape))
            Exit();

        // A / Space — Start/Stop
        if (KeyJustPressed(Keys::A) || ButtonJustPressed(Buttons::A))
            pathFinder_.SetIsSearching(!pathFinder_.IsSearching());

        // B — Reset map
        if (KeyJustPressed(Keys::B) || ButtonJustPressed(Buttons::B))
            map_.SetMapReload(true);

        // X — Next search type
        if (KeyJustPressed(Keys::X) || ButtonJustPressed(Buttons::X))
            pathFinder_.NextSearchType();

        // Y — Next map
        if (KeyJustPressed(Keys::Y) || ButtonJustPressed(Buttons::Y))
            map_.CycleMap();

        // Right/Left — adjust time step
        if (KeyJustPressed(Keys::Right) || DPadJustPressed(Buttons::DPadRight))
            pathFinder_.SetTimeStep(MathHelper::Clamp(pathFinder_.TimeStep() + 0.1f, 0.0f, 1.0f));
        if (KeyJustPressed(Keys::Left)  || DPadJustPressed(Buttons::DPadLeft))
            pathFinder_.SetTimeStep(MathHelper::Clamp(pathFinder_.TimeStep() - 0.1f, 0.0f, 1.0f));
    }

    // Draw button icons at the bottom. Text labels are omitted (SpriteFont not available).
    void DrawHUD() {
        float y = (float)(gameplayArea_.getBottomProperty() + 8);
        float bw = (float)buttonA_->getWidthProperty();

        spriteBatch_->Draw(*buttonA_, Vector2(10.0f,   y), Color(255,255,255,255));
        spriteBatch_->Draw(*buttonB_, Vector2(150.0f,  y), Color(255,255,255,255));
        spriteBatch_->Draw(*buttonY_, Vector2(250.0f,  y), Color(255,255,255,255));
        spriteBatch_->Draw(*buttonX_, Vector2(400.0f,  y), Color(255,255,255,255));

        // Time step slider bar (visual only, no text label)
        DrawBar(125, (int)y + 40, 200, 20, pathFinder_.TimeStep());
    }

    void DrawBar(int x, int y, int w, int h, float normalized) {
        // Use Vector2 scale to stretch 1x1 white texture (Rectangle Draw not reliable in CNA)
        spriteBatch_->Draw(*onePixelWhite_,
            Vector2((float)x, (float)y),
            std::nullopt, Color(255, 255, 255, 255),
            0.0f, Vector2::Zero, Vector2((float)w, (float)(h / 2)),
            SpriteEffects::None, 0.0f);
        int knobX = x + (int)(w * normalized);
        spriteBatch_->Draw(*onePixelWhite_,
            Vector2((float)knobX, (float)(y - h / 4)),
            std::nullopt, Color(255, 165, 0, 255),
            0.0f, Vector2::Zero, Vector2(20.0f, (float)h),
            SpriteEffects::None, 0.0f);
    }

    bool KeyJustPressed(Keys key) const {
        return curKeyboard_.IsKeyDown(key) && !prevKeyboard_.IsKeyDown(key);
    }
    bool ButtonJustPressed(Buttons btn) const {
        return curGamePad_.IsButtonDown(btn) && !prevGamePad_.IsButtonDown(btn);
    }
    bool DPadJustPressed(Buttons btn) const {
        return curGamePad_.IsButtonDown(btn) && !prevGamePad_.IsButtonDown(btn);
    }
};

} // namespace Pathfinding

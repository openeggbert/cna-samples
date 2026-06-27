#pragma once
#include <memory>
#include <optional>
#include <string>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "PrimitiveBatch.hpp"
#include "Tank.hpp"

namespace PathDrawing {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

class PathDrawingGame : public Microsoft::Xna::Framework::Game {
    GraphicsDeviceManager graphics_;

    std::unique_ptr<SpriteBatch>    spriteBatch_;
    std::unique_ptr<PrimitiveBatch> primitiveBatch_;

    std::optional<Texture2D> groundTexture_;
    static constexpr int groundSize = 300;

    std::unique_ptr<Tank> tank_;

    bool drawingWaypoints_ = false;

    MouseState prevMouse_;
    MouseState currentMouse_;

    // --- F1 help overlay ---
    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "PathDrawingGame";
        return name;
    }

    PathDrawingGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");
    }

protected:
    void LoadContent() override {
        spriteBatch_    = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        primitiveBatch_ = std::make_unique<PrimitiveBatch>(getGraphicsDeviceProperty());

        groundTexture_.emplace(getContentProperty().Load<Texture2D>("ground"));

        tank_ = std::make_unique<Tank>(getGraphicsDeviceProperty(), getContentProperty());
        tank_->Reset(Vector2(100.0f, 100.0f));
        tank_->SetMoveSpeed(225.0f);
        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        KeyboardState keyboard = Keyboard::GetState();
        GamePadState  gamePad  = GamePad::GetState(PlayerIndex::One);

        if (gamePad.IsButtonDown(Buttons::Back) || keyboard.IsKeyDown(Keys::Escape))
            Exit();

        tank_->Update(gameTime);

        prevMouse_    = currentMouse_;
        currentMouse_ = Mouse::GetState();

        Vector2 mousePos((float)currentMouse_.getXProperty(), (float)currentMouse_.getYProperty());

        bool mouseJustPressed  = currentMouse_.getLeftButtonProperty() == ButtonState::Pressed
                              && prevMouse_.getLeftButtonProperty()    == ButtonState::Released;
        bool mouseJustReleased = currentMouse_.getLeftButtonProperty() == ButtonState::Released
                              && prevMouse_.getLeftButtonProperty()    == ButtonState::Pressed;
        bool mouseHeld         = currentMouse_.getLeftButtonProperty() == ButtonState::Pressed;

        if (mouseJustPressed && tank_->HitTest(mousePos)) {
            tank_->Waypoints().Clear();
            drawingWaypoints_ = true;
            tank_->Waypoints().Enqueue(mousePos);
        } else if (mouseJustReleased) {
            drawingWaypoints_ = false;
        }

        // Equivalent to FreeDrag gesture: mouse held and moved since last frame
        if (drawingWaypoints_ && mouseHeld) {
            Vector2 prevPos((float)prevMouse_.getXProperty(), (float)prevMouse_.getYProperty());
            if (mousePos.X != prevPos.X || mousePos.Y != prevPos.Y)
                tank_->Waypoints().Enqueue(mousePos);
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255));

        // All sprite draws in one Begin/End to avoid state issues.
        spriteBatch_->Begin();
        DrawGround();
        tank_->Draw(*spriteBatch_);
        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty()  - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);
            spriteBatch_->Draw(*helpTexture_, Vector2(sx, sy), Color(255, 255, 255, 255));
        }
        spriteBatch_->End();

        DrawPath();

        Game::Draw(gameTime);
    }

private:
    void DrawGround() {
        Viewport vp = getGraphicsDeviceProperty().getViewportProperty();
        int vw = vp.getWidthProperty();
        int vh = vp.getHeightProperty();

        for (int ty = 0; ty * groundSize < vh; ty++) {
            for (int tx = 0; tx * groundSize < vw; tx++) {
                spriteBatch_->Draw(*groundTexture_,
                    Vector2((float)(tx * groundSize), (float)(ty * groundSize)),
                    Color(255, 255, 255, 255));
            }
        }
    }

    void DrawPath() {
        if (tank_->Waypoints().Count() < 1) return;

        primitiveBatch_->Begin(PrimitiveType::LineList);

        primitiveBatch_->AddVertex(tank_->Location(), Color(255, 255, 255, 255));

        for (int i = 1; i < tank_->Waypoints().Count(); i++) {
            primitiveBatch_->AddVertex(tank_->Waypoints()[i], Color(255, 255, 255, 255));
            if (i < tank_->Waypoints().Count() - 1)
                primitiveBatch_->AddVertex(tank_->Waypoints()[i], Color(255, 255, 255, 255));
        }

        primitiveBatch_->End();
    }
};

} // namespace PathDrawing

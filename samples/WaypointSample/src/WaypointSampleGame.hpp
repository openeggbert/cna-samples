#pragma once
#include <memory>
#include <optional>
#include <string>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Tank.hpp"

namespace WaypointSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

class WaypointSampleGame : public Microsoft::Xna::Framework::Game {
    static constexpr int   screenWidth     = 853;
    static constexpr int   screenHeight    = 480;
    static constexpr float cursorMoveSpeed = 250.0f;

    GraphicsDeviceManager graphics_;
    std::unique_ptr<SpriteBatch> spriteBatch_;

    std::optional<Texture2D> cursorTexture_;
    Vector2 cursorCenter_;
    Vector2 cursorLocation_;

    Tank tank_;

    KeyboardState prevKeyboard_;
    KeyboardState curKeyboard_;
    GamePadState  prevGamePad_;
    GamePadState  curGamePad_;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "WaypointSampleGame";
        return name;
    }

    WaypointSampleGame() : graphics_(this) {
        graphics_.setPreferredBackBufferWidthProperty(screenWidth);
        graphics_.setPreferredBackBufferHeightProperty(screenHeight);
        getContentProperty().setRootDirectoryProperty("Content");
    }

protected:
    void Initialize() override {
        cursorLocation_ = Vector2((float)screenWidth / 2.0f, (float)screenHeight / 2.0f);
        tank_.Reset(Vector2((float)screenWidth / 4.0f, (float)screenHeight / 4.0f));
        Game::Initialize();
    }

    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        cursorTexture_.emplace(getContentProperty().Load<Texture2D>("cursor"));
        cursorCenter_ = Vector2(
            (float)(cursorTexture_->getWidthProperty()  / 2),
            (float)(cursorTexture_->getHeightProperty() / 2));
        tank_.LoadContent(getContentProperty());
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        HandleInput(elapsed);
        tank_.Update(gameTime);
        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255));

        spriteBatch_->Begin();
        tank_.Draw(*spriteBatch_);
        spriteBatch_->Draw(*cursorTexture_, cursorLocation_, std::nullopt,
            Color(255, 255, 255, 255), 0.0f, cursorCenter_,
            1.0f, SpriteEffects::None, 0.0f);
        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    void HandleInput(float elapsed) {
        prevKeyboard_ = curKeyboard_;
        prevGamePad_  = curGamePad_;
        curGamePad_   = GamePad::GetState(PlayerIndex::One);
        curKeyboard_  = Keyboard::GetState();

        if (curGamePad_.IsButtonDown(Buttons::Back) || curKeyboard_.IsKeyDown(Keys::Escape))
            Exit();

        // Cursor movement
        cursorLocation_.X += curGamePad_.getThumbSticksProperty().getLeftProperty().X
                             * cursorMoveSpeed * elapsed;
        cursorLocation_.Y -= curGamePad_.getThumbSticksProperty().getLeftProperty().Y
                             * cursorMoveSpeed * elapsed;

        if (curKeyboard_.IsKeyDown(Keys::Up))    cursorLocation_.Y -= elapsed * cursorMoveSpeed;
        if (curKeyboard_.IsKeyDown(Keys::Down))  cursorLocation_.Y += elapsed * cursorMoveSpeed;
        if (curKeyboard_.IsKeyDown(Keys::Left))  cursorLocation_.X -= elapsed * cursorMoveSpeed;
        if (curKeyboard_.IsKeyDown(Keys::Right)) cursorLocation_.X += elapsed * cursorMoveSpeed;

        cursorLocation_.X = MathHelper::Clamp(cursorLocation_.X, 0.0f, (float)screenWidth);
        cursorLocation_.Y = MathHelper::Clamp(cursorLocation_.Y, 0.0f, (float)screenHeight);

        // B — cycle behavior
        if (KeyJustPressed(Keys::B) || ButtonJustPressed(Buttons::B))
            tank_.CycleBehaviorType();

        // A — place waypoint
        if (KeyJustPressed(Keys::A) || ButtonJustPressed(Buttons::A))
            tank_.Waypoints().Enqueue(cursorLocation_);

        // X — reset
        if (KeyJustPressed(Keys::X) || ButtonJustPressed(Buttons::X))
            tank_.Reset(Vector2((float)screenWidth / 4.0f, (float)screenHeight / 4.0f));
    }

    bool KeyJustPressed(Keys key) const {
        return curKeyboard_.IsKeyDown(key) && !prevKeyboard_.IsKeyDown(key);
    }
    bool ButtonJustPressed(Buttons btn) const {
        return curGamePad_.IsButtonDown(btn) && !prevGamePad_.IsButtonDown(btn);
    }
};

} // namespace WaypointSample

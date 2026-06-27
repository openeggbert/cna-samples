#pragma once
#include <vector>
#include <string>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "System/Random.hpp"

namespace RectangleCollision {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

class RectangleCollisionGame : public Microsoft::Xna::Framework::Game {
    GraphicsDeviceManager graphics_;
    SpriteBatch* spriteBatch_ = nullptr;

    Texture2D personTexture_;
    Texture2D blockTexture_;

    Vector2 personPosition_;
    static constexpr int PersonMoveSpeed = 5;

    std::vector<Vector2> blockPositions_;
    float BlockSpawnProbability = 0.01f;
    static constexpr int BlockFallSpeed = 2;

    System::Random random_;

    bool personHit_ = false;

    Rectangle safeBounds_;
    static constexpr float SafeAreaPortion = 0.05f;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "RectangleCollisionGame";
        return name;
    }

    RectangleCollisionGame() : graphics_(this) {
        getContentProperty().setRootDirectoryProperty("Content");
    }

protected:
    void Initialize() override {
        Game::Initialize();

        Viewport viewport = getGraphicsDeviceProperty().getViewportProperty();
        safeBounds_ = Rectangle(
            (int)(viewport.getWidthProperty()  * SafeAreaPortion),
            (int)(viewport.getHeightProperty() * SafeAreaPortion),
            (int)(viewport.getWidthProperty()  * (1 - 2 * SafeAreaPortion)),
            (int)(viewport.getHeightProperty() * (1 - 2 * SafeAreaPortion)));

        personPosition_.X = (float)((safeBounds_.Width  - personTexture_.getWidthProperty())  / 2);
        personPosition_.Y = (float) (safeBounds_.Height - personTexture_.getHeightProperty());
    }

    void LoadContent() override {
        blockTexture_  = getContentProperty().Load<Texture2D>("Block");
        personTexture_ = getContentProperty().Load<Texture2D>("Person");

        spriteBatch_ = new SpriteBatch(getGraphicsDeviceProperty());
    }

    void Update(GameTime& gameTime) override {
        KeyboardState keyboard = Keyboard::GetState();
        GamePadState  gamePad  = GamePad::GetState(PlayerIndex::One);

        if (gamePad.IsButtonDown(Buttons::Back) || keyboard.IsKeyDown(Keys::Escape))
            Exit();

        if (keyboard.IsKeyDown(Keys::Left) ||
            gamePad.getDPadProperty().getLeftProperty() == ButtonState::Pressed)
            personPosition_.X -= PersonMoveSpeed;

        if (keyboard.IsKeyDown(Keys::Right) ||
            gamePad.getDPadProperty().getRightProperty() == ButtonState::Pressed)
            personPosition_.X += PersonMoveSpeed;

        personPosition_.X = MathHelper::Clamp(
            personPosition_.X,
            (float)safeBounds_.getLeftProperty(),
            (float)(safeBounds_.getRightProperty() - personTexture_.getWidthProperty()));

        if (random_.NextDouble() < BlockSpawnProbability) {
            float x = (float)random_.NextDouble() *
                (float)(getWindowProperty().getClientBoundsProperty().Width
                        - blockTexture_.getWidthProperty());
            blockPositions_.push_back(Vector2(x, -(float)blockTexture_.getHeightProperty()));
        }

        Rectangle personRect(
            (int)personPosition_.X, (int)personPosition_.Y,
            personTexture_.getWidthProperty(), personTexture_.getHeightProperty());

        personHit_ = false;
        for (int i = 0; i < (int)blockPositions_.size(); i++) {
            blockPositions_[i] = Vector2(blockPositions_[i].X,
                                         blockPositions_[i].Y + BlockFallSpeed);

            Rectangle blockRect(
                (int)blockPositions_[i].X, (int)blockPositions_[i].Y,
                blockTexture_.getWidthProperty(), blockTexture_.getHeightProperty());

            if (personRect.Intersects(blockRect))
                personHit_ = true;

            if (blockPositions_[i].Y > getWindowProperty().getClientBoundsProperty().Height) {
                blockPositions_.erase(blockPositions_.begin() + i);
                i--;
            }
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        if (personHit_)
            getGraphicsDeviceProperty().Clear(Color(255, 0, 0, 255));
        else
            getGraphicsDeviceProperty().Clear(Color(100, 149, 237, 255));

        spriteBatch_->Begin();
        spriteBatch_->Draw(personTexture_, personPosition_, Color(255, 255, 255, 255));
        for (const Vector2& pos : blockPositions_)
            spriteBatch_->Draw(blockTexture_, pos, Color(255, 255, 255, 255));
        spriteBatch_->End();

        Game::Draw(gameTime);
    }
};

} // namespace RectangleCollision

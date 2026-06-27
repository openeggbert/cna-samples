#pragma once
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Input/GamePad.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "System/Random.hpp"

namespace TransformedCollision {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

struct Block {
    Vector2 Position;
    float Rotation = 0.0f;
};

class TransformedCollisionGame : public Microsoft::Xna::Framework::Game {
    GraphicsDeviceManager graphics_;

    std::optional<Texture2D> personTexture_;
    std::optional<Texture2D> blockTexture_;

    std::vector<Color> personTextureData_;
    std::vector<Color> blockTextureData_;

    std::unique_ptr<SpriteBatch> spriteBatch_;

    Vector2 personPosition_;
    static constexpr int PersonMoveSpeed = 5;

    std::vector<Block> blocks_;
    float blockSpawnProbability_ = 0.01f;
    static constexpr int BlockFallSpeed = 1;
    static constexpr float BlockRotateSpeed = 0.005f;
    Vector2 blockOrigin_;

    System::Random random_;
    bool personHit_ = false;

    Rectangle safeBounds_;
    static constexpr float SafeAreaPortion = 0.05f;

public:
    const std::string& GetTypeName() const override {
        static const std::string name = "TransformedCollisionGame";
        return name;
    }

    TransformedCollisionGame() : graphics_(this) {
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

        personPosition_.X = (float)((safeBounds_.Width - personTexture_->getWidthProperty()) / 2);
        personPosition_.Y = (float)(safeBounds_.Height - personTexture_->getHeightProperty());
    }

    void LoadContent() override {
        blockTexture_.emplace(getContentProperty().Load<Texture2D>("SpinnerBlock"));
        personTexture_.emplace(getContentProperty().Load<Texture2D>("Person"));

        int blockPixels  = blockTexture_->getWidthProperty()  * blockTexture_->getHeightProperty();
        int personPixels = personTexture_->getWidthProperty() * personTexture_->getHeightProperty();
        blockTextureData_.resize(blockPixels,   Color(0, 0, 0, 0));
        personTextureData_.resize(personPixels, Color(0, 0, 0, 0));
        blockTexture_->GetData(blockTextureData_.data(),   blockPixels);
        personTexture_->GetData(personTextureData_.data(), personPixels);

        blockOrigin_ = Vector2(
            (float)(blockTexture_->getWidthProperty()  / 2),
            (float)(blockTexture_->getHeightProperty() / 2));

        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
    }

    void Update(GameTime& gameTime) override {
        KeyboardState keyboard = Keyboard::GetState();
        GamePadState  gamePad  = GamePad::GetState(PlayerIndex::One);

        if (gamePad.IsButtonDown(Buttons::Back) || keyboard.IsKeyDown(Keys::Escape))
            Exit();

        if (keyboard.IsKeyDown(Keys::Left)  || gamePad.getDPadProperty().getLeftProperty()  == ButtonState::Pressed)
            personPosition_.X -= PersonMoveSpeed;
        if (keyboard.IsKeyDown(Keys::Right) || gamePad.getDPadProperty().getRightProperty() == ButtonState::Pressed)
            personPosition_.X += PersonMoveSpeed;

        personPosition_.X = MathHelper::Clamp(
            personPosition_.X,
            (float)safeBounds_.getLeftProperty(),
            (float)(safeBounds_.getRightProperty() - personTexture_->getWidthProperty()));

        Matrix personTransform = Matrix::CreateTranslation(
            Vector3(personPosition_.X, personPosition_.Y, 0.0f));

        // Spawn new falling blocks
        if (random_.NextDouble() < blockSpawnProbability_) {
            Block newBlock;
            float x = (float)random_.NextDouble() *
                (getWindowProperty().getClientBoundsProperty().Width - blockTexture_->getWidthProperty());
            newBlock.Position = Vector2(x, (float)(-blockTexture_->getHeightProperty()));
            newBlock.Rotation = (float)random_.NextDouble() * MathHelper::TwoPi;
            blocks_.push_back(newBlock);
        }

        Rectangle personRectangle(
            (int)personPosition_.X, (int)personPosition_.Y,
            personTexture_->getWidthProperty(), personTexture_->getHeightProperty());

        personHit_ = false;
        for (int i = 0; i < (int)blocks_.size(); i++) {
            blocks_[i].Position.Y += BlockFallSpeed;
            blocks_[i].Rotation   += BlockRotateSpeed;

            Matrix blockTransform =
                Matrix::CreateTranslation(Vector3(-blockOrigin_.X, -blockOrigin_.Y, 0.0f)) *
                Matrix::CreateRotationZ(blocks_[i].Rotation) *
                Matrix::CreateTranslation(Vector3(blocks_[i].Position.X, blocks_[i].Position.Y, 0.0f));

            Rectangle blockRectangle = CalculateBoundingRectangle(
                Rectangle(0, 0, blockTexture_->getWidthProperty(), blockTexture_->getHeightProperty()),
                blockTransform);

            if (personRectangle.Intersects(blockRectangle)) {
                if (IntersectPixels(personTransform,
                                    personTexture_->getWidthProperty(),
                                    personTexture_->getHeightProperty(),
                                    personTextureData_,
                                    blockTransform,
                                    blockTexture_->getWidthProperty(),
                                    blockTexture_->getHeightProperty(),
                                    blockTextureData_))
                    personHit_ = true;
            }

            if (blocks_[i].Position.Y >
                    getWindowProperty().getClientBoundsProperty().Height + blockOrigin_.Length()) {
                blocks_.erase(blocks_.begin() + i);
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

        spriteBatch_->Draw(*personTexture_, personPosition_, Color(255, 255, 255, 255));

        for (const Block& block : blocks_) {
            spriteBatch_->Draw(*blockTexture_, block.Position,
                               std::nullopt, Color(255, 255, 255, 255),
                               block.Rotation, blockOrigin_,
                               1.0f, SpriteEffects::None, 0.0f);
        }

        spriteBatch_->End();

        Game::Draw(gameTime);
    }

private:
    static bool IntersectPixels(
        const Matrix& transformA, int widthA, int heightA, const std::vector<Color>& dataA,
        const Matrix& transformB, int widthB, int heightB, const std::vector<Color>& dataB)
    {
        Matrix transformAToB = transformA * Matrix::Invert(transformB);

        Vector2 stepX  = Vector2::TransformNormal(Vector2::UnitX, transformAToB);
        Vector2 stepY  = Vector2::TransformNormal(Vector2::UnitY, transformAToB);
        Vector2 yPosInB = Vector2::Transform(Vector2::Zero, transformAToB);

        for (int yA = 0; yA < heightA; yA++) {
            Vector2 posInB = yPosInB;
            for (int xA = 0; xA < widthA; xA++) {
                int xB = (int)std::round(posInB.X);
                int yB = (int)std::round(posInB.Y);
                if (0 <= xB && xB < widthB && 0 <= yB && yB < heightB) {
                    Color colorA = dataA[xA + yA * widthA];
                    Color colorB = dataB[xB + yB * widthB];
                    if (colorA.getAProperty() != 0 && colorB.getAProperty() != 0)
                        return true;
                }
                posInB.X += stepX.X;
                posInB.Y += stepX.Y;
            }
            yPosInB.X += stepY.X;
            yPosInB.Y += stepY.Y;
        }
        return false;
    }

    static Rectangle CalculateBoundingRectangle(const Rectangle& rectangle, const Matrix& transform) {
        Vector2 leftTop(    (float)rectangle.getLeftProperty(),  (float)rectangle.getTopProperty());
        Vector2 rightTop(   (float)rectangle.getRightProperty(), (float)rectangle.getTopProperty());
        Vector2 leftBottom( (float)rectangle.getLeftProperty(),  (float)rectangle.getBottomProperty());
        Vector2 rightBottom((float)rectangle.getRightProperty(), (float)rectangle.getBottomProperty());

        leftTop     = Vector2::Transform(leftTop,     transform);
        rightTop    = Vector2::Transform(rightTop,    transform);
        leftBottom  = Vector2::Transform(leftBottom,  transform);
        rightBottom = Vector2::Transform(rightBottom, transform);

        Vector2 min = Vector2::Min(Vector2::Min(leftTop, rightTop),
                                   Vector2::Min(leftBottom, rightBottom));
        Vector2 max = Vector2::Max(Vector2::Max(leftTop, rightTop),
                                   Vector2::Max(leftBottom, rightBottom));

        return Rectangle((int)min.X, (int)min.Y,
                         (int)(max.X - min.X), (int)(max.Y - min.Y));
    }
};

} // namespace TransformedCollision

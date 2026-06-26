#pragma once
#include <array>
#include <memory>
#include <string>
#include <vector>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/PlayerIndex.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Buttons.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "System/TimeSpan.hpp"
#include "Direction.hpp"
#include "InputManager.hpp"
#include "Move.hpp"
#include "MoveList.hpp"

namespace InputSequenceSample {

class InputSequenceGame : public Microsoft::Xna::Framework::Game {
    Microsoft::Xna::Framework::GraphicsDeviceManager graphics_;
    std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> spriteBatch_;

    std::vector<Move>     moves_;
    MoveList              moveList_;
    std::array<InputManager, 2> inputManagers_;
    std::array<Move*, 2>  playerMoves_    = { nullptr, nullptr };
    std::array<System::TimeSpan, 2> playerMoveTimes_;

    const System::TimeSpan MoveTimeOut = System::TimeSpan::FromSeconds(1.0);

    // Direction textures.
    Microsoft::Xna::Framework::Graphics::Texture2D upTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D downTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D leftTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D rightTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D upLeftTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D upRightTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D downLeftTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D downRightTexture_;

    // Button textures.
    Microsoft::Xna::Framework::Graphics::Texture2D aButtonTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D bButtonTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D xButtonTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D yButtonTexture_;

    // Other textures.
    Microsoft::Xna::Framework::Graphics::Texture2D plusTexture_;
    Microsoft::Xna::Framework::Graphics::Texture2D padFaceTexture_;

    static std::vector<Move> BuildMoves() {
        using namespace Microsoft::Xna::Framework::Input;
        namespace D = Direction;
        std::vector<Move> m;
        m.emplace_back("Jump",        std::vector<Buttons>{Buttons::A},                                     true);
        m.emplace_back("Punch",       std::vector<Buttons>{Buttons::X},                                     true);
        m.emplace_back("Double Jump", std::vector<Buttons>{Buttons::A, Buttons::A});
        m.emplace_back("Jump Kick",   std::vector<Buttons>{Buttons::A | Buttons::X});
        m.emplace_back("Quad Punch",  std::vector<Buttons>{Buttons::X, Buttons::Y, Buttons::X, Buttons::Y});
        m.emplace_back("Fireball",    std::vector<Buttons>{D::Down, D::DownRight, D::Right | Buttons::X});
        m.emplace_back("Long Jump",   std::vector<Buttons>{D::Up, D::Up, Buttons::A});
        m.emplace_back("Back Flip",   std::vector<Buttons>{D::Down, D::Down | Buttons::A});
        m.emplace_back("30 Lives",    std::vector<Buttons>{D::Up, D::Up, D::Down, D::Down,
                                                           D::Left, D::Right, D::Left, D::Right,
                                                           Buttons::B, Buttons::A});
        return m;
    }

    static std::array<InputManager, 2> BuildManagers(int bufCapacity) {
        using Microsoft::Xna::Framework::PlayerIndex;
        return { InputManager(PlayerIndex::One, bufCapacity),
                 InputManager(PlayerIndex::Two, bufCapacity) };
    }

public:
    InputSequenceGame()
        : graphics_(this)
        , moves_(BuildMoves())
        , moveList_(moves_)
        , inputManagers_(BuildManagers(moveList_.LongestMoveLength()))
    {
        getContentProperty().setRootDirectoryProperty("Content");
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "InputSequenceGame";
        return name;
    }

protected:
    void LoadContent() override {
        using namespace Microsoft::Xna::Framework::Graphics;
        spriteBatch_     = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        upTexture_        = getContentProperty().Load<Texture2D>("Up");
        downTexture_      = getContentProperty().Load<Texture2D>("Down");
        leftTexture_      = getContentProperty().Load<Texture2D>("Left");
        rightTexture_     = getContentProperty().Load<Texture2D>("Right");
        upLeftTexture_    = getContentProperty().Load<Texture2D>("UpLeft");
        upRightTexture_   = getContentProperty().Load<Texture2D>("UpRight");
        downLeftTexture_  = getContentProperty().Load<Texture2D>("DownLeft");
        downRightTexture_ = getContentProperty().Load<Texture2D>("DownRight");
        aButtonTexture_   = getContentProperty().Load<Texture2D>("A");
        bButtonTexture_   = getContentProperty().Load<Texture2D>("B");
        xButtonTexture_   = getContentProperty().Load<Texture2D>("X");
        yButtonTexture_   = getContentProperty().Load<Texture2D>("Y");
        plusTexture_      = getContentProperty().Load<Texture2D>("Plus");
        padFaceTexture_   = getContentProperty().Load<Texture2D>("PadFace");
    }

    void Update(Microsoft::Xna::Framework::GameTime& gameTime) override {
        using namespace Microsoft::Xna::Framework::Input;
        for (int i = 0; i < 2; ++i) {
            if (gameTime.getTotalGameTimeProperty() - playerMoveTimes_[i] > MoveTimeOut)
                playerMoves_[i] = nullptr;

            inputManagers_[i].Update(gameTime);

            if (inputManagers_[i].GamePadState.IsButtonDown(Buttons::Back) ||
                inputManagers_[i].KeyboardState.IsKeyDown(Keys::Escape))
            {
                Exit();
            }

            Move* newMove = moveList_.DetectMove(inputManagers_[i]);
            if (newMove != nullptr) {
                playerMoves_[i]     = newMove;
                playerMoveTimes_[i] = gameTime.getTotalGameTimeProperty();
            }
        }
        Game::Update(gameTime);
    }

    void Draw(const Microsoft::Xna::Framework::GameTime& gameTime) override {
        using namespace Microsoft::Xna::Framework;
        getGraphicsDeviceProperty().Clear(Color::CornflowerBlue);

        spriteBatch_->Begin();

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        Vector2 topLeft(50.0f, 50.0f);
        Vector2 bottomRight((float)vp.getWidthProperty() - topLeft.X,
                             (float)vp.getHeightProperty() - topLeft.Y);

        Vector2 position = topLeft;

        // Draw each move's button sequence.
        for (auto& move : moves_) {
            Vector2 size = MeasureMove(move);
            if (position.X + size.X > bottomRight.X) {
                position.X = topLeft.X;
                position.Y = position.Y + size.Y;
            }
            DrawMove(move, position);
            position.X = position.X + size.X + 30.0f;
        }

        // Skip some space before player input rows.
        position.Y = position.Y + 90.0f;

        // Draw each player's input buffer.
        for (int i = 0; i < 2; ++i) {
            position.X = topLeft.X;
            DrawPlayerInput(i, position);
            position.Y = position.Y + 80.0f;
        }

        spriteBatch_->End();
        Game::Draw(gameTime);
    }

private:
    // Returns the texture for a given direction, or nullptr if none.
    const Microsoft::Xna::Framework::Graphics::Texture2D*
    GetDirectionTexture(Microsoft::Xna::Framework::Input::Buttons direction) const {
        if (direction == Direction::Up)        return &upTexture_;
        if (direction == Direction::Down)      return &downTexture_;
        if (direction == Direction::Left)      return &leftTexture_;
        if (direction == Direction::Right)     return &rightTexture_;
        if (direction == Direction::UpLeft)    return &upLeftTexture_;
        if (direction == Direction::UpRight)   return &upRightTexture_;
        if (direction == Direction::DownLeft)  return &downLeftTexture_;
        if (direction == Direction::DownRight) return &downRightTexture_;
        return nullptr;
    }

    Microsoft::Xna::Framework::Vector2
    MeasureButtons(Microsoft::Xna::Framework::Input::Buttons buttons) const {
        using namespace Microsoft::Xna::Framework::Input;
        Buttons direction = Direction::FromButtons(buttons);
        float width = 0.0f;
        if (direction != Direction::None) {
            width = (float)GetDirectionTexture(direction)->getWidthProperty();
            if ((buttons & ~direction) != Direction::None)
                width += (float)plusTexture_.getWidthProperty()
                       + (float)padFaceTexture_.getWidthProperty();
        } else {
            width = (float)padFaceTexture_.getWidthProperty();
        }
        return Microsoft::Xna::Framework::Vector2(width,
                   (float)padFaceTexture_.getHeightProperty());
    }

    Microsoft::Xna::Framework::Vector2
    MeasureSequence(const std::vector<Microsoft::Xna::Framework::Input::Buttons>& seq) const {
        float width = 0.0f;
        for (auto btn : seq)
            width = width + MeasureButtons(btn).X;
        return Microsoft::Xna::Framework::Vector2(width,
                   (float)padFaceTexture_.getHeightProperty());
    }

    Microsoft::Xna::Framework::Vector2 MeasureMove(const Move& move) const {
        // Text size omitted (no SpriteFont); width/height from icon sequence only.
        return MeasureSequence(move.Sequence);
    }

    void DrawButtons(Microsoft::Xna::Framework::Input::Buttons buttons,
                     Microsoft::Xna::Framework::Vector2 position) {
        using namespace Microsoft::Xna::Framework;
        using namespace Microsoft::Xna::Framework::Input;
        using namespace Microsoft::Xna::Framework::Graphics;

        Buttons direction = Direction::FromButtons(buttons);
        const Texture2D* dirTex = GetDirectionTexture(direction);

        if (dirTex != nullptr) {
            spriteBatch_->Draw(*dirTex, position, Color::White);
            position.X = position.X + (float)dirTex->getWidthProperty();
        }

        if ((buttons & ~direction) != Direction::None) {
            if (dirTex != nullptr) {
                spriteBatch_->Draw(plusTexture_, position, Color::White);
                position.X = position.X + (float)plusTexture_.getWidthProperty();
            }
            spriteBatch_->Draw(padFaceTexture_, position, Color::White);
            if ((buttons & Buttons::A) != Direction::None)
                spriteBatch_->Draw(aButtonTexture_, position, Color::White);
            if ((buttons & Buttons::B) != Direction::None)
                spriteBatch_->Draw(bButtonTexture_, position, Color::White);
            if ((buttons & Buttons::X) != Direction::None)
                spriteBatch_->Draw(xButtonTexture_, position, Color::White);
            if ((buttons & Buttons::Y) != Direction::None)
                spriteBatch_->Draw(yButtonTexture_, position, Color::White);
        }
    }

    void DrawSequence(const std::vector<Microsoft::Xna::Framework::Input::Buttons>& seq,
                      Microsoft::Xna::Framework::Vector2 position) {
        for (auto btn : seq) {
            DrawButtons(btn, position);
            position.X = position.X + MeasureButtons(btn).X;
        }
    }

    void DrawMove(const Move& move, Microsoft::Xna::Framework::Vector2 position) {
        // Move name (DrawString) omitted — no SpriteFont in CNA yet.
        DrawSequence(move.Sequence, position);
    }

    void DrawPlayerInput(int i, Microsoft::Xna::Framework::Vector2 position) {
        // "Player N input" text label omitted — no SpriteFont in CNA yet.
        // Draw the player's input buffer as button icons.
        DrawSequence(inputManagers_[i].Buffer, position);
    }
};

} // namespace InputSequenceSample

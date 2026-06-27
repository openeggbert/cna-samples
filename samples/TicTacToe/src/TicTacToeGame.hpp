#pragma once

#include <CNA/Entrypoint.hpp>
#include <Microsoft/Xna/Framework/Game.hpp>
#include <Microsoft/Xna/Framework/GraphicsDeviceManager.hpp>
#include <Microsoft/Xna/Framework/GameTime.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteFont.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>
#include <Microsoft/Xna/Framework/Input/Keys.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>
#include <System/Random.hpp>

#include <optional>
#include <memory>
#include <string>

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;
using namespace Microsoft::Xna::Framework::Input;

namespace TicTacToe {

enum class TicTacToeState {
    PlayerTurn,
    AITurn,
    GameOver
};

class TicTacToeGame : public Game {
public:
    TicTacToeGame() {
        getContentProperty().setRootDirectoryProperty("Content");
        graphics_ = std::make_unique<GraphicsDeviceManager>(this);
        graphics_->setPreferredBackBufferWidthProperty(800);
        graphics_->setPreferredBackBufferHeightProperty(600);
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "TicTacToeGame";
        return name;
    }

protected:
    void LoadContent() override {
        spriteBatch_ = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());

        blankTexture_  = getContentProperty().Load<Texture2D>("blank");
        circleTexture_ = getContentProperty().Load<Texture2D>("circle");
        xTexture_      = getContentProperty().Load<Texture2D>("x");
        font_          = getContentProperty().Load<SpriteFont>("font");

        helpTexture_.emplace(getContentProperty().Load<Texture2D>("help"));

        InitBoard();
    }

    void Update(GameTime& gameTime) override {
        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();

        // F1 help overlay
        bool curF1 = Keyboard::GetState().IsKeyDown(Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        // Exit
        if (Keyboard::GetState().IsKeyDown(Keys::Escape)) {
            Exit();
        }

        auto mouse = Mouse::GetState();
        bool curLeft = (mouse.getLeftButtonProperty() == ButtonState::Pressed);
        bool clicked = curLeft && !prevLeft_;
        prevLeft_ = curLeft;

        switch (state_) {
            case TicTacToeState::PlayerTurn:
                if (clicked) {
                    HandleBoardClick(mouse.getXProperty(), mouse.getYProperty());
                }
                break;

            case TicTacToeState::AITurn:
                aiDelay_ -= elapsed;
                if (aiDelay_ <= 0.0f) {
                    DoAIMove();
                }
                break;

            case TicTacToeState::GameOver:
                // N key = new game
                if (Keyboard::GetState().IsKeyDown(Keys::N) && !prevN_) {
                    ResetGame();
                }
                prevN_ = Keyboard::GetState().IsKeyDown(Keys::N);
                break;
        }

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Color(0x1a, 0x1a, 0x2e, 255));

        spriteBatch_->Begin();

        DrawBoard();
        DrawMarks();
        DrawStatusText();

        // F1 help overlay
        if (helpTimer_ > 0.0f && helpTexture_.has_value()) {
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
    static constexpr int kRows    = 3;
    static constexpr int kCols    = 3;
    static constexpr int kSquareW = 90;
    static constexpr int kSquareH = 90;
    static constexpr int kBoardW  = 300;
    static constexpr int kBoardH  = 300;
    static constexpr int kGap     = 5;

    std::unique_ptr<GraphicsDeviceManager> graphics_;
    std::unique_ptr<SpriteBatch>           spriteBatch_;

    std::optional<Texture2D>  blankTexture_;
    std::optional<Texture2D>  circleTexture_;
    std::optional<Texture2D>  xTexture_;
    std::optional<SpriteFont> font_;

    std::optional<Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool  prevF1_    = false;

    // board: ' '=empty, 'X'=player, 'O'=AI
    char board_[kRows][kCols];
    Rectangle squareBounds_[kRows][kCols];
    Rectangle boardBounds_;

    TicTacToeState state_  = TicTacToeState::PlayerTurn;
    std::string    status_ = "Your turn — click a square (X)";

    bool  prevLeft_ = false;
    bool  prevN_    = false;
    float aiDelay_  = 0.0f;

    System::Random random_;

    void InitBoard() {
        for (int r = 0; r < kRows; r++)
            for (int c = 0; c < kCols; c++)
                board_[r][c] = ' ';

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        int bx = (vp.getWidthProperty()  - kBoardW) / 2;
        int by = (vp.getHeightProperty() - kBoardH) / 2 - 20;


        boardBounds_ = Rectangle(bx, by, kBoardW, kBoardH);

        int cellStep = (kBoardW - kGap) / kCols;
        for (int r = 0; r < kRows; r++) {
            for (int c = 0; c < kCols; c++) {
                squareBounds_[r][c] = Rectangle(
                    bx + kGap + c * cellStep,
                    by + kGap + r * cellStep,
                    kSquareW,
                    kSquareH);
            }
        }

        // Randomly choose who goes first
        if (random_.Next(0, 2) == 0) {
            state_    = TicTacToeState::AITurn;
            aiDelay_  = 0.6f;
            status_   = "AI goes first…";
        } else {
            state_  = TicTacToeState::PlayerTurn;
            status_ = "Your turn — click a square (X)";
        }
    }

    void ResetGame() {
        for (int r = 0; r < kRows; r++)
            for (int c = 0; c < kCols; c++)
                board_[r][c] = ' ';

        if (random_.Next(0, 2) == 0) {
            state_    = TicTacToeState::AITurn;
            aiDelay_  = 0.6f;
            status_   = "AI goes first…";
        } else {
            state_  = TicTacToeState::PlayerTurn;
            status_ = "Your turn — click a square (X)";
        }
    }

    void HandleBoardClick(int mx, int my) {
        for (int r = 0; r < kRows; r++) {
            for (int c = 0; c < kCols; c++) {
                Rectangle& sq = squareBounds_[r][c];
                if (mx >= sq.X && mx < sq.X + sq.Width &&
                    my >= sq.Y && my < sq.Y + sq.Height) {
                    if (board_[r][c] == ' ') {
                        board_[r][c] = 'X';
                        AfterMove();
                    }
                    return;
                }
            }
        }
    }

    void DoAIMove() {
        // Pick a random empty square
        int r, c;
        do {
            r = random_.Next(0, kRows);
            c = random_.Next(0, kCols);
        } while (board_[r][c] != ' ');

        board_[r][c] = 'O';
        AfterMove();
    }

    void AfterMove() {
        char w = CheckWinner();
        if (w == 'X') {
            status_ = "You win!  (N = new game)";
            state_  = TicTacToeState::GameOver;
        } else if (w == 'O') {
            status_ = "AI wins!  (N = new game)";
            state_  = TicTacToeState::GameOver;
        } else if (w == 'T') {
            status_ = "Tie!  (N = new game)";
            state_  = TicTacToeState::GameOver;
        } else if (state_ == TicTacToeState::PlayerTurn) {
            state_   = TicTacToeState::AITurn;
            aiDelay_ = 0.5f;
            status_  = "AI is thinking…";
        } else {
            state_  = TicTacToeState::PlayerTurn;
            status_ = "Your turn — click a square (X)";
        }
    }

    char CheckWinner() {
        // Rows and columns
        for (int i = 0; i < 3; i++) {
            if (board_[i][0] != ' ' && board_[i][0] == board_[i][1] && board_[i][1] == board_[i][2])
                return board_[i][0];
            if (board_[0][i] != ' ' && board_[0][i] == board_[1][i] && board_[1][i] == board_[2][i])
                return board_[0][i];
        }
        // Diagonals
        if (board_[0][0] != ' ' && board_[0][0] == board_[1][1] && board_[1][1] == board_[2][2])
            return board_[0][0];
        if (board_[0][2] != ' ' && board_[0][2] == board_[1][1] && board_[1][1] == board_[2][0])
            return board_[0][2];
        // Check for tie (board full)
        for (int r = 0; r < kRows; r++)
            for (int c = 0; c < kCols; c++)
                if (board_[r][c] == ' ') return ' ';
        return 'T';
    }

    void DrawBoard() {
        // Board background
        spriteBatch_->Draw(*blankTexture_, boardBounds_, Color(30, 30, 50, 255));

        // Individual squares
        for (int r = 0; r < kRows; r++) {
            for (int c = 0; c < kCols; c++) {
                spriteBatch_->Draw(*blankTexture_, squareBounds_[r][c], Color(230, 230, 230, 255));
            }
        }
    }

    void DrawMarks() {
        for (int r = 0; r < kRows; r++) {
            for (int c = 0; c < kCols; c++) {
                if (board_[r][c] == 'X') {
                    spriteBatch_->Draw(*xTexture_, squareBounds_[r][c], Color(220, 50, 50, 255));
                } else if (board_[r][c] == 'O') {
                    spriteBatch_->Draw(*circleTexture_, squareBounds_[r][c], Color(50, 150, 220, 255));
                }
            }
        }
    }

    void DrawStatusText() {
        if (!font_.has_value()) return;

        auto& vp = getGraphicsDeviceProperty().getViewportProperty();
        Vector2 measure = font_->MeasureString(status_);
        float tx = (vp.getWidthProperty()  - measure.X) / 2.0f;
        float ty = (float)(boardBounds_.Y + boardBounds_.Height + 20);

        // Shadow
        spriteBatch_->DrawString(*font_, status_, Vector2(tx + 1, ty + 1), Color(0, 0, 0, 200));
        spriteBatch_->DrawString(*font_, status_, Vector2(tx, ty), Color(255, 255, 255, 255));
    }
};

} // namespace TicTacToe

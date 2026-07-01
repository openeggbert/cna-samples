#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureSample.hpp"
#include "Microsoft/Xna/Framework/Input/Touch/GestureType.hpp"

#include "YachtTypes.hpp"
#include "YachtPlayer.hpp"
#include "Dice.hpp"
#include "DiceHandler.hpp"
#include "HumanPlayer.hpp"
#include "AIPlayer.hpp"
#include "AudioManager.hpp"
#include "InputState.hpp"

namespace Yacht {

using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::MathHelper;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using Microsoft::Xna::Framework::Input::Touch::GestureSample;
using Microsoft::Xna::Framework::Input::Touch::GestureType;

// Bundle of the score-related fonts GameStateHandler needs to draw the
// score card and leader board. Threaded in explicitly by GameplayScreen
// (which owns the actual SpriteFont instances loaded from its own
// ContentManager calls) rather than reached for via a YachtGame-wide
// static, to avoid a header-only circular include between YachtGame.hpp and
// the gameplay object headers it composes (see missing.md).
struct ScoreFonts {
    SpriteFont* Regular;
    SpriteFont* Score;
    SpriteFont* ScoreBold;
    SpriteFont* LeaderScore;
};

// Manages the local (offline) turn state machine and draws the score card /
// leader board. Ported from Objects/GameStateHandler.cs (whose own file
// header still says "ScoreCard.cs" -- a leftover from an earlier refactor
// in the original itself). Per the approved plan, every GameTypes.Online-
// gated code path (IsWaitingForPlayer, InitializeOnlinePlayers, SetState,
// UpdateScoreCard, the server-driven half of FinishTurn, ShowGameOver, and
// the "message"/"waiting for other players" HUD text, which was always
// null/false on the local turn state machine and dead in offline play
// anyway) is dropped; only the local turn state machine remains: Roll ->
// hold dice -> SelectScore -> FinishTurn, 12 rounds per player, highest
// total score wins.
class GameStateHandler {
public:
    static const std::array<std::string, 12> ScoreTypesNames;

    GameStateHandler(DiceHandler& diceHandler, InputState& input, const std::string& name,
                     Rectangle screenBounds, ContentManager& contentManager,
                     SpriteFont& font, ScoreFonts fonts)
        : diceHandler_(&diceHandler), input_(&input), screenBounds_(screenBounds),
          contentManager_(&contentManager), font_(&font), fonts_(fonts) {
        LoadNewOfflinePlayers(name);
    }

    bool IsInitialized() const { return isInitialized_; }

    bool IsScoreSelect() const { return selectedScore_.has_value(); }
    std::optional<YachtCombination> SelectedScore() const { return selectedScore_; }

    YachtPlayer* CurrentPlayer() const { return players_[state_.CurrentPlayer].get(); }
    YachtPlayer* WinnerPlayer() const { return winnerPlayer_; }
    bool IsGameOver() const { return isGameOver_; }

    void HandleInput(const GestureSample& sample) {
        if (sample.getGestureTypeProperty() == GestureType::VerticalDrag) {
            ScrollBy(sample.getPositionProperty(), sample.getDeltaProperty().Y);
        }
    }

    // Mouse: the desktop parallel to the gesture-based VerticalDrag scroll
    // above -- called once per frame (not per-gesture).
    void HandleMouseInput(InputState& input) {
        if (input.IsLeftMouseDown()) {
            ScrollBy(input.MousePosition(), input.MouseDelta().Y);
        }
    }

    void Draw(SpriteBatch& spriteBatch) {
        DrawScore(spriteBatch);
        DrawLeaderBoard(spriteBatch);
    }

    // Write the selected score to the score table and move to the next
    // player. Checks if the game is over.
    void FinishTurn() {
        if (!selectedScore_.has_value())
            return;

        PlayerInformation& current = state_.Players[state_.CurrentPlayer];
        int scoreIndex = (int)selectedScore_.value() - 1;
        current.ScoreCard[scoreIndex] = CombinationScore(selectedScore_.value(), currentDice_.value());
        current.TotalScore += current.ScoreCard[scoreIndex];

        state_.CurrentPlayer = (state_.CurrentPlayer + 1) % (int)players_.size();
        state_.StepsMade++;

        if (state_.StepsMade == 12 * (int)players_.size()) {
            state_.CurrentPlayer = HighestPlayerScore();
            winnerPlayer_ = players_[state_.CurrentPlayer].get();
            isGameOver_ = true;

            if (dynamic_cast<HumanPlayer*>(winnerPlayer_) != nullptr)
                AudioManager::PlaySound("Winner");
            else
                AudioManager::PlaySound("Loss");
        } else {
            AudioManager::PlaySoundRandom("TurnChange", 2);
        }

        selectedScore_.reset();
    }

    // Sets the dice used to calculate the possible scores.
    void setScoreDice(std::optional<std::array<Dice*, DiceHandler::DiceAmount>> dice) {
        currentDice_ = dice;
    }

    // Select a score line to serve as the user's score for the current turn.
    bool SelectScore(std::optional<YachtCombination> selectedScore) {
        if (selectedScore.has_value() &&
            state_.Players[state_.CurrentPlayer].ScoreCard[(int)selectedScore.value() - 1] == NullScore &&
            currentDice_.has_value()) {
            selectedScore_ = selectedScore;
            AudioManager::PlaySound("ScoreSelect");
            return true;
        } else if (!selectedScore.has_value()) {
            selectedScore_.reset();
            return true;
        }
        return false;
    }

    // Checks whether a specified rectangle intersects a specified score line.
    bool IntersectLine(Rectangle rectangle, int index) const {
        rectangle.Y -= (int)scoreOffset_.Y;
        return scoreLine_[index].Intersects(rectangle);
    }

    // Calculate the score of the supplied dice according to a specified
    // combination. `dice` holds 5 entries, some of which may be nullptr.
    static int CombinationScore(YachtCombination combination,
                                std::array<Dice*, DiceHandler::DiceAmount> dice) {
        std::sort(dice.begin(), dice.end(),
                 [](const Dice* a, const Dice* b) { return DiceLessThan(a, b); });

        Dice* first = First(dice);
        Dice* last = Last(dice);

        switch (combination) {
            case YachtCombination::Yacht:
                if (first != nullptr && last != nullptr && Times(dice, first->Value) == 5)
                    return 50;
                return 0;
            case YachtCombination::LargeStraight:
                if (first != nullptr && last != nullptr &&
                    CheckConsecutiveDice(dice) && last->Value == DiceValue::Six)
                    return 30;
                return 0;
            case YachtCombination::SmallStraight:
                if (first != nullptr && last != nullptr &&
                    CheckConsecutiveDice(dice) && last->Value == DiceValue::Five)
                    return 30;
                return 0;
            case YachtCombination::FourOfAKind:
                if (first != nullptr && last != nullptr &&
                    (Times(dice, first->Value) >= 4 || Times(dice, last->Value) >= 4))
                    return Sum(dice, std::nullopt);
                return 0;
            case YachtCombination::FullHouse:
                if (first != nullptr && last != nullptr &&
                    ((Times(dice, first->Value) == 3 && Times(dice, last->Value) == 2) ||
                     (Times(dice, first->Value) == 2 && Times(dice, last->Value) == 3)))
                    return Sum(dice, std::nullopt);
                return 0;
            case YachtCombination::Choise:
                return Sum(dice, std::nullopt);
            case YachtCombination::Sixes:
            case YachtCombination::Fives:
            case YachtCombination::Fours:
            case YachtCombination::Threes:
            case YachtCombination::Twos:
            case YachtCombination::Ones:
                return Sum(dice, (DiceValue)(int)combination);
            default:
                return 0;
        }
    }

private:
    // Load and initialize the offline players (the original's
    // LoadNewOfflinePlayers -- there is no "load saved game" path left,
    // since tombstoning/save-load is dropped).
    void LoadNewOfflinePlayers(const std::string& name) {
        auto human = std::make_unique<HumanPlayer>(name, diceHandler_, *input_, screenBounds_, *font_);
        human->LoadAssets(*contentManager_);
        players_.push_back(std::move(human));
        players_.push_back(std::make_unique<AIPlayer>("Josh", diceHandler_));
        players_.push_back(std::make_unique<AIPlayer>("Charles", diceHandler_));
        players_.push_back(std::make_unique<AIPlayer>("Alex", diceHandler_));

        state_.Players.clear();
        for (auto& p : players_) {
            p->setGameStateHandler(this);
            PlayerInformation info;
            info.Name = p->getName();
            info.ScoreCard.fill(NullScore);
            state_.Players.push_back(info);
        }

        LoadAssets();

        for (size_t i = 0; i < scorePosition_.size(); i++)
            scorePosition_[i] = Vector2(20, 50.0f + 42.0f * (float)i);

        for (size_t i = 0; i < scoreLine_.size(); i++)
            scoreLine_[i] = Rectangle((int)scorePosition_[i].X, (int)scorePosition_[i].Y, 200, 42);

        for (size_t i = 0; i < players_.size(); i++)
            playerPositions_[i] = Vector2(
                (float)(screenBounds_.getRightProperty() - leaderBoardTexture_->getWidthProperty()),
                (float)(screenBounds_.getTopProperty() + 10 +
                       (leaderBoardTexture_->getHeightProperty() + 20) * (int)i));

        isInitialized_ = true;
    }

    void LoadAssets() {
        scoreCardTexture_.emplace(contentManager_->Load<Texture2D>("Images/NameAndTotal"));
        scoreLinesTexture_.emplace(contentManager_->Load<Texture2D>("Images/Score"));
        leaderBoardTexture_.emplace(contentManager_->Load<Texture2D>("Images/leaderboardBg"));
        activeLeaderBoardTexture_.emplace(contentManager_->Load<Texture2D>("Images/leaderboardBg_active"));
        scrollThumbTexture_.emplace(contentManager_->Load<Texture2D>("Images/ScrollThumb"));
        starTexture_.emplace(contentManager_->Load<Texture2D>("Images/Dot"));
    }

    void ScrollBy(Vector2 position, float deltaY) {
        Rectangle touchRect((int)position.X - 5, (int)position.Y - 5, 10, 10);
        Rectangle scrollLineBounds = scoreLinesTexture_->getBoundsProperty();
        scrollLineBounds.Y += 10;

        if (scrollLineBounds.Intersects(touchRect))
            scoreOffset_.Y += deltaY;

        scoreOffset_.Y = MathHelper::Clamp(scoreOffset_.Y,
                                          (float)(scrollLineRectDestination_.Height - scrollLineBounds.Height), 0.0f);
    }

    void DrawScore(SpriteBatch& spriteBatch) {
        Rectangle sourceScrollLineRect = scrollLineRectDestination_;
        sourceScrollLineRect.Y = 0;
        sourceScrollLineRect.Y -= (int)scoreOffset_.Y;
        spriteBatch.Draw(*scoreLinesTexture_, scrollLineRectDestination_, sourceScrollLineRect, Color::White);

        PlayerInformation& current = state_.Players[state_.CurrentPlayer];

        for (size_t i = 0; i < ScoreTypesNames.size(); i++) {
            Vector2 position = scorePosition_[i] + scoreOffset_;
            if (scrollLineRectDestination_.Contains((int)position.X, (int)position.Y)) {
                spriteBatch.DrawString(*fonts_.Score, ScoreTypesNames[i], position,
                                      (YachtCombination)(i + 1) == selectedScore_ ? Color::Red : Color::Black);
            }

            int shown = current.ScoreCard[i];
            Color color = Color::Black;

            if (shown == NullScore && currentDice_.has_value()) {
                shown = CombinationScore((YachtCombination)(i + 1), currentDice_.value());
                color = (YachtCombination)(i + 1) == selectedScore_ ? Color::Red : Color::Gray;
            }

            if (shown != NullScore && scrollLineRectDestination_.Contains((int)position.X, (int)position.Y)) {
                spriteBatch.DrawString(*fonts_.Score, std::to_string(shown),
                                      scorePosition_[i] + Vector2(160, 0) + scoreOffset_, color);
            }
        }

        spriteBatch.Draw(*scoreCardTexture_, Vector2(0, 10), Color::White);

        float scrollYPos = (float)(scrollLineRectDestination_.Height - scrollThumbTexture_->getHeightProperty()) /
                          (float)(scoreLinesTexture_->getHeightProperty() - scrollLineRectDestination_.Height) *
                          scoreOffset_.Y;
        spriteBatch.Draw(*scrollThumbTexture_, Vector2(0, 45 - scrollYPos), Color::White);

        spriteBatch.DrawString(*fonts_.ScoreBold,
                              "#" + std::to_string(state_.CurrentPlayer + 1) + " " +
                                  players_[state_.CurrentPlayer]->getName(),
                              Vector2(10, 10), Color::Brown);

        spriteBatch.DrawString(*fonts_.ScoreBold, "Total", totalScore_, Color::Brown);
        spriteBatch.DrawString(*fonts_.ScoreBold, std::to_string(current.TotalScore),
                              totalScore_ + Vector2(160, 0), Color::Brown);
    }

    void DrawLeaderBoard(SpriteBatch& spriteBatch) {
        for (size_t i = 0; i < players_.size(); i++) {
            spriteBatch.Draw((int)i == state_.CurrentPlayer ? *activeLeaderBoardTexture_ : *leaderBoardTexture_,
                             playerPositions_[i], Color::White);

            Vector2 measure = fonts_.Regular->MeasureString(players_[i]->getName());
            Vector2 playerNamePosition = playerPositions_[i] +
                Vector2((float)leaderBoardTexture_->getBoundsProperty().Width * 3.0f / 5.0f - measure.X, 0);

            spriteBatch.DrawString(*fonts_.Regular, players_[i]->getName(), playerNamePosition, Color::White);

            std::string total = std::to_string(state_.Players[i].TotalScore);
            measure = fonts_.Regular->MeasureString(total);
            Vector2 totalScorePosition = playerPositions_[i] +
                Vector2((float)leaderBoardTexture_->getWidthProperty() - measure.X - 20.0f, 0);

            spriteBatch.DrawString(*fonts_.LeaderScore, total, totalScorePosition, Color::White);

            if (dynamic_cast<HumanPlayer*>(players_[i].get()) != nullptr)
                spriteBatch.Draw(*starTexture_, playerNamePosition - Vector2(20, -10), Color::White);
        }
    }

    int HighestPlayerScore() const {
        int playerIndex = 0;
        for (size_t i = 0; i < players_.size(); i++) {
            if (AccumulateScore((int)i) > AccumulateScore(playerIndex))
                playerIndex = (int)i;
        }
        return playerIndex;
    }

    int AccumulateScore(int playerIndex) const {
        int total = 0;
        for (int i = 0; i < 12; i++)
            if (state_.Players[playerIndex].ScoreCard[i] != NullScore)
                total += state_.Players[playerIndex].ScoreCard[i];
        return total;
    }

    static Dice* First(const std::array<Dice*, DiceHandler::DiceAmount>& dice) {
        for (auto* d : dice) if (d != nullptr) return d;
        return nullptr;
    }

    static Dice* Last(const std::array<Dice*, DiceHandler::DiceAmount>& dice) {
        for (auto it = dice.rbegin(); it != dice.rend(); ++it) if (*it != nullptr) return *it;
        return nullptr;
    }

    static int Sum(const std::array<Dice*, DiceHandler::DiceAmount>& dice, std::optional<DiceValue> value) {
        int sum = 0;
        for (auto* d : dice)
            if (d != nullptr && (!value.has_value() || d->Value == value.value()))
                sum += (int)d->Value;
        return sum;
    }

    static int Times(const std::array<Dice*, DiceHandler::DiceAmount>& dice, std::optional<DiceValue> value) {
        int count = 0;
        for (auto* d : dice)
            if (d != nullptr && (!value.has_value() || d->Value == value.value()))
                count++;
        return count;
    }

    static bool CheckConsecutiveDice(const std::array<Dice*, DiceHandler::DiceAmount>& dice) {
        int count = 0;
        for (size_t i = 0; i + 1 < dice.size(); i++)
            if (dice[i] != nullptr && dice[i + 1] != nullptr &&
                (int)dice[i]->Value + 1 == (int)dice[i + 1]->Value)
                count++;
        return count == (int)dice.size() - 1;
    }

    DiceHandler* diceHandler_;
    InputState* input_;
    Rectangle screenBounds_;
    ContentManager* contentManager_;
    SpriteFont* font_;
    ScoreFonts fonts_;

    std::vector<std::unique_ptr<YachtPlayer>> players_;
    GameState state_;
    bool isInitialized_ = false;
    bool isGameOver_ = false;
    YachtPlayer* winnerPlayer_ = nullptr;
    std::optional<YachtCombination> selectedScore_;
    std::optional<std::array<Dice*, DiceHandler::DiceAmount>> currentDice_;

    std::optional<Texture2D> scoreCardTexture_, scoreLinesTexture_, leaderBoardTexture_,
                             activeLeaderBoardTexture_, scrollThumbTexture_, starTexture_;

    Vector2 scoreOffset_ = Vector2::Zero;
    std::array<Vector2, 12> scorePosition_{};
    std::array<Vector2, 4> playerPositions_{};
    Vector2 totalScore_ = Vector2(20, 445);

    std::array<Rectangle, 12> scoreLine_{};
    Rectangle scrollLineRectDestination_ = Rectangle(0, 42, 232, 405);
};

inline const std::array<std::string, 12> GameStateHandler::ScoreTypesNames = {
    "Ones", "Twos", "Threes", "Fours", "Fives", "Sixes", "Choice",
    "Full House", "4 of a kind", "Small 1-5", "Large 2-6", "Yacht"
};

// ---- HumanPlayer methods deferred from HumanPlayer.hpp (need GameStateHandler) ----

inline bool HumanPlayer::CanSelectScore() const {
    return gameStateHandler_ != nullptr && gameStateHandler_->IsScoreSelect();
}

inline void HumanPlayer::TryMoveDiceAt(Vector2 point) {
    auto rollingDice = diceHandler_->getRollingDice();
    auto holdingDice = diceHandler_->getHoldingDice();

    Rectangle touchRect((int)point.X - 5, (int)point.Y - 5, 10, 10);

    for (int i = 0; i < DiceHandler::DiceAmount; i++) {
        bool rollHit = rollingDice.has_value() && (*rollingDice)[i] != nullptr &&
                      !(*rollingDice)[i]->IsRolling() && (*rollingDice)[i]->Intersects(touchRect);
        bool holdHit = holdingDice.has_value() && (*holdingDice)[i] != nullptr &&
                      (*holdingDice)[i]->Intersects(touchRect);

        if (rollHit || holdHit) {
            diceHandler_->MoveDice(i);
            if (!diceHandler_->getHoldingDice().has_value())
                gameStateHandler_->SelectScore(std::nullopt);
        }
    }
}

inline void HumanPlayer::TrySelectScoreAt(Vector2 point) {
    Rectangle touchRect((int)point.X - 5, (int)point.Y - 5, 10, 10);

    for (int i = 0; i < 12; i++) {
        if (gameStateHandler_->IntersectLine(touchRect, i)) {
            gameStateHandler_->SelectScore((YachtCombination)(i + 1));
        }
    }
}

inline void HumanPlayer::DrawSelectedScore(SpriteBatch& spriteBatch) {
    if (gameStateHandler_ != nullptr && gameStateHandler_->IsScoreSelect()) {
        auto holdingDice = diceHandler_->getHoldingDice();
        if (holdingDice.has_value()) {
            YachtCombination selected = gameStateHandler_->SelectedScore().value();
            int selectedScoreValue = GameStateHandler::CombinationScore(selected, *holdingDice);

            std::string text = GameStateHandler::ScoreTypesNames[(int)selected - 1];
            for (char& c : text) c = (char)std::toupper((unsigned char)c);

            Vector2 position((float)score_->Position.X, (float)roll_->Position.Y);
            position.Y += (float)(roll_->getTexture().getHeightProperty() + 10);
            Vector2 measure = font_->MeasureString(text);
            position.X += (float)score_->getTexture().getBoundsProperty().getCenterProperty().X - measure.X / 2.0f;
            spriteBatch.DrawString(*font_, text, position, Color::White);

            text = std::to_string(selectedScoreValue);
            position.Y += measure.Y;
            measure = font_->MeasureString(text);
            position.X = (float)score_->Position.X;
            position.X += (float)score_->getTexture().getBoundsProperty().getCenterProperty().X - measure.X / 2.0f;
            spriteBatch.DrawString(*font_, text, position, Color::White);
        }
    }
}

inline void HumanPlayer::HandleScoreButtonClick() {
    if (gameStateHandler_ != nullptr && gameStateHandler_->IsScoreSelect()) {
        gameStateHandler_->FinishTurn();
        AudioManager::PlaySoundRandom("Pencil", 3);
        diceHandler_->Reset(gameStateHandler_->IsGameOver());
    }
}

// ---- AIPlayer method deferred from AIPlayer.hpp (needs GameStateHandler) ----

inline void AIPlayer::PerformPlayerLogic() {
    switch (State) {
        case AIState::Roll:
            diceHandler_->Roll();
            State = AIState::Rolling;
            break;
        case AIState::Rolling:
            if (!diceHandler_->DiceRolling())
                State = AIState::ChooseDice;
            break;
        case AIState::ChooseDice:
            diceHandler_->MoveDice(random_.Next(0, 5));
            if (diceHandler_->getHoldingDice().has_value() && random_.Next(0, 5) == 1)
                State = AIState::SelectScore;
            break;
        case AIState::SelectScore:
            if (gameStateHandler_->SelectScore((YachtCombination)random_.Next(1, 13)))
                State = AIState::WriteScore;
            break;
        case AIState::WriteScore:
            if (gameStateHandler_ != nullptr && gameStateHandler_->IsScoreSelect()) {
                gameStateHandler_->FinishTurn();
                diceHandler_->Reset(gameStateHandler_->IsGameOver());
                State = AIState::Roll;
            }
            break;
        default:
            break;
    }
}

} // namespace Yacht

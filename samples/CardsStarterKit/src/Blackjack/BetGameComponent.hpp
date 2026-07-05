#pragma once

// BetGameComponent.hpp -- C++ port of Misc/BetGameComponent.cs (XNA 4.0
// CardsStarterKit sample). Draws the betting chips and Deal/Clear buttons,
// and drives the AI players' automatic betting.

#include <array>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/DrawableGameComponent.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Input/Mouse.hpp"
#include "Microsoft/Xna/Framework/Input/MouseState.hpp"
#include "Microsoft/Xna/Framework/Input/ButtonState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "../CardsFramework/CardsGame.hpp"
#include "../CardsFramework/FlipGameComponentAnimation.hpp"
#include "../CardsFramework/Player.hpp"
#include "../CardsFramework/TransitionGameComponentAnimation.hpp"
#include "../GameStateManagement/InputState.hpp"
#include "AudioManager.hpp"
#include "BlackJackTable.hpp"
#include "BlackjackCommon.hpp"
#include "BlackjackAIPlayer.hpp"
#include "BlackjackPlayer.hpp"
#include "Button.hpp"
#include "InputHelper.hpp"

namespace Blackjack {

using CardsFramework::AnimatedGameComponent;
using CardsFramework::CardsGame;
using CardsFramework::FlipGameComponentAnimation;
using CardsFramework::TransitionGameComponentAnimation;
using GameStateManagement::InputState;
using Microsoft::Xna::Framework::DrawableGameComponent;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Color;
using Microsoft::Xna::Framework::Input::Mouse;
using Microsoft::Xna::Framework::Input::MouseState;
using Microsoft::Xna::Framework::Input::ButtonState;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class BlackjackCardGame;  // forward declaration (Game/BlackjackCardGame.hpp)

class BetGameComponent : public DrawableGameComponent {
public:
    BetGameComponent(std::vector<std::shared_ptr<CardsFramework::Player>>& players, InputState& input,
                     const std::string& theme, CardsGame& cardGame)
        : DrawableGameComponent(*cardGame.GameInstance), players_(players), theme_(theme), cardGame_(cardGame),
          input_(input) {}

    void Initialize() override {
        for (auto* c : getGameProperty().getComponentsProperty()) {
            inputHelper_ = dynamic_cast<InputHelper*>(c);
            if (inputHelper_) break;
        }

        getGameProperty().setIsMouseVisibleProperty(true);
        DrawableGameComponent::Initialize();

        spriteBatch_.emplace(getGraphicsDeviceProperty());

        Rectangle size = chipsAssets_.at(assetNames_[0]).getBoundsProperty();
        Rectangle bounds = spriteBatch_->getGraphicsDeviceProperty()->getViewportProperty().getTitleSafeAreaProperty();

        positions_.resize(chipsAssets_.size());
        positions_[chipsAssets_.size() - 1] =
            Vector2((float)(bounds.X + 10), (float)(bounds.Y + bounds.Height - size.Height - 80));
        for (size_t chipIndex = 2; chipIndex <= chipsAssets_.size(); chipIndex++) {
            size = chipsAssets_.at(assetNames_[chipsAssets_.size() - chipIndex]).getBoundsProperty();
            positions_[chipsAssets_.size() - chipIndex] =
                positions_[chipsAssets_.size() - (chipIndex - 1)] - Vector2(0.0f, size.Height + 10.0f);
        }

        bet_ = cardGame_.AddComponent<Button>("ButtonRegular", "ButtonPressed", input_, cardGame_);
        bet_->Bounds = Rectangle(bounds.X + 10, bounds.Y + bounds.Height - 60, 100, 50);
        bet_->Font = &*cardGame_.Font;
        bet_->Text = std::string("Deal");
        bet_->Click.Add([this](System::Object*, const System::EventArgs&) { Bet_Click(); });

        clear_ = cardGame_.AddComponent<Button>("ButtonRegular", "ButtonPressed", input_, cardGame_);
        clear_->Bounds = Rectangle(bounds.X + 120, bounds.Y + bounds.Height - 60, 100, 50);
        clear_->Font = &*cardGame_.Font;
        clear_->Text = std::string("Clear");
        clear_->Click.Add([this](System::Object*, const System::EventArgs&) { Clear_Click(); });

        ShowAndEnableButtons(false);
    }

    void LoadContent() override {
        blankChip_ = getGameProperty().getContentProperty().Load<Texture2D>("Images/Chips/chipWhite");

        for (int assetName : assetNames_)
            chipsAssets_.emplace(assetName,
                                 getGameProperty().getContentProperty().Load<Texture2D>(
                                     "Images/Chips/chip" + std::to_string(assetName)));

        DrawableGameComponent::LoadContent();
    }

    void Update(GameTime& gameTime) override;  // defined in BlackjackCardGame.hpp (needs BlackjackCardGame)

    void Draw(const GameTime& gameTime) override {
        spriteBatch_->Begin();

        for (size_t chipIndex = 0; chipIndex < chipsAssets_.size(); chipIndex++)
            spriteBatch_->Draw(chipsAssets_.at(assetNames_[chipIndex]), positions_[chipIndex], Color::White);

        for (size_t playerIndex = 0; playerIndex < players_.size(); playerIndex++) {
            auto* table = static_cast<BlackJackTable*>(cardGame_.GameTableInstance.get());
            Vector2 position = (*table)[(int)playerIndex] + table->RingOffset +
                               Vector2((float)table->RingTexture.getBoundsProperty().Width, 0.0f);
            auto* player = static_cast<BlackjackPlayer*>(players_[playerIndex].get());
            spriteBatch_->DrawString(*cardGame_.Font, "$" + std::to_string((int)player->BetAmount), position, Color::White);
            spriteBatch_->DrawString(*cardGame_.Font, "$" + std::to_string((int)player->Balance),
                                     position + Vector2(0.0f, 30.0f), Color::White);
        }

        spriteBatch_->End();
        DrawableGameComponent::Draw(gameTime);
    }

    // Adds a chip to a player's betting zone.
    void AddChip(int playerIndex, int chipValue, bool secondHand) {
        auto* player = static_cast<BlackjackPlayer*>(players_[playerIndex].get());
        if (player->Bet((float)chipValue)) {
            currentBet_ += chipValue;

            auto chipComponent = cardGame_.AddComponent<AnimatedGameComponent>(cardGame_, &chipsAssets_.at(chipValue));
            chipComponent->setVisibleProperty(false);

            Vector2 offset = GetChipOffset(playerIndex, secondHand);
            Vector2 position = (*cardGame_.GameTableInstance)[playerIndex] + offset +
                               Vector2(-(float)currentChipComponents_.size() * 2.0f, (float)currentChipComponents_.size());

            int currentChipIndex = 0;
            for (size_t chipIndex = 0; chipIndex < assetNames_.size(); chipIndex++) {
                if (assetNames_[chipIndex] == chipValue) {
                    currentChipIndex = (int)chipIndex;
                    break;
                }
            }

            auto transition = std::make_unique<TransitionGameComponentAnimation>(positions_[currentChipIndex], position);
            transition->Duration = TimeSpan::FromSeconds(1.0);
            AnimatedGameComponent* rawChip = chipComponent.get();
            transition->PerformBeforeStart = [rawChip](void*) { rawChip->setVisibleProperty(true); };
            transition->PerformWhenDone = [](void*) { AudioManager::PlaySound("Bet"); };
            chipComponent->AddAnimation(std::move(transition));

            auto flip = std::make_unique<FlipGameComponentAnimation>();
            flip->Duration = TimeSpan::FromSeconds(1.0);
            flip->setAnimationCycles(3);
            chipComponent->AddAnimation(std::move(flip));

            currentChipComponents_.push_back(chipComponent);
        }
    }

    // Adds chips totalling `amount` to a player, or an insurance chip.
    void AddChips(int playerIndex, float amount, bool insurance, bool secondHand) {
        if (insurance)
            AddInsuranceChipAnimation(amount);
        else
            AddChipsForAmount(playerIndex, amount, secondHand);
    }

    void Reset() {
        ShowAndEnableButtons(true);
        currentChipComponents_.clear();
    }

    // Updates every player's balance in light of their bets and the dealer's hand.
    void CalculateBalance(BlackjackPlayer& dealerPlayer) {
        for (auto& playerBase : players_) {
            auto* player = static_cast<BlackjackPlayer*>(playerBase.get());

            float factor = CalculateFactorForHand(dealerPlayer, *player, HandTypes::First);

            if (player->IsSplit) {
                float factor2 = CalculateFactorForHand(dealerPlayer, *player, HandTypes::Second);
                float initialBet = player->BetAmount / ((player->Double ? 2.0f : 1.0f) + (player->SecondDouble ? 2.0f : 1.0f));
                float bet1 = initialBet * (player->Double ? 2.0f : 1.0f);
                float bet2 = initialBet * (player->SecondDouble ? 2.0f : 1.0f);

                player->Balance += bet1 * factor + bet2 * factor2;

                if (player->IsInsurance && dealerPlayer.BlackJack)
                    player->Balance += initialBet;
            } else {
                if (player->IsInsurance && dealerPlayer.BlackJack)
                    player->Balance += player->BetAmount;

                player->Balance += player->BetAmount * factor;
            }

            player->ClearBet();
        }
    }

    int GetCurrentPlayer() const {
        for (size_t playerIndex = 0; playerIndex < players_.size(); playerIndex++) {
            if (!static_cast<BlackjackPlayer*>(players_[playerIndex].get())->IsDoneBetting)
                return (int)playerIndex;
        }
        return -1;
    }

private:
    void HandleInput(MouseState mouseState) {
        bool isPressed = false;
        Vector2 position;

        if (mouseState.getLeftButtonProperty() == ButtonState::Pressed) {
            isPressed = true;
            position = Vector2((float)mouseState.getXProperty(), (float)mouseState.getYProperty());
        } else if (inputHelper_ && inputHelper_->IsPressed) {
            isPressed = true;
            position = inputHelper_->PointPosition();
        }

        if (isPressed) {
            if (!isKeyDown_) {
                int chipValue = GetIntersectingChipValue(position);
                if (chipValue != 0)
                    AddChip(GetCurrentPlayer(), chipValue, false);
                isKeyDown_ = true;
            }
        } else {
            isKeyDown_ = false;
        }
    }

    int GetIntersectingChipValue(Vector2 position) const {
        Rectangle touchTap((int)position.X - 1, (int)position.Y - 1, 2, 2);
        for (size_t chipIndex = 0; chipIndex < chipsAssets_.size(); chipIndex++) {
            Rectangle size = chipsAssets_.at(assetNames_[chipIndex]).getBoundsProperty();
            size.X = (int)positions_[chipIndex].X;
            size.Y = (int)positions_[chipIndex].Y;
            if (size.Intersects(touchTap))
                return assetNames_[chipIndex];
        }
        return 0;
    }

    void AddChipsForAmount(int playerIndex, float amount, bool secondHand) {
        while (amount > 0) {
            if (amount >= 5) {
                for (int chipIndex = (int)assetNames_.size(); chipIndex > 0; chipIndex--) {
                    while (assetNames_[chipIndex - 1] <= amount) {
                        AddChip(playerIndex, assetNames_[chipIndex - 1], secondHand);
                        amount -= assetNames_[chipIndex - 1];
                    }
                }
            } else {
                amount = 0;
            }
        }
    }

    void AddInsuranceChipAnimation(float amount) {
        auto chipComponent = cardGame_.AddComponent<AnimatedGameComponent>(cardGame_, &blankChip_);
        chipComponent->TextColor = Color::Black;
        chipComponent->setEnabledProperty(true);
        chipComponent->setVisibleProperty(false);

        auto transition = std::make_unique<TransitionGameComponentAnimation>(
            positions_[0], Vector2((float)getGraphicsDeviceProperty().getViewportProperty().getWidthProperty() / 2.0f,
                                   insuranceYPosition_));
        AnimatedGameComponent* rawChip = chipComponent.get();
        transition->PerformBeforeStart = [rawChip](void*) { rawChip->setVisibleProperty(true); };
        std::string amountText = std::to_string((int)amount);
        transition->PerformWhenDone = [rawChip, amountText](void*) {
            rawChip->Text = amountText;
            AudioManager::PlaySound("Bet");
        };
        transition->Duration = TimeSpan::FromSeconds(1.0);
        transition->StartTime = System::DateTime::getNowProperty();
        chipComponent->AddAnimation(std::move(transition));

        auto flip = std::make_unique<FlipGameComponentAnimation>();
        flip->Duration = TimeSpan::FromSeconds(1.0);
        flip->setAnimationCycles(3);
        chipComponent->AddAnimation(std::move(flip));
    }

    Vector2 GetChipOffset(int playerIndex, bool secondHand) const {
        (void)playerIndex;
        auto* table = static_cast<BlackJackTable*>(cardGame_.GameTableInstance.get());
        Vector2 offset = table->RingOffset +
                        Vector2((float)(table->RingTexture.getBoundsProperty().Width - blankChip_.getBoundsProperty().Width),
                               (float)(table->RingTexture.getBoundsProperty().Height - blankChip_.getBoundsProperty().Height)) /
                            2.0f;
        if (secondHand)
            offset = offset + secondHandOffset_;
        return offset;
    }

    void ShowAndEnableButtons(bool visibleEnabled) {
        bet_->setVisibleProperty(visibleEnabled);
        bet_->setEnabledProperty(visibleEnabled);
        clear_->setVisibleProperty(visibleEnabled);
        clear_->setEnabledProperty(visibleEnabled);
    }

    float CalculateFactorForHand(BlackjackPlayer& dealerPlayer, BlackjackPlayer& player, HandTypes currentHand) {
        player.CalculateValues();

        bool blackjack, bust, considerAce;
        int playerValue;
        switch (currentHand) {
            case HandTypes::First:
                blackjack = player.BlackJack;
                bust = player.Bust;
                playerValue = player.FirstValue();
                considerAce = player.FirstValueConsiderAce();
                break;
            case HandTypes::Second:
                blackjack = player.SecondBlackJack;
                bust = player.SecondBust;
                playerValue = player.SecondValue();
                considerAce = player.SecondValueConsiderAce();
                break;
            default:
                throw std::runtime_error("Player has an unsupported hand type.");
        }

        if (considerAce)
            playerValue += 10;

        if (bust) return -1.0f;

        if (dealerPlayer.Bust) return blackjack ? 1.5f : 1.0f;

        if (dealerPlayer.BlackJack) return blackjack ? 0.0f : -1.0f;

        if (blackjack) return 1.5f;

        int dealerValue = dealerPlayer.FirstValue();
        if (dealerPlayer.FirstValueConsiderAce())
            dealerValue += 10;

        if (playerValue > dealerValue) return 1.0f;
        if (playerValue < dealerValue) return -1.0f;
        return 0.0f;
    }

    void Clear_Click() {
        currentBet_ = 0;
        static_cast<BlackjackPlayer*>(players_[GetCurrentPlayer()].get())->ClearBet();
        for (auto& chip : currentChipComponents_)
            cardGame_.RemoveComponent(chip);
        currentChipComponents_.clear();
    }

    void Bet_Click() {
        int playerIndex = GetCurrentPlayer();
        if (currentBet_ == 0) {
            // ShowPlayerPass() is defined on BlackjackCardGame; invoked via
            // the callback installed by BlackjackCardGame::Initialize()
            // (see missing.md: sidesteps a forward-declaration cycle between
            // BetGameComponent and BlackjackCardGame).
            if (ShowPlayerPass) ShowPlayerPass(playerIndex);
        }
        static_cast<BlackjackPlayer*>(players_[playerIndex].get())->IsDoneBetting = true;
        currentChipComponents_.clear();
        currentBet_ = 0;
    }

public:
    // Set once by BlackjackCardGame::Initialize() -- see Bet_Click() comment.
    std::function<void(int)> ShowPlayerPass;

private:
    std::vector<std::shared_ptr<CardsFramework::Player>>& players_;
    std::string theme_;
    std::array<int, 4> assetNames_ = {5, 25, 100, 500};
    std::map<int, Texture2D> chipsAssets_;
    Texture2D blankChip_;
    std::vector<Vector2> positions_;
    CardsGame& cardGame_;
    std::optional<SpriteBatch> spriteBatch_;

    bool isKeyDown_ = false;

    std::shared_ptr<Button> bet_;
    std::shared_ptr<Button> clear_;

    static constexpr float insuranceYPosition_ = 120.0f;
    Vector2 secondHandOffset_ = Vector2(25.0f, 30.0f);

    std::vector<std::shared_ptr<AnimatedGameComponent>> currentChipComponents_;
    int currentBet_ = 0;
    InputState& input_;
    InputHelper* inputHelper_ = nullptr;
};

} // namespace Blackjack

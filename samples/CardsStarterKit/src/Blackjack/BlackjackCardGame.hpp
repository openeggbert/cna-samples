#pragma once

// BlackjackCardGame.hpp -- C++ port of Blackjack/Game/BlackjackCardGame.cs
// (XNA 4.0 CardsStarterKit sample). The core Blackjack game logic: betting,
// dealing, hit/stand/double/split/insurance, dealer AI, and round settlement.
//
// Forward-declaration note: BetGameComponent.hpp forward-declares
// BlackjackCardGame (it only needs it for `((BlackjackCardGame)cardGame).State`
// inside Update()), so BetGameComponent::Update() and the ShowPlayerPass
// callback wiring are both defined out-of-line at the bottom of this file,
// after BlackjackCardGame is a complete type -- same "define out-of-line
// once the dependency is complete" pattern already used throughout this
// port (see CardsGame.hpp, Hand.hpp).

#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "System/DateTime.hpp"

#include "../CardsFramework/CardsGame.hpp"
#include "../CardsFramework/FlipGameComponentAnimation.hpp"
#include "../CardsFramework/FramesetGameComponentAnimation.hpp"
#include "../CardsFramework/Hand.hpp"
#include "../CardsFramework/ScaleGameComponentAnimation.hpp"
#include "../CardsFramework/TraditionalCard.hpp"
#include "../CardsFramework/TransitionGameComponentAnimation.hpp"
#include "../GameStateManagement/ScreenManager.hpp"
#include "AudioManager.hpp"
#include "BetGameComponent.hpp"
#include "BlackJackTable.hpp"
#include "BlackjackAIPlayer.hpp"
#include "BlackjackCommon.hpp"
#include "BlackjackAnimatedDealerHandComponent.hpp"
#include "BlackjackAnimatedPlayerHandComponent.hpp"
#include "BlackjackGameEventArgs.hpp"
#include "BlackjackPlayer.hpp"
#include "BlackjackRule.hpp"
#include "Button.hpp"
#include "BustRule.hpp"
#include "InsuranceRule.hpp"

namespace Blackjack {

using CardsFramework::AnimatedCardsGameComponent;
using CardsFramework::AnimatedGameComponent;
using CardsFramework::AnimatedHandGameComponent;
using CardsFramework::CardsGame;
using CardsFramework::CardSuit;
using CardsFramework::CardValue;
using CardsFramework::FlipGameComponentAnimation;
using CardsFramework::GameRule;
using CardsFramework::Hand;
using CardsFramework::ScaleGameComponentAnimation;
using CardsFramework::TraditionalCard;
using CardsFramework::TransitionGameComponentAnimation;
using GameStateManagement::ScreenManager;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;

enum class BlackjackGameState {
    Shuffling,
    Betting,
    Playing,
    Dealing,
    RoundEnd,
    GameOver,
};

class BlackjackCardGame : public CardsGame {
public:
    BlackjackGameState State = BlackjackGameState::Shuffling;

    BlackjackCardGame(Rectangle tableBounds, Vector2 dealerPosition,
                      std::function<Vector2(int)> placeOrder, ScreenManager& screenManager,
                      const std::string& theme)
        : CardsGame(6, 0, CardSuit::AllSuits, CardValue::NonJokers, kMinPlayers, kMaxPlayers,
                    std::make_shared<BlackJackTable>(kRingOffset, tableBounds, dealerPosition, kMaxPlayers,
                                                     placeOrder, theme, screenManager.getGameProperty()),
                    theme, screenManager.getGameProperty()),
          screenManager_(&screenManager) {
        dealerPlayer_ = std::make_shared<BlackjackPlayer>("Dealer", this);
        turnFinishedByPlayer_.assign(MaximumPlayers, false);
        animatedHands_.assign(kMaxPlayers, nullptr);
        animatedSecondHands_.assign(kMaxPlayers, nullptr);
    }

    // Performs necessary initializations.
    void Initialize() {
        CardsGame::LoadContent();

        betGameComponent_ = AddComponent<BetGameComponent>(players, screenManager_->input, Theme, *this);
        betGameComponent_->ShowPlayerPass = [this](int playerIndex) { ShowPlayerPass(playerIndex); };

        Rectangle safeArea = screenManager_->SafeArea();
        static const std::string buttonsText[] = {"Hit", "Stand", "Double", "Split", "Insurance"};
        for (size_t buttonIndex = 0; buttonIndex < 5; buttonIndex++) {
            auto button = AddComponent<Button>("ButtonRegular", "ButtonPressed", screenManager_->input, *this);
            button->Text = buttonsText[buttonIndex];
            button->Bounds = Rectangle(safeArea.X + 10 + (int)buttonIndex * 110, safeArea.Y + safeArea.Height - 60, 100, 50);
            button->Font = &*Font;
            button->setVisibleProperty(false);
            button->setEnabledProperty(false);
            buttons_[buttonsText[buttonIndex]] = button;
        }

        newGame_ = AddComponent<Button>("ButtonRegular", "ButtonPressed", screenManager_->input, *this);
        newGame_->Text = std::string("New Hand");
        newGame_->Bounds = Rectangle(safeArea.X + 10, safeArea.Y + safeArea.Height - 60, 200, 50);
        newGame_->Font = &*Font;
        newGame_->setVisibleProperty(false);
        newGame_->setEnabledProperty(false);

        Rectangle insuranceBounds = buttons_["Insurance"]->Bounds;
        insuranceBounds.Width = 200;
        buttons_["Insurance"]->Bounds = insuranceBounds;

        newGame_->Click.Add([this](System::Object*, const System::EventArgs&) { NewGame_Click(); });
        buttons_["Hit"]->Click.Add([this](System::Object*, const System::EventArgs&) { Hit_Click(); });
        buttons_["Stand"]->Click.Add([this](System::Object*, const System::EventArgs&) { Stand_Click(); });
        buttons_["Double"]->Click.Add([this](System::Object*, const System::EventArgs&) { Double_Click(); });
        buttons_["Split"]->Click.Add([this](System::Object*, const System::EventArgs&) { Split_Click(); });
        buttons_["Insurance"]->Click.Add([this](System::Object*, const System::EventArgs&) { Insurance_Click(); });
    }

    // Performs the game's update logic.
    void Update(GameTime& gameTime) {
        FlushPendingReleases();

        switch (State) {
            case BlackjackGameState::Shuffling:
                ShowShuffleAnimation();
                break;
            case BlackjackGameState::Betting:
                EnableButtons(false);
                break;
            case BlackjackGameState::Dealing:
                State = BlackjackGameState::Playing;
                Deal();
                StartPlaying();
                break;
            case BlackjackGameState::Playing: {
                for (auto& p : players)
                    static_cast<BlackjackPlayer*>(p.get())->CalculateValues();
                dealerPlayer_->CalculateValues();

                if (!CheckForRunningAnimations<AnimatedCardsGameComponent>()) {
                    auto* player = static_cast<BlackjackPlayer*>(GetCurrentPlayer());
                    if (auto* aiPlayer = dynamic_cast<BlackjackAIPlayer*>(player))
                        aiPlayer->AIPlay();

                    CheckRules();

                    if (State == BlackjackGameState::Playing && GetCurrentPlayer() == nullptr)
                        EndRound();

                    SetButtonAvailability();
                } else {
                    EnableButtons(false);
                }
                break;
            }
            case BlackjackGameState::RoundEnd: {
                if (dealerHandComponent_->EstimatedTimeForAnimationsCompletion() == TimeSpan::Zero) {
                    betGameComponent_->CalculateBalance(*dealerPlayer_);
                    if (static_cast<BlackjackPlayer*>(players[0].get())->Balance < 5) {
                        EndGame();
                    } else {
                        newGame_->setEnabledProperty(true);
                        newGame_->setVisibleProperty(true);
                    }
                }
                break;
            }
            case BlackjackGameState::GameOver:
                break;
        }

        (void)gameTime;
    }

    // Renders the visual elements the game itself is responsible for.
    void Draw(const GameTime& gameTime) {
        (void)gameTime;
        CardsSpriteBatch->Begin();

        switch (State) {
            case BlackjackGameState::Playing:
                ShowPlayerValues();
                break;
            case BlackjackGameState::RoundEnd:
                if (dealerHandComponent_->EstimatedTimeForAnimationsCompletion() == TimeSpan::Zero)
                    ShowDealerValue();
                ShowPlayerValues();
                break;
            default:
                break;
        }

        CardsSpriteBatch->End();
    }

    // ---- CardsGame overrides ----

    void AddPlayer(std::shared_ptr<CardsFramework::Player> player) override {
        if (dynamic_cast<BlackjackPlayer*>(player.get()) && (int)players.size() < MaximumPlayers)
            players.push_back(player);
    }

    CardsFramework::Player* GetCurrentPlayer() override {
        for (size_t playerIndex = 0; playerIndex < players.size(); playerIndex++) {
            if (static_cast<BlackjackPlayer*>(players[playerIndex].get())->MadeBet() &&
                !turnFinishedByPlayer_[playerIndex])
                return players[playerIndex].get();
        }
        return nullptr;
    }

    // All card values are their face number, except jack/queen/king = 10;
    // an ace's value is 1 here -- game logic treats it as 11 where appropriate.
    int CardValue(const TraditionalCard& card) const override {
        return std::min(CardsGame::CardValue(card), 10);
    }

    // Deals 2 cards to each player (including the dealer) with staggered animations.
    void Deal() override {
        if (State != BlackjackGameState::Playing) return;

        for (int dealIndex = 0; dealIndex < 2; dealIndex++) {
            for (size_t playerIndex = 0; playerIndex < players.size(); playerIndex++) {
                auto* player = static_cast<BlackjackPlayer*>(players[playerIndex].get());
                if (player->MadeBet()) {
                    TraditionalCard& card = dealer_.DealCardToHand(player->PlayerHand);
                    AddDealAnimation(&card, animatedHands_[playerIndex].get(), true, dealDuration_,
                                     System::DateTime::getNowProperty() +
                                         TimeSpan::FromSeconds(dealDuration_.getTotalSecondsProperty() *
                                                               (dealIndex * (int)players.size() + (int)playerIndex)));
                }
            }
            TraditionalCard& dealerCard = dealer_.DealCardToHand(dealerPlayer_->PlayerHand);
            AddDealAnimation(&dealerCard, dealerHandComponent_.get(), dealIndex == 0, dealDuration_,
                             System::DateTime::getNowProperty());
        }
    }

    // Sets up rules/events and shows the participating hands.
    void StartPlaying() override {
        if (MinimumPlayers > (int)players.size() || (int)players.size() > MaximumPlayers)
            return;

        auto bustRule = std::make_unique<BustRule>(players);
        bustRule->RuleMatch.Add([this](System::Object*, const System::EventArgs& e) {
            BustGameRule(static_cast<const BlackjackGameEventArgs&>(e));
        });
        rules.push_back(std::move(bustRule));

        auto blackjackRule = std::make_unique<BlackJackRule>(players);
        blackjackRule->RuleMatch.Add([this](System::Object*, const System::EventArgs& e) {
            BlackJackGameRule(static_cast<const BlackjackGameEventArgs&>(e));
        });
        rules.push_back(std::move(blackjackRule));

        auto insuranceRule = std::make_unique<InsuranceRule>(dealerPlayer_->PlayerHand);
        insuranceRule->RuleMatch.Add([this](System::Object*, const System::EventArgs&) { InsuranceGameRule(); });
        rules.push_back(std::move(insuranceRule));

        for (size_t playerIndex = 0; playerIndex < players.size(); playerIndex++) {
            bool madeBet = static_cast<BlackjackPlayer*>(players[playerIndex].get())->MadeBet();
            animatedHands_[playerIndex]->setVisibleProperty(!madeBet);
        }
    }

    // ---- Player actions (called by button click handlers) ----

    void Hit() {
        auto* player = static_cast<BlackjackPlayer*>(GetCurrentPlayer());
        if (player == nullptr) return;

        int playerIndex = IndexOfPlayer(player);
        switch (player->CurrentHandType) {
            case HandTypes::First: {
                TraditionalCard& card = dealer_.DealCardToHand(player->PlayerHand);
                AddDealAnimation(&card, animatedHands_[playerIndex].get(), true, dealDuration_,
                                 System::DateTime::getNowProperty());
                break;
            }
            case HandTypes::Second: {
                TraditionalCard& card = dealer_.DealCardToHand(*player->SecondHand);
                AddDealAnimation(&card, animatedSecondHands_[playerIndex].get(), true, dealDuration_,
                                 System::DateTime::getNowProperty());
                break;
            }
        }
    }

    void Stand() {
        auto* player = static_cast<BlackjackPlayer*>(GetCurrentPlayer());
        if (player == nullptr) return;

        int playerIndex = IndexOfPlayer(player);
        if (!player->IsSplit) {
            turnFinishedByPlayer_[playerIndex] = true;
        } else {
            switch (player->CurrentHandType) {
                case HandTypes::First:
                    if (player->SecondBlackJack)
                        turnFinishedByPlayer_[playerIndex] = true;
                    else
                        player->CurrentHandType = HandTypes::Second;
                    break;
                case HandTypes::Second:
                    turnFinishedByPlayer_[playerIndex] = true;
                    break;
            }
        }
    }

    void Split() {
        auto* player = static_cast<BlackjackPlayer*>(GetCurrentPlayer());
        int playerIndex = IndexOfPlayer(player);

        player->InitializeSecondHand();

        Vector2 sourcePosition = animatedHands_[playerIndex]->GetCardGameComponent(1)->CurrentPosition;
        Vector2 targetPosition = animatedHands_[playerIndex]->GetCardGameComponent(0)->CurrentPosition + secondHandOffset_;

        auto animation = std::make_unique<TransitionGameComponentAnimation>(sourcePosition, targetPosition);
        animation->StartTime = System::DateTime::getNowProperty();
        animation->Duration = TimeSpan::FromSeconds(0.5);
        TimeSpan splitAnimTime = animation->EstimatedTimeForAnimationCompletion();

        player->SplitHand();

        betGameComponent_->AddChips(playerIndex, player->BetAmount, false, true);

        animatedSecondHands_[playerIndex] = AddComponent<BlackjackAnimatedPlayerHandComponent>(
            playerIndex, secondHandOffset_, *player->SecondHand, *this);

        auto animatedCard = animatedSecondHands_[playerIndex]->GetCardGameComponent(0);
        animatedCard->IsFaceDown = false;
        animatedCard->AddAnimation(std::move(animation));

        TraditionalCard& card = dealer_.DealCardToHand(player->PlayerHand);
        AddDealAnimation(&card, animatedHands_[playerIndex].get(), true, dealDuration_,
                         System::DateTime::getNowProperty() + splitAnimTime);
        TraditionalCard& card2 = dealer_.DealCardToHand(*player->SecondHand);
        AddDealAnimation(&card2, animatedSecondHands_[playerIndex].get(), true, dealDuration_,
                         System::DateTime::getNowProperty() + splitAnimTime + dealDuration_);
    }

    void Double() {
        auto* player = static_cast<BlackjackPlayer*>(GetCurrentPlayer());
        int playerIndex = IndexOfPlayer(player);

        switch (player->CurrentHandType) {
            case HandTypes::First: {
                player->Double = true;
                float betAmount = player->BetAmount;
                if (player->IsSplit) betAmount /= 2.0f;
                betGameComponent_->AddChips(playerIndex, betAmount, false, false);
                break;
            }
            case HandTypes::Second: {
                player->SecondDouble = true;
                if (!player->Double)
                    betGameComponent_->AddChips(playerIndex, player->BetAmount / 2.0f, false, true);
                else
                    betGameComponent_->AddChips(playerIndex, player->BetAmount / 3.0f, false, true);
                break;
            }
        }
        Hit();
        Stand();
    }

    // Displays every currently-in-play hand, then starts shuffling/dealing.
    void StartRound() {
        playerHandValueTexts_.clear();
        AudioManager::PlaySound("Shuffle");
        dealer_.Shuffle();
        DisplayPlayingHands();
        State = BlackjackGameState::Shuffling;
    }

    // Adds a visual "passed" cue over a player's hand.
    void ShowPlayerPass(int indexPlayer) {
        auto passComponent = AddComponent<AnimatedGameComponent>(*this, &CardsAssets.at("pass"));
        passComponent->CurrentPosition = (*GameTableInstance)[indexPlayer];
        passComponent->setVisibleProperty(false);

        std::function<void(void*)> performWhenDone;
        if (indexPlayer == 0)
            performWhenDone = [this](void*) { showInsurance_ = false; };

        auto scale = std::make_unique<ScaleGameComponentAnimation>(2.0f, 1.0f);
        scale->setAnimationCycles(1);
        AnimatedGameComponent* raw = passComponent.get();
        scale->PerformBeforeStart = [raw](void*) { raw->setVisibleProperty(true); };
        scale->StartTime = System::DateTime::getNowProperty();
        scale->Duration = TimeSpan::FromSeconds(1.0);
        scale->PerformWhenDone = performWhenDone;
        passComponent->AddAnimation(std::move(scale));
    }

    // Checks whether a running animation of the given component type exists.
    template <typename T>
    bool CheckForRunningAnimations() {
        for (auto* component : GameInstance->getComponentsProperty()) {
            if (auto* animated = dynamic_cast<T*>(component)) {
                if (animated->IsAnimating())
                    return true;
            }
        }
        return false;
    }

private:
    static constexpr int kMaxPlayers = 3;
    static constexpr int kMinPlayers = 1;
    static inline const Vector2 kRingOffset = Vector2(0.0f, 110.0f);

    void ShowShuffleAnimation() {
        auto animationComponent = AddComponent<AnimatedGameComponent>(*GameInstance);
        animationComponent->CurrentPosition = GameTableInstance->DealerPosition;
        animationComponent->setVisibleProperty(false);

        auto anim = std::make_unique<CardsFramework::FramesetGameComponentAnimation>(
            &CardsAssets.at("Shuffle_" + Theme), 32, 11, frameSize_);
        anim->Duration = TimeSpan::FromSeconds(1.5);
        AnimatedGameComponent* raw = animationComponent.get();
        anim->PerformBeforeStart = [raw](void*) { raw->setVisibleProperty(true); };
        anim->PerformWhenDone = [this, raw](void*) {
            AudioManager::PlaySound("Shuffle");
            RemoveComponentByRaw(raw);
        };
        animationComponent->AddAnimation(std::move(anim));

        State = BlackjackGameState::Betting;
    }

    void ShowDealerValue() {
        std::string dealerValue = std::to_string(dealerPlayer_->FirstValue());
        if (dealerPlayer_->FirstValueConsiderAce()) {
            if (dealerPlayer_->FirstValue() + 10 == 21)
                dealerValue = "21";
            else
                dealerValue += "\\" + std::to_string(dealerPlayer_->FirstValue() + 10);
        }

        Vector2 measure = Font->MeasureString(dealerValue);
        Vector2 position = GameTableInstance->DealerPosition - Vector2(measure.X + 20.0f, 0.0f);

        CardsSpriteBatch->Draw(screenManager_->getBlankTexture(),
                               Rectangle((int)position.X - 4, (int)position.Y, (int)measure.X + 8, (int)measure.Y),
                               Color::Black);
        CardsSpriteBatch->DrawString(*Font, dealerValue, position, Color::White);
    }

    void ShowPlayerValues() {
        CardsFramework::Player* currentPlayer = GetCurrentPlayer();

        for (size_t playerIndex = 0; playerIndex < players.size(); playerIndex++) {
            auto* player = static_cast<BlackjackPlayer*>(players[playerIndex].get());
            Color color = (player == currentPlayer) ? Color::Red : Color::White;

            std::optional<std::string> playerHandValueText;
            std::optional<std::string> playerSecondHandValueText;

            if (!animatedHands_[playerIndex]->IsAnimating()) {
                if (player->FirstValue() > 0) {
                    std::string text = std::to_string(player->FirstValue());
                    if (player->FirstValueConsiderAce()) {
                        text = (player->FirstValue() + 10 == 21) ? "21"
                                                                  : text + "\\" + std::to_string(player->FirstValue() + 10);
                    }
                    playerHandValueText = text;
                    playerHandValueTexts_[player] = text;
                }

                if (player->IsSplit && player->SecondValue() > 0) {
                    std::string text = std::to_string(player->SecondValue());
                    if (player->SecondValueConsiderAce()) {
                        text = (player->SecondValue() + 10 == 21) ? "21"
                                                                   : text + "\\" + std::to_string(player->SecondValue() + 10);
                    }
                    playerSecondHandValueText = text;
                    playerSecondHandValueTexts_[player] = text;
                }
            } else {
                auto it = playerHandValueTexts_.find(player);
                if (it != playerHandValueTexts_.end()) playerHandValueText = it->second;
                auto it2 = playerSecondHandValueTexts_.find(player);
                if (it2 != playerSecondHandValueTexts_.end()) playerSecondHandValueText = it2->second;
            }

            if (player->IsSplit) {
                color = (player->CurrentHandType == HandTypes::First && player == currentPlayer) ? Color::Red : Color::White;
                if (playerHandValueText.has_value())
                    DrawValue(*animatedHands_[playerIndex], (int)playerIndex, *playerHandValueText, color);

                color = (player->CurrentHandType == HandTypes::Second && player == currentPlayer) ? Color::Red : Color::White;
                if (playerSecondHandValueText.has_value())
                    DrawValue(*animatedSecondHands_[playerIndex], (int)playerIndex, *playerSecondHandValueText, color);
            } else if (playerHandValueText.has_value()) {
                DrawValue(*animatedHands_[playerIndex], (int)playerIndex, *playerHandValueText, color);
            }
        }
    }

    void DrawValue(AnimatedHandGameComponent& animatedHand, int place, const std::string& value, Color valueColor) {
        Hand& hand = animatedHand.HandRef;

        Vector2 position = (*GameTableInstance).PlaceOrder(place) + animatedHand.GetCardRelativePosition(hand.Count() - 1);
        Vector2 measure = Font->MeasureString(value);

        position.X += (CardsAssets.at("CardBack_" + Theme).getBoundsProperty().Width - measure.X) / 2.0f;
        position.Y -= measure.Y + 5.0f;

        CardsSpriteBatch->Draw(screenManager_->getBlankTexture(),
                               Rectangle((int)position.X - 4, (int)position.Y, (int)measure.X + 8, (int)measure.Y),
                               Color::Black);
        CardsSpriteBatch->DrawString(*Font, value, position, valueColor);
    }

    void AddDealAnimation(TraditionalCard* card, AnimatedHandGameComponent* animatedHand, bool flipCard,
                          TimeSpan duration, System::DateTime startTime) {
        int cardLocationInHand = animatedHand->GetCardLocationInHand(card);
        auto cardComponent = animatedHand->GetCardGameComponent(cardLocationInHand);

        auto transition = std::make_unique<TransitionGameComponentAnimation>(
            GameTableInstance->DealerPosition,
            animatedHand->CurrentPosition + animatedHand->GetCardRelativePosition(cardLocationInHand));
        transition->StartTime = startTime;
        AnimatedGameComponent* rawCard = cardComponent.get();
        transition->PerformBeforeStart = [rawCard](void*) { rawCard->setVisibleProperty(true); };
        transition->PerformWhenDone = [](void*) { AudioManager::PlaySound("Deal"); };
        cardComponent->AddAnimation(std::move(transition));

        if (flipCard) {
            auto flip = std::make_unique<FlipGameComponentAnimation>();
            flip->IsFromFaceDownToFaceUp = true;
            flip->Duration = duration;
            flip->StartTime = startTime + duration;
            flip->PerformWhenDone = [](void*) { AudioManager::PlaySound("Flip"); };
            cardComponent->AddAnimation(std::move(flip));
        }
    }

    void CueOverPlayerHand(BlackjackPlayer* player, const std::string& assetName, HandTypes animationHand,
                           AnimatedHandGameComponent* waitForHand) {
        int playerIndex = IndexOfPlayer(player);
        AnimatedHandGameComponent* currentAnimatedHand;
        Vector2 currentPosition;

        if (playerIndex >= 0) {
            switch (animationHand) {
                case HandTypes::First:
                    currentAnimatedHand = animatedHands_[playerIndex].get();
                    currentPosition = currentAnimatedHand->CurrentPosition;
                    break;
                case HandTypes::Second:
                    currentAnimatedHand = animatedSecondHands_[playerIndex].get();
                    currentPosition = currentAnimatedHand->CurrentPosition + secondHandOffset_;
                    break;
                default:
                    throw std::runtime_error("Player has an unsupported hand type.");
            }
        } else {
            currentAnimatedHand = dealerHandComponent_.get();
            currentPosition = currentAnimatedHand->CurrentPosition;
        }

        auto animationComponent = AddComponent<AnimatedGameComponent>(*this, &CardsAssets.at(assetName));
        animationComponent->CurrentPosition = currentPosition;
        animationComponent->setVisibleProperty(false);

        TimeSpan estimatedTime = waitForHand ? waitForHand->EstimatedTimeForAnimationsCompletion()
                                             : currentAnimatedHand->EstimatedTimeForAnimationsCompletion();

        auto scale = std::make_unique<ScaleGameComponentAnimation>(2.0f, 1.0f);
        scale->StartTime = System::DateTime::getNowProperty() + estimatedTime;
        scale->Duration = TimeSpan::FromSeconds(1.0);
        AnimatedGameComponent* raw = animationComponent.get();
        scale->PerformBeforeStart = [raw](void*) { raw->setVisibleProperty(true); };
        animationComponent->AddAnimation(std::move(scale));
    }

    void EndRound() {
        RevealDealerFirstCard();
        DealerAI();
        ShowResults();
        State = BlackjackGameState::RoundEnd;
    }

    void ShowDealerHand() {
        dealerHandComponent_ = AddComponent<BlackjackAnimatedDealerHandComponent>(-1, dealerPlayer_->PlayerHand, *this);
    }

    void RevealDealerFirstCard() {
        auto cardComponent = dealerHandComponent_->GetCardGameComponent(1);
        auto flip = std::make_unique<FlipGameComponentAnimation>();
        flip->Duration = TimeSpan::FromSeconds(0.5);
        flip->StartTime = System::DateTime::getNowProperty();
        cardComponent->AddAnimation(std::move(flip));
    }

    void ShowResults() {
        int dealerValue = dealerPlayer_->FirstValue();
        if (dealerPlayer_->FirstValueConsiderAce())
            dealerValue += 10;

        for (auto& p : players) {
            auto* player = static_cast<BlackjackPlayer*>(p.get());
            ShowResultForPlayer(player, dealerValue, HandTypes::First);
            if (player->IsSplit)
                ShowResultForPlayer(player, dealerValue, HandTypes::Second);
        }
    }

    void ShowResultForPlayer(BlackjackPlayer* player, int dealerValue, HandTypes currentHandType) {
        bool blackjack, bust;
        int playerValue;

        switch (currentHandType) {
            case HandTypes::First:
                blackjack = player->BlackJack;
                bust = player->Bust;
                playerValue = player->FirstValue();
                if (player->FirstValueConsiderAce()) playerValue += 10;
                break;
            case HandTypes::Second:
                blackjack = player->SecondBlackJack;
                bust = player->SecondBust;
                playerValue = player->SecondValue();
                if (player->SecondValueConsiderAce()) playerValue += 10;
                break;
            default:
                throw std::runtime_error("Player has an unsupported hand type.");
        }

        if (player->MadeBet() && (!blackjack || (dealerPlayer_->BlackJack && blackjack)) && !bust) {
            std::string assetName = GetResultAsset(player, dealerValue, playerValue);
            CueOverPlayerHand(player, assetName, currentHandType, dealerHandComponent_.get());
        }
    }

    // Note: matches the original exactly -- it reads player->BlackJack (the
    // *first*-hand flag) here even when called for the second hand, rather
    // than the hand-specific blackjack/bust booleans ShowResultForPlayer
    // already computed. Faithfully reproduced, not "fixed" -- see missing.md.
    std::string GetResultAsset(BlackjackPlayer* player, int dealerValue, int playerValue) const {
        if (dealerPlayer_->Bust) return "win";
        if (dealerPlayer_->BlackJack) return player->BlackJack ? "push" : "lose";
        if (playerValue < dealerValue) return "lose";
        if (playerValue > dealerValue) return "win";
        return "push";
    }

    void DealerAI() {
        dealerPlayer_->CalculateValues();
        int dealerValue = dealerPlayer_->FirstValue();
        if (dealerPlayer_->FirstValueConsiderAce()) dealerValue += 10;

        if (dealerValue > 21) {
            dealerPlayer_->Bust = true;
            CueOverPlayerHand(dealerPlayer_.get(), "bust", HandTypes::First, dealerHandComponent_.get());
        } else if (dealerValue == 21) {
            dealerPlayer_->BlackJack = true;
            CueOverPlayerHand(dealerPlayer_.get(), "blackjack", HandTypes::First, dealerHandComponent_.get());
        }

        if (dealerPlayer_->BlackJack || dealerPlayer_->Bust) return;

        int cardsDealt = 0;
        while (dealerValue <= 17) {
            TraditionalCard& card = dealer_.DealCardToHand(dealerPlayer_->PlayerHand);
            AddDealAnimation(&card, dealerHandComponent_.get(), true, dealDuration_,
                             System::DateTime::getNowProperty() + TimeSpan::FromMilliseconds(1000 * (cardsDealt + 1)));
            cardsDealt++;
            dealerPlayer_->CalculateValues();
            dealerValue = dealerPlayer_->FirstValue();
            if (dealerPlayer_->FirstValueConsiderAce()) dealerValue += 10;

            if (dealerValue > 21) {
                dealerPlayer_->Bust = true;
                CueOverPlayerHand(dealerPlayer_.get(), "bust", HandTypes::First, dealerHandComponent_.get());
            }
        }
    }

    void DisplayPlayingHands() {
        for (size_t playerIndex = 0; playerIndex < players.size(); playerIndex++) {
            animatedHands_[playerIndex] = AddComponent<BlackjackAnimatedPlayerHandComponent>(
                (int)playerIndex, players[playerIndex]->PlayerHand, *this);
        }
        ShowDealerHand();
    }

    void SetButtonAvailability() {
        auto* player = static_cast<BlackjackPlayer*>(GetCurrentPlayer());
        if (player == nullptr || dynamic_cast<BlackjackAIPlayer*>(player)) {
            EnableButtons(false);
            ChangeButtonsVisibility(false);
            return;
        }

        EnableButtons(true);
        ChangeButtonsVisibility(true);

        buttons_["Insurance"]->setVisibleProperty(showInsurance_);
        buttons_["Insurance"]->setEnabledProperty(showInsurance_);

        if (!player->IsSplit) {
            if (player->BetAmount > player->Balance || player->PlayerHand.Count() != 2) {
                buttons_["Double"]->setVisibleProperty(false);
                buttons_["Double"]->setEnabledProperty(false);
            }
            if (player->PlayerHand.Count() != 2 || player->PlayerHand[0].Value != player->PlayerHand[1].Value ||
                player->BetAmount > player->Balance) {
                buttons_["Split"]->setVisibleProperty(false);
                buttons_["Split"]->setEnabledProperty(false);
            }
        } else {
            float initialBet = player->BetAmount / ((player->Double ? 2.0f : 1.0f) + (player->SecondDouble ? 2.0f : 1.0f));
            if (initialBet > player->Balance || player->CurrentHand().Count() != 2) {
                buttons_["Double"]->setVisibleProperty(false);
                buttons_["Double"]->setEnabledProperty(false);
            }
            buttons_["Split"]->setVisibleProperty(false);
            buttons_["Split"]->setEnabledProperty(false);
        }
    }

    void EndGame() {
        long long estimatedTicks = 0;
        for (auto* component : GameInstance->getComponentsProperty()) {
            if (auto* animated = dynamic_cast<AnimatedGameComponent*>(component)) {
                long long ticks = animated->EstimatedTimeForAnimationsCompletion().getTicksProperty();
                if (ticks > estimatedTicks) estimatedTicks = ticks;
            }
        }

        Texture2D* texture = &CardsAssets.emplace("youLose", GameInstance->getContentProperty().Load<Texture2D>("Images/youLose"))
                                  .first->second;

        auto animationComponent = AddComponent<AnimatedGameComponent>(*this, texture);
        auto& viewport = GameInstance->getGraphicsDeviceProperty().getViewportProperty();
        animationComponent->CurrentPosition = Vector2(
            (float)(viewport.getBoundsProperty().Width / 2 - texture->getWidthProperty() / 2),
            (float)(viewport.getBoundsProperty().Height / 2 - texture->getHeightProperty() / 2));
        animationComponent->setVisibleProperty(false);

        Rectangle bounds = viewport.getBoundsProperty();
        Vector2 center((float)(bounds.X + bounds.Width / 2), (float)(bounds.Y + bounds.Height / 2));
        auto backButton = AddComponent<Button>("ButtonRegular", "ButtonPressed", screenManager_->input, *this);
        backButton->Bounds = Rectangle((int)center.X - 100, (int)center.Y + 80, 200, 50);
        backButton->Font = &*Font;
        backButton->Text = std::string("Main Menu");
        backButton->setVisibleProperty(false);
        backButton->setEnabledProperty(true);
        backButton->Click.Add([this](System::Object*, const System::EventArgs&) { BackButton_Click(); });

        AnimatedGameComponent* rawAnim = animationComponent.get();
        std::shared_ptr<Button> backButtonPtr = backButton;
        auto stall = std::make_unique<CardsFramework::AnimatedGameComponentAnimation>();
        stall->Duration = TimeSpan::FromTicks(estimatedTicks) + TimeSpan::FromSeconds(1);
        stall->PerformWhenDone = [this, rawAnim, backButtonPtr](void*) {
            State = BlackjackGameState::GameOver;
            rawAnim->setVisibleProperty(true);
            backButtonPtr->setVisibleProperty(true);
            ResetGame(rawAnim, backButtonPtr.get());
        };
        animationComponent->AddAnimation(std::move(stall));
    }

    // Removes every gameplay component except the two just revealed.
    void ResetGame(AnimatedGameComponent* keepAnim, Button* keepButton) {
        std::vector<IGameComponent*> toRemove;
        for (auto* component : GameInstance->getComponentsProperty()) {
            if (component != keepAnim && component != keepButton &&
                (dynamic_cast<BetGameComponent*>(component) || dynamic_cast<AnimatedGameComponent*>(component) ||
                 dynamic_cast<Button*>(component))) {
                toRemove.push_back(component);
            }
        }
        for (auto* c : toRemove)
            RemoveComponentByRaw(c);
    }

    // Removes stale gameplay components between rounds and resets state.
    void FinishTurn() {
        std::vector<IGameComponent*> snapshot;
        for (auto* c : GameInstance->getComponentsProperty())
            snapshot.push_back(c);

        for (auto* component : snapshot) {
            // Original also excludes `is BlackjackCardGame`, but CardsGame
            // (unlike XNA's) is never itself a Game.Components entry in this
            // port or the original -- that check can never match, so it's
            // dropped here rather than ported as dead code.
            bool keep = dynamic_cast<CardsFramework::GameTable*>(component) ||
                       dynamic_cast<BetGameComponent*>(component) || dynamic_cast<Button*>(component) ||
                       dynamic_cast<ScreenManager*>(component) || dynamic_cast<InputHelper*>(component);
            if (keep) continue;

            if (auto* animatedCard = dynamic_cast<AnimatedCardsGameComponent*>(component)) {
                auto transition = std::make_unique<TransitionGameComponentAnimation>(
                    animatedCard->CurrentPosition,
                    Vector2(animatedCard->CurrentPosition.X,
                           (float)GameInstance->getGraphicsDeviceProperty().getViewportProperty().getHeightProperty()));
                transition->Duration = TimeSpan::FromSeconds(0.40);
                IGameComponent* raw = component;
                transition->PerformWhenDone = [this, raw](void*) { RemoveComponentByRaw(raw); };
                animatedCard->AddAnimation(std::move(transition));
            } else {
                RemoveComponentByRaw(component);
            }
        }

        for (size_t playerIndex = 0; playerIndex < players.size(); playerIndex++) {
            auto* player = static_cast<BlackjackPlayer*>(players[playerIndex].get());
            player->ResetValues();
            player->PlayerHand.DealCardsToHand(deadCards_, player->PlayerHand.Count());
            turnFinishedByPlayer_[playerIndex] = false;
            animatedHands_[playerIndex] = nullptr;
            animatedSecondHands_[playerIndex] = nullptr;
        }

        betGameComponent_->Reset();
        betGameComponent_->setEnabledProperty(true);

        dealerPlayer_->PlayerHand.DealCardsToHand(deadCards_, dealerPlayer_->PlayerHand.Count());
        dealerPlayer_->ResetValues();

        rules.clear();
    }

    void InsuranceGameRule() {
        auto* player = static_cast<BlackjackPlayer*>(players[0].get());
        if (player->Balance >= player->BetAmount / 2.0f)
            showInsurance_ = true;
    }

    void BustGameRule(const BlackjackGameEventArgs& args) {
        showInsurance_ = false;
        auto* player = static_cast<BlackjackPlayer*>(args.Player);

        CueOverPlayerHand(player, "bust", args.Hand, nullptr);

        switch (args.Hand) {
            case HandTypes::First:
                player->Bust = true;
                if (player->IsSplit && !player->SecondBlackJack)
                    player->CurrentHandType = HandTypes::Second;
                else
                    turnFinishedByPlayer_[IndexOfPlayer(player)] = true;
                break;
            case HandTypes::Second:
                player->SecondBust = true;
                turnFinishedByPlayer_[IndexOfPlayer(player)] = true;
                break;
        }
    }

    void BlackJackGameRule(const BlackjackGameEventArgs& args) {
        showInsurance_ = false;
        auto* player = static_cast<BlackjackPlayer*>(args.Player);

        CueOverPlayerHand(player, "blackjack", args.Hand, nullptr);

        switch (args.Hand) {
            case HandTypes::First:
                player->BlackJack = true;
                if (player->IsSplit)
                    player->CurrentHandType = HandTypes::Second;
                else
                    turnFinishedByPlayer_[IndexOfPlayer(player)] = true;
                break;
            case HandTypes::Second:
                player->SecondBlackJack = true;
                if (player->CurrentHandType == HandTypes::Second)
                    turnFinishedByPlayer_[IndexOfPlayer(player)] = true;
                break;
        }
    }

    void Insurance_Click() {
        auto* player = static_cast<BlackjackPlayer*>(GetCurrentPlayer());
        if (player == nullptr) return;
        player->IsInsurance = true;
        player->Balance -= player->BetAmount / 2.0f;
        betGameComponent_->AddChips(IndexOfPlayer(player), player->BetAmount / 2.0f, true, false);
        showInsurance_ = false;
    }

    void NewGame_Click() {
        FinishTurn();
        StartRound();
        newGame_->setEnabledProperty(false);
        newGame_->setVisibleProperty(false);
    }

    void Hit_Click() { Hit(); showInsurance_ = false; }
    void Stand_Click() { Stand(); showInsurance_ = false; }
    void Double_Click() { Double(); showInsurance_ = false; }
    void Split_Click() { Split(); showInsurance_ = false; }

    // Defined out-of-line at the bottom of Screens/GameplayScreen.hpp: needs
    // BackgroundScreen/MainMenuScreen, which themselves need GameplayScreen,
    // which needs BlackjackCardGame -- a genuine three-way cycle already
    // present in the original C# (trivial there; C# has no header/forward-
    // declaration ordering to satisfy). See missing.md.
    void BackButton_Click();

    void ChangeButtonsVisibility(bool visible) {
        buttons_["Hit"]->setVisibleProperty(visible);
        buttons_["Stand"]->setVisibleProperty(visible);
        buttons_["Double"]->setVisibleProperty(visible);
        buttons_["Split"]->setVisibleProperty(visible);
        buttons_["Insurance"]->setVisibleProperty(visible);
    }

    void EnableButtons(bool enabled) {
        buttons_["Hit"]->setEnabledProperty(enabled);
        buttons_["Stand"]->setEnabledProperty(enabled);
        buttons_["Double"]->setEnabledProperty(enabled);
        buttons_["Split"]->setEnabledProperty(enabled);
        buttons_["Insurance"]->setEnabledProperty(enabled);
    }

    int IndexOfPlayer(CardsFramework::Player* player) const {
        for (size_t i = 0; i < players.size(); i++)
            if (players[i].get() == player) return (int)i;
        return -1;
    }

    std::map<CardsFramework::Player*, std::string> playerHandValueTexts_;
    std::map<CardsFramework::Player*, std::string> playerSecondHandValueTexts_;
    Hand deadCards_;
    std::shared_ptr<BlackjackPlayer> dealerPlayer_;
    std::vector<bool> turnFinishedByPlayer_;
    TimeSpan dealDuration_ = TimeSpan::FromMilliseconds(500);

    std::vector<std::shared_ptr<AnimatedHandGameComponent>> animatedHands_;
    std::vector<std::shared_ptr<AnimatedHandGameComponent>> animatedSecondHands_;

    std::shared_ptr<BetGameComponent> betGameComponent_;
    std::shared_ptr<AnimatedHandGameComponent> dealerHandComponent_;
    std::map<std::string, std::shared_ptr<Button>> buttons_;
    std::shared_ptr<Button> newGame_;
    bool showInsurance_ = false;

    Vector2 secondHandOffset_ = Vector2(100.0f, 25.0f);
    Vector2 frameSize_ = Vector2(180.0f, 180.0f);

    ScreenManager* screenManager_;
};

// ---- BetGameComponent::Update (needs BlackjackCardGame's full definition) ----

inline void BetGameComponent::Update(GameTime& gameTime) {
    if (!players_.empty()) {
        auto* lastPlayer = static_cast<BlackjackPlayer*>(players_.back().get());
        auto& blackjackGame = static_cast<BlackjackCardGame&>(cardGame_);

        if (blackjackGame.State == BlackjackGameState::Betting && !lastPlayer->IsDoneBetting) {
            int playerIndex = GetCurrentPlayer();
            auto* player = static_cast<BlackjackPlayer*>(players_[playerIndex].get());

            if (auto* aiPlayer = dynamic_cast<BlackjackAIPlayer*>(player)) {
                ShowAndEnableButtons(false);
                int bet = aiPlayer->AIBet();
                if (bet == 0)
                    Bet_Click();
                else
                    AddChip(playerIndex, bet, false);
            } else {
                ShowAndEnableButtons(true);
                HandleInput(Mouse::GetState());
            }
        }

        if (lastPlayer->IsDoneBetting) {
            if (!blackjackGame.CheckForRunningAnimations<AnimatedGameComponent>()) {
                ShowAndEnableButtons(false);
                blackjackGame.State = BlackjackGameState::Dealing;
                setEnabledProperty(false);
            }
        }
    }

    DrawableGameComponent::Update(gameTime);
}

} // namespace Blackjack

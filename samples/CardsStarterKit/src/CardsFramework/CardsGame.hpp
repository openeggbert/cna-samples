#pragma once

// CardsGame.hpp -- C++ port of Game/CardsGame.cs (XNA 4.0 CardsStarterKit
// sample). A cards-game handler: derive from this class to implement a
// specific card game (see Blackjack/BlackjackCardGame.hpp).
//
// Ownership note (see samples/CardsStarterKit/missing.md): XNA's
// Game.Components collection holds GC-reachable references; CNA's
// GameComponentCollection (like the real XNA one) stores only non-owning
// IGameComponent* pointers. Every AnimatedGameComponent this game creates
// dynamically (dealt cards, chips, cue banners, per-round hands) therefore
// needs an explicit owner. CardsGame provides AddComponent<T>()/
// RemoveComponent() for this, following the same "defer actual destruction
// to the start of the next frame" pattern already established for
// HoneycombRush/NinjAcademy's ScreenManager (see NEXT.md's "pure virtual
// method called" pattern-to-watch-for): destroying a component while
// Game::Update()/Draw() is mid-iteration over its Components snapshot is
// undefined behavior, so removed components are kept alive in
// pendingRelease_ until FlushPendingReleases() is called at the top of the
// next frame (BlackjackCardGame::Update() does this).

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/IGameComponent.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteFont.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "AnimatedCardsGameComponent.hpp"
#include "AnimatedGameComponent.hpp"
#include "CardPacket.hpp"
#include "GameRule.hpp"
#include "GameTable.hpp"
#include "Player.hpp"
#include "TraditionalCard.hpp"
#include "UIUtility.hpp"

namespace CardsFramework {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::IGameComponent;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::SpriteFont;
using Microsoft::Xna::Framework::Graphics::Texture2D;

// A cards-game handler. Use a singleton of a class that derives from this to
// empower a cards-game, calling the various methods in order to allow the
// implementing instance to run the game.
class CardsGame {
public:
    int MinimumPlayers;
    int MaximumPlayers;

    std::string Theme;
    // `protected internal Dictionary<string, Texture2D> cardsAssets` in the
    // original -- kept public here since Button/BetGameComponent/animated
    // components in the Blackjack namespace need direct access, same as the
    // original's `internal` visibility allowed within the same assembly.
    std::unordered_map<std::string, Texture2D> CardsAssets;
    std::shared_ptr<GameTable> GameTableInstance;
    std::optional<SpriteFont> Font;
    std::optional<SpriteBatch> CardsSpriteBatch;
    // Renamed from the original's `Game` property -- `Game` is also this
    // project's CNA type name, and `CardsGame::Game` would shadow it.
    Game* GameInstance = nullptr;

    CardsGame(int decks, int jokersInDeck, CardSuit suits, CardValue cardValues, int minimumPlayers,
              int maximumPlayers, std::shared_ptr<GameTable> gameTable, const std::string& theme, Game& game)
        : MinimumPlayers(minimumPlayers), MaximumPlayers(maximumPlayers), Theme(theme),
          GameTableInstance(std::move(gameTable)), GameInstance(&game),
          dealer_(decks, jokersInDeck, suits, cardValues) {
        GameTableInstance->setDrawOrderProperty(-10000);
        game.getComponentsProperty().Add(GameTableInstance.get());
    }

    virtual ~CardsGame() = default;

    // Checks which of the game's rules need to be fired.
    virtual void CheckRules() {
        for (auto& rule : rules)
            rule->Check();
    }

    // Returns a card's value in the scope of the game.
    virtual int CardValue(const TraditionalCard& card) const {
        switch (card.Value) {
            case CardValue::Ace: return 1;
            case CardValue::Two: return 2;
            case CardValue::Three: return 3;
            case CardValue::Four: return 4;
            case CardValue::Five: return 5;
            case CardValue::Six: return 6;
            case CardValue::Seven: return 7;
            case CardValue::Eight: return 8;
            case CardValue::Nine: return 9;
            case CardValue::Ten: return 10;
            case CardValue::Jack: return 11;
            case CardValue::Queen: return 12;
            case CardValue::King: return 13;
            default: throw std::invalid_argument("Ambiguous card value");
        }
    }

    // Adds a player to the game.
    virtual void AddPlayer(std::shared_ptr<Player> player) = 0;

    // Gets the player who is currently taking his turn.
    virtual Player* GetCurrentPlayer() = 0;

    // Deals cards to the participating players.
    virtual void Deal() = 0;

    // Initializes the game and lets the players start playing.
    virtual void StartPlaying() = 0;

    // Loads the basic contents for a card game (the card assets).
    void LoadContent() {
        CardsSpriteBatch.emplace(GameInstance->getGraphicsDeviceProperty());

        // Initialize a full deck to enumerate every asset name to load.
        CardPacket fullDeck(1, 2, CardSuit::AllSuits, CardValue::NonJokers | CardValue::Jokers);
        for (int cardIndex = 0; cardIndex < 54; cardIndex++) {
            LoadUITexture("Cards", UIUtility::GetCardAssetName(fullDeck[cardIndex]));
        }
        LoadUITexture("Cards", "CardBack_" + Theme);

        Font = GameInstance->getContentProperty().Load<SpriteFont>("Fonts/Regular");

        GameTableInstance->Initialize();
    }

    // Loads a UI texture for the game, taking the theme into account.
    void LoadUITexture(const std::string& folder, const std::string& assetName) {
        CardsAssets.emplace(assetName,
                            GameInstance->getContentProperty().Load<Texture2D>("Images/" + folder + "/" + assetName));
    }

    // ---- Component ownership helpers (see file header comment) ----

    template <typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args) {
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);
        GameInstance->getComponentsProperty().Add(ptr.get());
        ownedComponents_.push_back(ptr);
        return ptr;
    }

    void RemoveComponent(const std::shared_ptr<IGameComponent>& comp) {
        if (comp) RemoveComponentByRaw(comp.get());
    }

    void RemoveComponentByRaw(IGameComponent* raw) {
        if (!raw) return;
        (void)GameInstance->getComponentsProperty().Remove(raw);
        for (size_t i = 0; i < ownedComponents_.size(); i++) {
            if (ownedComponents_[i].get() == raw) {
                pendingRelease_.push_back(std::move(ownedComponents_[i]));
                ownedComponents_.erase(ownedComponents_.begin() + i);
                break;
            }
        }
    }

    // Releases components removed since the last call. Must be called once
    // per frame, before iterating/mutating components further (see file
    // header comment) -- BlackjackCardGame::Update() does this first thing.
    void FlushPendingReleases() { pendingRelease_.clear(); }

protected:
    std::vector<std::unique_ptr<GameRule>> rules;
    std::vector<std::shared_ptr<Player>> players;
    CardPacket dealer_;

private:
    std::vector<std::shared_ptr<IGameComponent>> ownedComponents_;
    std::vector<std::shared_ptr<IGameComponent>> pendingRelease_;
};

// ---- Out-of-line definitions needing CardsGame's full layout ----

inline AnimatedGameComponent::AnimatedGameComponent(CardsGame& cardGame, Texture2D* currentFrame)
    : DrawableGameComponent(*cardGame.GameInstance) {
    CardGame = &cardGame;
    CurrentFrame = currentFrame;
}

inline void AnimatedGameComponent::Draw(const GameTime& gameTime) {
    DrawableGameComponent::Draw(gameTime);

    SpriteBatch* spriteBatch;
    std::unique_ptr<SpriteBatch> localBatch;
    if (CardGame != nullptr) {
        spriteBatch = &*CardGame->CardsSpriteBatch;
    } else {
        localBatch = std::make_unique<SpriteBatch>(getGraphicsDeviceProperty());
        spriteBatch = localBatch.get();
    }

    spriteBatch->Begin();

    if (CurrentDestination.has_value()) {
        if (CurrentFrame != nullptr) {
            spriteBatch->Draw(*CurrentFrame, *CurrentDestination, Color::White);
            if (Text.has_value()) {
                Vector2 size = CardGame->Font->MeasureString(*Text);
                Vector2 textPosition(CurrentDestination->X + CurrentDestination->Width / 2.0f - size.X / 2.0f,
                                     CurrentDestination->Y + CurrentDestination->Height / 2.0f - size.Y / 2.0f);
                spriteBatch->DrawString(*CardGame->Font, *Text, textPosition, TextColor);
            }
        }
    } else {
        if (CurrentFrame != nullptr) {
            spriteBatch->Draw(*CurrentFrame, CurrentPosition, Color::White);
            if (Text.has_value()) {
                Rectangle frameBounds = CurrentFrame->getBoundsProperty();
                Vector2 size = CardGame->Font->MeasureString(*Text);
                Vector2 textPosition(CurrentPosition.X + frameBounds.Width / 2.0f - size.X / 2.0f,
                                     CurrentPosition.Y + frameBounds.Height / 2.0f - size.Y / 2.0f);
                spriteBatch->DrawString(*CardGame->Font, *Text, textPosition, TextColor);
            }
        }
    }

    spriteBatch->End();
}

inline void AnimatedCardsGameComponent::Update(GameTime& gameTime) {
    AnimatedGameComponent::Update(gameTime);

    CurrentFrame = IsFaceDown ? &CardGame->CardsAssets.at("CardBack_" + CardGame->Theme)
                              : &CardGame->CardsAssets.at(UIUtility::GetCardAssetName(*Card));
}

inline void AnimatedCardsGameComponent::Draw(const GameTime& gameTime) {
    AnimatedGameComponent::Draw(gameTime);

    CardGame->CardsSpriteBatch->Begin();

    if (CurrentFrame != nullptr) {
        if (CurrentDestination.has_value()) {
            CardGame->CardsSpriteBatch->Draw(*CurrentFrame, *CurrentDestination, Color::White);
        } else {
            CardGame->CardsSpriteBatch->Draw(*CurrentFrame, CurrentPosition, Color::White);
        }
    }

    CardGame->CardsSpriteBatch->End();
}

} // namespace CardsFramework

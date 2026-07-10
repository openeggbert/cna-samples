#pragma once

// RolePlayingGame.hpp -- C++ port of RolePlayingGame.cs (the Game subclass)
// plus the final cross-referencing method bodies for Session, CombatEngine,
// TileEngine, and Party that could not be defined in their own headers due to
// mutual dependencies (Session <-> CombatEngine <-> screens, TileEngine <->
// Session, Party <-> Session/LevelUpScreen) -- consolidated here exactly like
// every other multi-file ScreenManager port in this repo (see e.g.
// NinjAcademy's/CardsStarterKit's "cross-referencing method definitions").
//
// AudioManager.Initialize in the original also wires up a GamerServicesComponent
// and an XACT AudioEngine from a compiled .xgs/.xwb/.xsb project; neither
// exists in this port (see AudioManager.hpp/missing.md) so those two lines are
// dropped.

#include <memory>
#include <optional>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GraphicsDeviceManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Input/Keyboard.hpp"
#include "Microsoft/Xna/Framework/Input/Keys.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "AudioManager.hpp"
#include "Combat/CombatEngine.hpp"
#include "Data/ContentLoader.hpp"
#include "Fonts.hpp"
#include "GameScreens/ChestScreen.hpp"
#include "GameScreens/DialogueScreen.hpp"
#include "GameScreens/GameOverScreen.hpp"
#include "GameScreens/GameplayScreen.hpp"
#include "GameScreens/Hud.hpp"
#include "GameScreens/InnScreen.hpp"
#include "GameScreens/LevelUpScreen.hpp"
#include "GameScreens/PlayerNpcScreen.hpp"
#include "GameScreens/QuestLogScreen.hpp"
#include "GameScreens/QuestNpcScreen.hpp"
#include "GameScreens/RewardsScreen.hpp"
#include "GameScreens/StoreScreen.hpp"
#include "InputManager.hpp"
#include "MenuScreens/MainMenuScreen.hpp"
#include "ScreenManager/ScreenManager.hpp"
#include "Session/Session.hpp"
#include "TileEngine/TileEngine.hpp"

namespace RolePlaying {

class RolePlayingGame : public Microsoft::Xna::Framework::Game {
public:
    RolePlayingGame() {
        graphics_ = std::make_unique<Microsoft::Xna::Framework::GraphicsDeviceManager>(this);

        getContentProperty().setRootDirectoryProperty("Content");

        contentLoader_ = std::make_unique<RolePlayingGameData::ContentLoader>(getContentProperty());

        screenManager_ = std::make_shared<ScreenManager>(*this);
        getComponentsProperty().Add(screenManager_.get());
    }

    const std::string& GetTypeName() const override {
        static const std::string name = "RolePlayingGame";
        return name;
    }

protected:
    void Initialize() override {
        InputManager::Initialize();

        graphics_->setPreferredBackBufferWidthProperty(1280);
        graphics_->setPreferredBackBufferHeightProperty(720);
        graphics_->ApplyChanges();

        Game::Initialize();

        AudioManager::Initialize(getContentProperty());
        CombatEngine::SetScreenManager(*screenManager_);

        TileEngine::SetViewport(getGraphicsDeviceProperty().getViewportProperty());

        screenManager_->AddScreen(std::make_shared<MainMenuScreen>(*contentLoader_));
    }

    void LoadContent() override {
        Fonts::LoadContent(getContentProperty());
        Game::LoadContent();

        overlayBatch_ = std::make_unique<Microsoft::Xna::Framework::Graphics::SpriteBatch>(getGraphicsDeviceProperty());
        helpTexture_.emplace(getContentProperty().Load<Microsoft::Xna::Framework::Graphics::Texture2D>("help"));
    }

    void UnloadContent() override {
        Fonts::UnloadContent();
        Game::UnloadContent();
    }

    void Update(GameTime& gameTime) override {
        InputManager::Update();

        float elapsed = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
        bool curF1 = Microsoft::Xna::Framework::Input::Keyboard::GetState().IsKeyDown(Microsoft::Xna::Framework::Input::Keys::F1);
        if (curF1 && !prevF1_) helpTimer_ = 10.0f;
        prevF1_ = curF1;
        if (helpTimer_ > 0.0f) helpTimer_ -= elapsed;

        Game::Update(gameTime);
    }

    void Draw(const GameTime& gameTime) override {
        getGraphicsDeviceProperty().Clear(Microsoft::Xna::Framework::Color(0, 0, 0, 0));
        Game::Draw(gameTime);

        if (helpTimer_ > 0.0f) {
            int hw = helpTexture_->getWidthProperty();
            int hh = helpTexture_->getHeightProperty();
            auto& vp = getGraphicsDeviceProperty().getViewportProperty();
            float sx = (float)((vp.getWidthProperty() - hw) / 2);
            float sy = (float)((vp.getHeightProperty() - hh) / 2);

            overlayBatch_->Begin();
            overlayBatch_->Draw(*helpTexture_, Microsoft::Xna::Framework::Vector2(sx, sy),
                                Microsoft::Xna::Framework::Color(255, 255, 255, 255));
            overlayBatch_->End();
        }
    }

private:
    std::unique_ptr<Microsoft::Xna::Framework::GraphicsDeviceManager> graphics_;
    std::shared_ptr<ScreenManager> screenManager_;
    std::unique_ptr<RolePlayingGameData::ContentLoader> contentLoader_;

    std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> overlayBatch_;
    std::optional<Microsoft::Xna::Framework::Graphics::Texture2D> helpTexture_;
    float helpTimer_ = 0.0f;
    bool prevF1_ = false;
};

// ============================================================================
// Cross-referencing definitions
// ============================================================================

// ---- Session methods that depend on CombatEngine/screens ----

inline bool Session::EncounterTile(Microsoft::Xna::Framework::Point mapPosition) {
    if (singleton_->quest_ &&
        (singleton_->quest_->Stage == Quest::QuestStage::InProgress ||
         singleton_->quest_->Stage == Quest::QuestStage::RequirementsMet)) {
        for (auto& entry : singleton_->quest_->FixedCombatEntries) {
            if (EndsWith(TileEngine::Map()->AssetName(), entry->MapContentName) && entry->MapPosition == mapPosition) {
                EncounterFixedCombat(entry);
                return true;
            }
        }
    }
    for (auto& entry : TileEngine::Map()->FixedCombatEntries) {
        if (entry->MapPosition == mapPosition) { EncounterFixedCombat(entry); return true; }
    }
    if (singleton_->quest_) {
        for (auto& entry : singleton_->quest_->ChestEntries) {
            if (EndsWith(TileEngine::Map()->AssetName(), entry->MapContentName) && entry->MapPosition == mapPosition) {
                EncounterChest(entry);
                return true;
            }
        }
    }
    for (auto& entry : TileEngine::Map()->ChestEntries) {
        if (entry->MapPosition == mapPosition) { EncounterChest(entry); return true; }
    }
    for (auto& entry : TileEngine::Map()->PlayerNpcEntries) {
        if (entry->MapPosition == mapPosition) { EncounterPlayerNpc(entry); return true; }
    }
    for (auto& entry : TileEngine::Map()->QuestNpcEntries) {
        if (entry->MapPosition == mapPosition) { EncounterQuestNpc(entry); return true; }
    }
    for (auto& entry : TileEngine::Map()->PortalEntries) {
        if (entry->MapPosition == mapPosition) { EncounterPortal(entry); return true; }
    }
    for (auto& entry : TileEngine::Map()->InnEntries) {
        if (entry->MapPosition == mapPosition) { EncounterInn(entry); return true; }
    }
    for (auto& entry : TileEngine::Map()->StoreEntries) {
        if (entry->MapPosition == mapPosition) { EncounterStore(entry); return true; }
    }
    return false;
}

inline void Session::EncounterFixedCombat(const std::shared_ptr<MapEntry<FixedCombat>>& fixedCombatEntry) {
    if (!fixedCombatEntry || !fixedCombatEntry->Content) return;
    if (!CombatEngine::IsActive()) CombatEngine::StartNewCombat(fixedCombatEntry);
}

inline void Session::EncounterChest(const std::shared_ptr<MapEntry<Chest>>& chestEntry) {
    if (!chestEntry || !chestEntry->Content) return;
    singleton_->screenManager_->AddScreen(std::make_shared<ChestScreen>(chestEntry));
}

inline void Session::EncounterPlayerNpc(const std::shared_ptr<MapEntry<Player>>& playerEntry) {
    if (!playerEntry || !playerEntry->Content) return;
    singleton_->screenManager_->AddScreen(std::make_shared<PlayerNpcScreen>(playerEntry));
}

inline void Session::EncounterQuestNpc(const std::shared_ptr<MapEntry<QuestNpc>>& questNpcEntry) {
    if (!questNpcEntry || !questNpcEntry->Content) return;
    singleton_->screenManager_->AddScreen(std::make_shared<QuestNpcScreen>(questNpcEntry));
}

inline void Session::EncounterInn(const std::shared_ptr<MapEntry<Inn>>& innEntry) {
    if (!innEntry || !innEntry->Content) return;
    singleton_->screenManager_->AddScreen(std::make_shared<InnScreen>(innEntry->Content));
}

inline void Session::EncounterStore(const std::shared_ptr<MapEntry<Store>>& storeEntry) {
    if (!storeEntry || !storeEntry->Content) return;
    singleton_->screenManager_->AddScreen(std::make_shared<StoreScreen>(storeEntry->Content));
}

inline bool Session::CheckForRandomCombat(const std::shared_ptr<RandomCombat>& randomCombat) {
    if (!randomCombat || randomCombat->CombatProbability <= 0) return false;
    if (CombatEngine::IsActive()) return false;
    if ((int)random_.Next(0, 100) < randomCombat->CombatProbability) {
        CombatEngine::StartNewCombat(randomCombat);
        return true;
    }
    return false;
}

inline void Session::UpdateQuest() {
    if (!party_ || !questLine_) return;

    if (!quest_ && !questLine_->Quests.empty() && !IsQuestLineComplete()) {
        quest_ = questLine_->Quests[currentQuestIndex_];
        quest_->Stage = Quest::QuestStage::NotStarted;
        party_->ClearMonsterKills();
        modifiedQuestChests_.clear();
        removedQuestChests_.clear();
        removedQuestFixedCombats_.clear();
    }

    if (quest_ && !IsQuestLineComplete()) {
        switch (quest_->Stage) {
            case Quest::QuestStage::NotStarted:
                quest_->Stage = Quest::QuestStage::InProgress;
                if (!quest_->AreRequirementsMet()) screenManager_->AddScreen(std::make_shared<QuestLogScreen>(quest_));
                break;

            case Quest::QuestStage::InProgress: {
                for (auto& req : quest_->MonsterRequirements) {
                    req->CompletedCount = 0;
                    auto it = party_->MonsterKills().find(req->Content->AssetName());
                    if (it != party_->MonsterKills().end()) req->CompletedCount = it->second;
                }
                for (auto& req : quest_->GearRequirements) {
                    req->CompletedCount = 0;
                    for (auto& entry : party_->Inventory())
                        if (entry->Content == req->Content) req->CompletedCount += entry->Count;
                }
                if (quest_->AreRequirementsMet()) {
                    for (auto& req : quest_->GearRequirements) party_->RemoveFromInventory(req->Content, req->Count);
                    if (quest_->DestinationMapContentName.empty()) {
                        quest_->Stage = Quest::QuestStage::Completed;
                        if (!quest_->CompletionMessage.empty()) {
                            auto dialogue = std::make_shared<DialogueScreen>();
                            dialogue->TitleText = "Quest Complete";
                            dialogue->BackText.clear();
                            dialogue->DialogueText = quest_->CompletionMessage;
                            screenManager_->AddScreen(dialogue);
                        }
                    } else {
                        quest_->Stage = Quest::QuestStage::RequirementsMet;
                        screenManager_->AddScreen(std::make_shared<QuestLogScreen>(quest_));
                    }
                }
                break;
            }

            case Quest::QuestStage::RequirementsMet:
                break;

            case Quest::QuestStage::Completed: {
                auto rewards = std::make_shared<RewardsScreen>(RewardsScreen::RewardScreenMode::Quest,
                                                                quest_->ExperienceReward, quest_->GoldReward,
                                                                quest_->GearRewards);
                screenManager_->AddScreen(rewards);
                currentQuestIndex_++;
                quest_.reset();
                break;
            }
        }
    }
}

inline void Session::Update(const GameTime& gameTime) {
    if (!singleton_) return;
    if (CombatEngine::IsActive()) {
        CombatEngine::Update(gameTime);
    } else {
        singleton_->UpdateQuest();
        TileEngine::Update(gameTime);
    }
}

inline void Session::Draw(const GameTime& gameTime) {
    auto& spriteBatch = singleton_->screenManager_->getSpriteBatch();

    if (CombatEngine::IsActive()) {
        if (TileEngine::Map()->CombatTexture) {
            spriteBatch.Begin();
            spriteBatch.Draw(*TileEngine::Map()->CombatTexture, Microsoft::Xna::Framework::Vector2::Zero,
                             Microsoft::Xna::Framework::Color(255, 255, 255, 255));
            spriteBatch.End();
        }
        CombatEngine::Draw(gameTime);
    } else {
        singleton_->DrawNonCombat(gameTime);
    }

    if (singleton_->hud_) singleton_->hud_->Draw();
}

inline void Session::DrawNonCombat(const GameTime& gameTime) {
    auto& spriteBatch = screenManager_->getSpriteBatch();
    auto* map = TileEngine::Map().get();
    float elapsedSeconds = (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
    int viewportHeight = TileEngine::CurrentViewport().getHeightProperty();

    spriteBatch.Begin();
    if (map->Texture) {
        TileEngine::DrawLayers(spriteBatch, true, true, false);
        DrawShadows(spriteBatch);
    }
    spriteBatch.End();

    spriteBatch.Begin(); // BackToFront sort mode not used -- see missing.md

    auto& leader = *party_->Players[0];
    Microsoft::Xna::Framework::Vector2 position = TileEngine::PartyLeaderPosition().ScreenPosition();
    leader.CharacterDirection = TileEngine::PartyLeaderPosition().PositionDirection;
    leader.ResetAnimation(TileEngine::PartyLeaderPosition().IsMoving());
    auto* activeSprite =
        (leader.State == RolePlayingGameData::Character::CharacterState::Walking && leader.WalkingSprite)
            ? leader.WalkingSprite.get()
            : leader.MapSprite.get();
    if (activeSprite) {
        activeSprite->UpdateAnimation(elapsedSeconds);
        activeSprite->Draw(spriteBatch, position, 1.0f - position.Y / (float)viewportHeight);
    }

    for (auto& entry : map->PlayerNpcEntries) {
        if (!entry->Content) continue;
        Microsoft::Xna::Framework::Vector2 pos = TileEngine::GetScreenPosition(entry->MapPosition);
        entry->Content->ResetAnimation(false);
        if (entry->Content->MapSprite) {
            entry->Content->MapSprite->UpdateAnimation(elapsedSeconds);
            entry->Content->MapSprite->Draw(spriteBatch, pos, 1.0f - pos.Y / (float)viewportHeight);
        }
    }
    for (auto& entry : map->QuestNpcEntries) {
        if (!entry->Content) continue;
        Microsoft::Xna::Framework::Vector2 pos = TileEngine::GetScreenPosition(entry->MapPosition);
        entry->Content->ResetAnimation(false);
        if (entry->Content->MapSprite) {
            entry->Content->MapSprite->UpdateAnimation(elapsedSeconds);
            entry->Content->MapSprite->Draw(spriteBatch, pos, 1.0f - pos.Y / (float)viewportHeight);
        }
    }
    for (auto& entry : map->FixedCombatEntries) {
        if (!entry->Content || entry->Content->Entries.empty() || !entry->MapSprite) continue;
        Microsoft::Xna::Framework::Vector2 pos = TileEngine::GetScreenPosition(entry->MapPosition);
        entry->MapSprite->UpdateAnimation(elapsedSeconds);
        entry->MapSprite->Draw(spriteBatch, pos, 1.0f - pos.Y / (float)viewportHeight);
    }
    if (quest_ &&
        (quest_->Stage == Quest::QuestStage::InProgress || quest_->Stage == Quest::QuestStage::RequirementsMet)) {
        for (auto& entry : quest_->FixedCombatEntries) {
            if (!entry->Content || entry->Content->Entries.empty() || !entry->MapSprite ||
                !EndsWith(map->AssetName(), entry->MapContentName))
                continue;
            Microsoft::Xna::Framework::Vector2 pos = TileEngine::GetScreenPosition(entry->MapPosition);
            entry->MapSprite->UpdateAnimation(elapsedSeconds);
            entry->MapSprite->Draw(spriteBatch, pos, 1.0f - pos.Y / (float)viewportHeight);
        }
    }
    for (auto& entry : map->ChestEntries) {
        if (!entry->Content || !entry->Content->Texture) continue;
        Microsoft::Xna::Framework::Vector2 pos = TileEngine::GetScreenPosition(entry->MapPosition);
        spriteBatch.Draw(*entry->Content->Texture, pos, Microsoft::Xna::Framework::Color(255, 255, 255, 255));
    }
    if (quest_ &&
        (quest_->Stage == Quest::QuestStage::InProgress || quest_->Stage == Quest::QuestStage::RequirementsMet)) {
        for (auto& entry : quest_->ChestEntries) {
            if (!entry->Content || !entry->Content->Texture || !EndsWith(map->AssetName(), entry->MapContentName))
                continue;
            Microsoft::Xna::Framework::Vector2 pos = TileEngine::GetScreenPosition(entry->MapPosition);
            spriteBatch.Draw(*entry->Content->Texture, pos, Microsoft::Xna::Framework::Color(255, 255, 255, 255));
        }
    }
    spriteBatch.End();

    spriteBatch.Begin();
    if (map->Texture) TileEngine::DrawLayers(spriteBatch, false, false, true);
    spriteBatch.End();
}

inline void Session::DrawShadows(Microsoft::Xna::Framework::Graphics::SpriteBatch& spriteBatch) {
    auto* map = TileEngine::Map().get();
    auto drawShadow = [&](RolePlayingGameData::Character& character, Microsoft::Xna::Framework::Vector2 pos) {
        if (!character.ShadowTexture) return;
        spriteBatch.Draw(
            *character.ShadowTexture, pos, std::nullopt, Microsoft::Xna::Framework::Color(255, 255, 255, 255), 0.0f,
            Microsoft::Xna::Framework::Vector2(
                (float)(character.ShadowTexture->getWidthProperty() - map->TileSize.X) / 2.0f,
                (float)(character.ShadowTexture->getHeightProperty() - map->TileSize.Y) / 2.0f -
                    character.ShadowTexture->getHeightProperty() / 6.0f),
            1.0f, Microsoft::Xna::Framework::Graphics::SpriteEffects::None, 1.0f);
    };

    drawShadow(*party_->Players[0], TileEngine::PartyLeaderPosition().ScreenPosition());
    for (auto& entry : map->PlayerNpcEntries)
        if (entry->Content) drawShadow(*entry->Content, TileEngine::GetScreenPosition(entry->MapPosition));
    for (auto& entry : map->QuestNpcEntries)
        if (entry->Content) drawShadow(*entry->Content, TileEngine::GetScreenPosition(entry->MapPosition));
    for (auto& entry : map->FixedCombatEntries) {
        if (!entry->Content || entry->Content->Entries.empty()) continue;
        auto& monster = *entry->Content->Entries[0]->Content;
        drawShadow(monster, TileEngine::GetScreenPosition(entry->MapPosition));
    }
}

inline void Session::StartNewSession(const GameStartDescription& gameStartDescription, ScreenManager& screenManager,
                                      GameplayScreen& gameplayScreen, RolePlayingGameData::ContentLoader& contentLoader) {
    EndSession();

    singleton_ = new Session(screenManager, gameplayScreen, contentLoader);
    singleton_->hud_ = new Hud(screenManager);
    singleton_->hud_->LoadContent();

    ChangeMap(gameStartDescription.MapContentName, nullptr);

    singleton_->party_ = std::make_unique<Party>(gameStartDescription, contentLoader);

    singleton_->questLine_ = contentLoader.LoadQuestLine(gameStartDescription.QuestLineContentName)->Clone();
}

inline void Session::EndSession() {
    if (!singleton_) return;
    GameplayScreen* gameplayScreen = singleton_->gameplayScreen_;
    singleton_->gameplayScreen_ = nullptr;
    AudioManager::PopMusic();
    Session* old = singleton_;
    singleton_ = nullptr;
    delete old;
    if (gameplayScreen) gameplayScreen->ExitScreen();
}

// ---- CombatEngine methods that depend on Session ----

inline void CombatEngine::StartNewCombat(const std::shared_ptr<MapEntry<FixedCombat>>& fixedCombatEntry) {
    ClearCombat();
    activeFixedCombatEntry_ = fixedCombatEntry;
    fleeProbability_ = 0; // fixed encounters can't be fled in the original either
    for (auto& entry : fixedCombatEntry->Content->Entries) {
        for (int i = 0; i < entry->Count; i++) {
            CombatMonster cm;
            cm.monster = entry->Content;
            cm.currentHealthPoints = entry->Content->CharacterStatistics().HealthPoints;
            monsters_.push_back(cm);
        }
    }
    combatPlayers_ = Session::GetParty()->Players;
    BeginEncounter();
}

inline void CombatEngine::StartNewCombat(const std::shared_ptr<RandomCombat>& randomCombat) {
    ClearCombat();
    fleeProbability_ = randomCombat->FleeProbability;
    int count = randomCombat->MonsterCountRange.GenerateValue(Random());
    count = std::max(1, count);
    int totalWeight = 0;
    for (auto& e : randomCombat->Entries) totalWeight += e->Weight;
    for (int i = 0; i < count && totalWeight > 0; i++) {
        int roll = Random().Next(0, totalWeight);
        int accum = 0;
        for (auto& e : randomCombat->Entries) {
            accum += e->Weight;
            if (roll < accum) {
                CombatMonster cm;
                cm.monster = e->Content;
                cm.currentHealthPoints = e->Content->CharacterStatistics().HealthPoints;
                monsters_.push_back(cm);
                break;
            }
        }
    }
    combatPlayers_ = Session::GetParty()->Players;
    BeginEncounter();
}

inline void CombatEngine::CheckForEndOfCombat() {
    bool allMonstersDead = std::all_of(monsters_.begin(), monsters_.end(), [](auto& m) { return m.IsDead(); });
    bool allPlayersDead =
        std::all_of(combatPlayers_.begin(), combatPlayers_.end(),
                    [](auto& p) { return p->CurrentStatistics().HealthPoints <= 0; });

    if (allMonstersDead) {
        phase_ = Phase::Victory;
        GrantRewardsAndEnd();
    } else if (allPlayersDead) {
        phase_ = Phase::Defeat;
        auto* party = Session::GetParty();
        ClearCombat();
        Session::EndSession();
        (void)party;
    } else {
        AdvanceToNextLivingPlayer(true);
    }
}

inline void CombatEngine::GrantRewardsAndEnd() {
    auto* party = Session::GetParty();
    int gold = 0, experience = 0;
    std::vector<std::shared_ptr<RolePlayingGameData::Gear>> gearDrops;
    for (auto& m : monsters_) {
        gold += m.monster->CalculateGoldReward(Random());
        experience += m.monster->CalculateExperienceReward(Random());
        if (party) party->AddMonsterKill(*m.monster);
    }

    auto* screenManager = Session::GetScreenManager();
    if (activeFixedCombatEntry_) Session::RemoveFixedCombat(activeFixedCombatEntry_);

    ClearCombat();
    AudioManager::PopMusic();

    if (screenManager) {
        auto rewards =
            std::make_shared<RewardsScreen>(RewardsScreen::RewardScreenMode::Combat, experience, gold, gearDrops);
        screenManager->AddScreen(rewards);
    }
}

// ---- TileEngine methods that depend on Session ----

inline void TileEngine::Update(const GameTime& gameTime) {
    Vector2 autoMovement = UpdatePartyLeaderAutoMovement();

    Vector2 userMovement = Vector2::Zero;
    if (autoMovement == Vector2::Zero) {
        userMovement = UpdateUserMovement();
        if (userMovement != Vector2::Zero) {
            Point desiredTilePosition = partyLeaderPosition_.TilePosition;
            Vector2 desiredTileOffset = partyLeaderPosition_.TileOffset;
            PlayerPosition::CalculateMovement(Vector2::Multiply(userMovement, 15.0f), desiredTilePosition,
                                              desiredTileOffset);
            if (partyLeaderPosition_.TilePosition != desiredTilePosition && !MoveIntoTile(desiredTilePosition)) {
                userMovement = Vector2::Zero;
            }
        }
    }

    Point oldTilePosition = partyLeaderPosition_.TilePosition;
    Vector2 combinedMovement = autoMovement + userMovement;
    partyLeaderPosition_.Move(combinedMovement);

    if (autoMovement == Vector2::Zero && partyLeaderPosition_.TilePosition != oldTilePosition) {
        Session::CheckForRandomCombat(map_->RandomCombatData);
    }

    auto* party = Session::GetParty();
    Vector2 leaderOffset =
        party && !party->Players.empty() && party->Players[0]->MapSprite ? party->Players[0]->MapSprite->SourceOffset : Vector2::Zero;
    mapOriginPosition_ = mapOriginPosition_ + (viewportCenter_ - (partyLeaderPosition_.ScreenPosition() + leaderOffset));

    mapOriginPosition_.X = Microsoft::Xna::Framework::MathHelper::Min(mapOriginPosition_.X, (float)viewport_.getXProperty());
    mapOriginPosition_.Y = Microsoft::Xna::Framework::MathHelper::Min(mapOriginPosition_.Y, (float)viewport_.getYProperty());
    mapOriginPosition_.X += Microsoft::Xna::Framework::MathHelper::Max(
        (float)(viewport_.getXProperty() + viewport_.getWidthProperty()) -
            (mapOriginPosition_.X + map_->MapDimensions.X * map_->TileSize.X),
        0.0f);
    mapOriginPosition_.Y += Microsoft::Xna::Framework::MathHelper::Max(
        (float)(viewport_.getYProperty() + viewport_.getHeightProperty() - Hud::HudHeight) -
            (mapOriginPosition_.Y + map_->MapDimensions.Y * map_->TileSize.Y),
        0.0f);
}

inline bool TileEngine::MoveIntoTile(Point mapPosition) {
    if (map_->IsBlocked(mapPosition)) return false;
    if (Session::EncounterTile(mapPosition)) return false;
    return true;
}

// ---- Party methods that depend on Session/LevelUpScreen ----

inline void Party::GiveExperience(int experience) {
    if (experience <= 0) return;
    std::vector<std::shared_ptr<Player>> leveledUp;
    for (auto& player : Players) {
        int oldLevel = player->CharacterLevel();
        player->SetExperience(player->Experience() + experience);
        if (player->CharacterLevel() > oldLevel) leveledUp.push_back(player);
    }
    if (!leveledUp.empty()) Session::GetScreenManager()->AddScreen(std::make_shared<LevelUpScreen>(leveledUp));
}

} // namespace RolePlaying

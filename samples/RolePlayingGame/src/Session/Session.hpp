#pragma once

// Session.hpp -- C++ port of Session/Session.cs. SaveGame/LoadSession/
// StorageDevice persistence is dropped entirely (CNA has no StorageDevice
// equivalent, matching this project's established IsolatedStorageFile/
// StorageDevice-dropping precedent across every prior ScreenManager port) --
// a session always starts fresh via StartNewSession. See missing.md.

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "System/Random.hpp"

#include "../AudioManager.hpp"
#include "../Data/Characters/Player.hpp"
#include "../Data/Characters/QuestNpc.hpp"
#include "../Data/ContentLoader.hpp"
#include "../Data/GameStartDescription.hpp"
#include "../Data/Map/Map.hpp"
#include "../Data/Quests/QuestLine.hpp"
#include "../Data/WorldEntry.hpp"
#include "../ScreenManager/ScreenManager.hpp"
#include "../TileEngine/TileEngine.hpp"
#include "ModifiedChestEntry.hpp"
#include "Party.hpp"

namespace RolePlaying {

using RolePlayingGameData::Chest;
using RolePlayingGameData::FixedCombat;
using RolePlayingGameData::GameStartDescription;
using RolePlayingGameData::Inn;
using RolePlayingGameData::Map;
using RolePlayingGameData::MapEntry;
using RolePlayingGameData::Player;
using RolePlayingGameData::Portal;
using RolePlayingGameData::Quest;
using RolePlayingGameData::QuestLine;
using RolePlayingGameData::QuestNpc;
using RolePlayingGameData::RandomCombat;
using RolePlayingGameData::Store;
using RolePlayingGameData::WorldEntry;

class GameplayScreen; // fwd decl
class Hud;            // fwd decl

class Session {
public:
    static Party* GetParty() { return singleton_ ? singleton_->party_.get() : nullptr; }

    // Change the current map, arriving at the given portal if any (may be null).
    static void ChangeMap(const std::string& contentName, const std::shared_ptr<Portal>& originalPortal) {
        std::string mapContentName = contentName;
        const std::string prefix = "Maps/";
        if (mapContentName.rfind(prefix, 0) != 0) mapContentName = prefix + mapContentName;
        // strip "Maps/" for the loader, which prefixes it itself
        std::string mapName = mapContentName.substr(prefix.size());

        if (TileEngine::Map() && TileEngine::Map()->AssetName() == mapContentName) {
            TileEngine::SetMap(TileEngine::Map(),
                                originalPortal ? TileEngine::Map()->FindPortal(originalPortal->DestinationMapPortalName)
                                               : nullptr);
        }

        auto map = singleton_->contentLoader_.LoadMap(mapName)->Clone();
        singleton_->ModifyMap(*map);
        AudioManager::PlayMusic(map->MusicCueName);
        TileEngine::SetMap(map, originalPortal ? map->FindPortal(originalPortal->DestinationMapPortalName) : nullptr);
    }

    // Performs any actions associated with the given tile. Returns true if
    // anything was encountered.
    static bool EncounterTile(Microsoft::Xna::Framework::Point mapPosition);

    static void EncounterFixedCombat(const std::shared_ptr<MapEntry<FixedCombat>>& fixedCombatEntry);
    static void EncounterChest(const std::shared_ptr<MapEntry<Chest>>& chestEntry);
    static void EncounterPlayerNpc(const std::shared_ptr<MapEntry<Player>>& playerEntry);
    static void EncounterQuestNpc(const std::shared_ptr<MapEntry<QuestNpc>>& questNpcEntry);
    static void EncounterInn(const std::shared_ptr<MapEntry<Inn>>& innEntry);
    static void EncounterStore(const std::shared_ptr<MapEntry<Store>>& storeEntry);
    static void EncounterPortal(const std::shared_ptr<MapEntry<Portal>>& portalEntry) {
        ChangeMap(portalEntry->Content->DestinationMapContentName, portalEntry->Content);
    }

    static bool CheckForRandomCombat(const std::shared_ptr<RandomCombat>& randomCombat);

    static QuestLine* GetQuestLine() { return singleton_ ? singleton_->questLine_.get() : nullptr; }
    static bool IsQuestLineComplete() {
        return singleton_ && singleton_->questLine_ &&
               singleton_->currentQuestIndex_ >= (int)singleton_->questLine_->QuestContentNames.size();
    }
    static Quest* GetQuest() { return singleton_ ? singleton_->quest_.get() : nullptr; }
    static int CurrentQuestIndex() { return singleton_ ? singleton_->currentQuestIndex_ : -1; }

    void UpdateQuest();

    static void RemoveChest(const std::shared_ptr<MapEntry<Chest>>& mapEntry) {
        if (!mapEntry) return;
        if (TileEngine::Map()) {
            auto& entries = TileEngine::Map()->ChestEntries;
            auto it = std::remove_if(entries.begin(), entries.end(), [&](auto& e) {
                return e->ContentName == mapEntry->ContentName && e->MapPosition == mapEntry->MapPosition;
            });
            if (it != entries.end()) {
                WorldEntry<Chest> worldEntry;
                worldEntry.Content = mapEntry->Content;
                worldEntry.ContentName = mapEntry->ContentName;
                worldEntry.Count = mapEntry->Count;
                worldEntry.EntryDirection = mapEntry->EntryDirection;
                worldEntry.MapContentName = TileEngine::Map()->AssetName();
                worldEntry.MapPosition = mapEntry->MapPosition;
                entries.erase(it, entries.end());
                singleton_->removedMapChests_.push_back(worldEntry);
                return;
            }
        }
        if (singleton_->quest_) {
            auto& entries = singleton_->quest_->ChestEntries;
            auto it = std::remove_if(entries.begin(), entries.end(), [&](auto& e) {
                return e->ContentName == mapEntry->ContentName && e->MapPosition == mapEntry->MapPosition &&
                       EndsWith(TileEngine::Map()->AssetName(), e->MapContentName);
            });
            if (it != entries.end()) {
                WorldEntry<Chest> worldEntry;
                worldEntry.Content = mapEntry->Content;
                worldEntry.ContentName = mapEntry->ContentName;
                worldEntry.Count = mapEntry->Count;
                worldEntry.EntryDirection = mapEntry->EntryDirection;
                worldEntry.MapContentName = TileEngine::Map()->AssetName();
                worldEntry.MapPosition = mapEntry->MapPosition;
                entries.erase(it, entries.end());
                singleton_->removedQuestChests_.push_back(worldEntry);
            }
        }
    }

    static void RemoveFixedCombat(const std::shared_ptr<MapEntry<FixedCombat>>& mapEntry) {
        if (!mapEntry) return;
        if (TileEngine::Map()) {
            auto& entries = TileEngine::Map()->FixedCombatEntries;
            auto it = std::remove_if(entries.begin(), entries.end(), [&](auto& e) {
                return e->ContentName == mapEntry->ContentName && e->MapPosition == mapEntry->MapPosition;
            });
            if (it != entries.end()) {
                WorldEntry<FixedCombat> worldEntry;
                worldEntry.Content = mapEntry->Content;
                worldEntry.ContentName = mapEntry->ContentName;
                worldEntry.Count = mapEntry->Count;
                worldEntry.EntryDirection = mapEntry->EntryDirection;
                worldEntry.MapContentName = TileEngine::Map()->AssetName();
                worldEntry.MapPosition = mapEntry->MapPosition;
                entries.erase(it, entries.end());
                singleton_->removedMapFixedCombats_.push_back(worldEntry);
                return;
            }
        }
        if (singleton_->quest_) {
            auto& entries = singleton_->quest_->FixedCombatEntries;
            auto it = std::remove_if(entries.begin(), entries.end(), [&](auto& e) {
                return e->ContentName == mapEntry->ContentName && e->MapPosition == mapEntry->MapPosition &&
                       EndsWith(TileEngine::Map()->AssetName(), e->MapContentName);
            });
            if (it != entries.end()) {
                WorldEntry<FixedCombat> worldEntry;
                worldEntry.Content = mapEntry->Content;
                worldEntry.ContentName = mapEntry->ContentName;
                worldEntry.Count = mapEntry->Count;
                worldEntry.EntryDirection = mapEntry->EntryDirection;
                worldEntry.MapContentName = TileEngine::Map()->AssetName();
                worldEntry.MapPosition = mapEntry->MapPosition;
                entries.erase(it, entries.end());
                singleton_->removedQuestFixedCombats_.push_back(worldEntry);
            }
        }
    }

    static void RemovePlayerNpc(const std::shared_ptr<MapEntry<Player>>& mapEntry) {
        if (!mapEntry || !TileEngine::Map()) return;
        auto& entries = TileEngine::Map()->PlayerNpcEntries;
        auto it = std::remove_if(entries.begin(), entries.end(), [&](auto& e) {
            return e->ContentName == mapEntry->ContentName && e->MapPosition == mapEntry->MapPosition;
        });
        if (it != entries.end()) {
            WorldEntry<Player> worldEntry;
            worldEntry.Content = mapEntry->Content;
            worldEntry.ContentName = mapEntry->ContentName;
            worldEntry.Count = mapEntry->Count;
            worldEntry.EntryDirection = mapEntry->EntryDirection;
            worldEntry.MapContentName = TileEngine::Map()->AssetName();
            worldEntry.MapPosition = mapEntry->MapPosition;
            entries.erase(it, entries.end());
            singleton_->removedMapPlayerNpcs_.push_back(worldEntry);
        }
    }

    static void StoreModifiedChest(const std::shared_ptr<MapEntry<Chest>>& mapEntry) {
        if (!mapEntry || !mapEntry->Content) return;
        auto matches = [&](const ModifiedChestEntry& e) {
            return EndsWith(TileEngine::Map()->AssetName(), e.WorldEntry.MapContentName) &&
                   e.WorldEntry.ContentName == mapEntry->ContentName && e.WorldEntry.MapPosition == mapEntry->MapPosition;
        };

        bool inMap = TileEngine::Map() &&
                     std::any_of(TileEngine::Map()->ChestEntries.begin(), TileEngine::Map()->ChestEntries.end(),
                                 [&](auto& e) {
                                     return e->ContentName == mapEntry->ContentName && e->MapPosition == mapEntry->MapPosition;
                                 });
        if (inMap) {
            auto& list = singleton_->modifiedMapChests_;
            list.erase(std::remove_if(list.begin(), list.end(), matches), list.end());
            ModifiedChestEntry entry;
            entry.WorldEntry.Content = mapEntry->Content;
            entry.WorldEntry.ContentName = mapEntry->ContentName;
            entry.WorldEntry.Count = mapEntry->Count;
            entry.WorldEntry.EntryDirection = mapEntry->EntryDirection;
            entry.WorldEntry.MapContentName = TileEngine::Map()->AssetName();
            entry.WorldEntry.MapPosition = mapEntry->MapPosition;
            entry.ChestEntries = mapEntry->Content->Entries;
            entry.Gold = mapEntry->Content->Gold;
            list.push_back(entry);
            return;
        }

        bool inQuest =
            singleton_->quest_ && std::any_of(singleton_->quest_->ChestEntries.begin(), singleton_->quest_->ChestEntries.end(),
                                               [&](auto& e) {
                                                   return e->ContentName == mapEntry->ContentName &&
                                                          e->MapPosition == mapEntry->MapPosition &&
                                                          EndsWith(TileEngine::Map()->AssetName(), e->MapContentName);
                                               });
        if (inQuest) {
            auto& list = singleton_->modifiedQuestChests_;
            list.erase(std::remove_if(list.begin(), list.end(), matches), list.end());
            ModifiedChestEntry entry;
            entry.WorldEntry.Content = mapEntry->Content;
            entry.WorldEntry.ContentName = mapEntry->ContentName;
            entry.WorldEntry.Count = mapEntry->Count;
            entry.WorldEntry.EntryDirection = mapEntry->EntryDirection;
            entry.WorldEntry.MapContentName = TileEngine::Map()->AssetName();
            entry.WorldEntry.MapPosition = mapEntry->MapPosition;
            entry.ChestEntries = mapEntry->Content->Entries;
            entry.Gold = mapEntry->Content->Gold;
            list.push_back(entry);
        }
    }

    static ScreenManager* GetScreenManager() { return singleton_ ? singleton_->screenManager_ : nullptr; }
    static Hud* GetHud() { return singleton_ ? singleton_->hud_ : nullptr; }

    static bool IsActive() { return singleton_ != nullptr; }

    // Update the session for this frame (only called if there are no menus in use).
    static void Update(const GameTime& gameTime);
    static void Draw(const GameTime& gameTime);

    static void StartNewSession(const GameStartDescription& gameStartDescription, ScreenManager& screenManager,
                                 GameplayScreen& gameplayScreen, RolePlayingGameData::ContentLoader& contentLoader);

    static void EndSession();

    static System::Random& GetRandom() { return random_; }

private:
    Session(ScreenManager& screenManager, GameplayScreen& gameplayScreen, RolePlayingGameData::ContentLoader& contentLoader)
        : screenManager_(&screenManager), gameplayScreen_(&gameplayScreen), contentLoader_(contentLoader) {}

    void DrawNonCombat(const GameTime& gameTime);
    void DrawShadows(Microsoft::Xna::Framework::Graphics::SpriteBatch& spriteBatch);

    void ModifyMap(Map& map) {
        auto& chests = map.ChestEntries;
        chests.erase(std::remove_if(chests.begin(), chests.end(),
                                     [&](auto& e) {
                                         return std::any_of(removedMapChests_.begin(), removedMapChests_.end(), [&](auto& r) {
                                             return EndsWith(map.AssetName(), r.MapContentName) && r.ContentName == e->ContentName &&
                                                    r.MapPosition == e->MapPosition;
                                         });
                                     }),
                     chests.end());

        auto& combats = map.FixedCombatEntries;
        combats.erase(std::remove_if(combats.begin(), combats.end(),
                                      [&](auto& e) {
                                          return std::any_of(removedMapFixedCombats_.begin(), removedMapFixedCombats_.end(),
                                                              [&](auto& r) {
                                                                  return EndsWith(map.AssetName(), r.MapContentName) &&
                                                                         r.ContentName == e->ContentName &&
                                                                         r.MapPosition == e->MapPosition;
                                                              });
                                      }),
                      combats.end());

        auto& npcs = map.PlayerNpcEntries;
        npcs.erase(std::remove_if(npcs.begin(), npcs.end(),
                                   [&](auto& e) {
                                       return std::any_of(removedMapPlayerNpcs_.begin(), removedMapPlayerNpcs_.end(), [&](auto& r) {
                                           return EndsWith(map.AssetName(), r.MapContentName) && r.ContentName == e->ContentName &&
                                                  r.MapPosition == e->MapPosition;
                                       });
                                   }),
                   npcs.end());

        for (auto& entry : map.ChestEntries) {
            for (auto& modified : modifiedMapChests_) {
                if (EndsWith(map.AssetName(), modified.WorldEntry.MapContentName) &&
                    modified.WorldEntry.ContentName == entry->ContentName && modified.WorldEntry.MapPosition == entry->MapPosition) {
                    ModifyChest(*entry->Content, modified);
                    break;
                }
            }
        }
    }

    void ModifyQuest(Quest& quest) {
        auto& chests = quest.ChestEntries;
        chests.erase(std::remove_if(chests.begin(), chests.end(),
                                     [&](auto& e) {
                                         return std::any_of(removedQuestChests_.begin(), removedQuestChests_.end(), [&](auto& r) {
                                             return EndsWith(r.MapContentName, e->MapContentName) && r.ContentName == e->ContentName &&
                                                    r.MapPosition == e->MapPosition;
                                         });
                                     }),
                     chests.end());

        auto& combats = quest.FixedCombatEntries;
        combats.erase(std::remove_if(combats.begin(), combats.end(),
                                      [&](auto& e) {
                                          return std::any_of(removedQuestFixedCombats_.begin(), removedQuestFixedCombats_.end(),
                                                              [&](auto& r) {
                                                                  return EndsWith(r.MapContentName, e->MapContentName) &&
                                                                         r.ContentName == e->ContentName &&
                                                                         r.MapPosition == e->MapPosition;
                                                              });
                                      }),
                      combats.end());

        for (auto& entry : quest.ChestEntries) {
            for (auto& modified : modifiedQuestChests_) {
                if (modified.WorldEntry.MapContentName == entry->MapContentName &&
                    modified.WorldEntry.ContentName == entry->ContentName && modified.WorldEntry.MapPosition == entry->MapPosition) {
                    ModifyChest(*entry->Content, modified);
                    break;
                }
            }
        }
    }

    static void ModifyChest(Chest& chest, const ModifiedChestEntry& modifiedChestEntry) {
        chest.Gold = modifiedChestEntry.Gold;
        auto& entries = chest.Entries;
        entries.erase(std::remove_if(entries.begin(), entries.end(),
                                      [&](auto& e) {
                                          return !std::any_of(modifiedChestEntry.ChestEntries.begin(),
                                                               modifiedChestEntry.ChestEntries.end(),
                                                               [&](auto& m) { return e->ContentName == m->ContentName; });
                                      }),
                      entries.end());
        for (auto& entry : entries) {
            for (auto& modified : modifiedChestEntry.ChestEntries) {
                if (entry->ContentName == modified->ContentName) { entry->Count = modified->Count; break; }
            }
        }
    }

    static bool EndsWith(const std::string& value, const std::string& suffix) {
        if (suffix.empty()) return false;
        if (suffix.size() > value.size()) return false;
        return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    static inline Session* singleton_ = nullptr;

    std::unique_ptr<Party> party_;
    ScreenManager* screenManager_;
    GameplayScreen* gameplayScreen_;
    Hud* hud_ = nullptr;
    RolePlayingGameData::ContentLoader& contentLoader_;

    std::shared_ptr<QuestLine> questLine_;
    std::shared_ptr<Quest> quest_;
    int currentQuestIndex_ = 0;

    std::vector<WorldEntry<Chest>> removedMapChests_;
    std::vector<WorldEntry<Chest>> removedQuestChests_;
    std::vector<WorldEntry<FixedCombat>> removedMapFixedCombats_;
    std::vector<WorldEntry<FixedCombat>> removedQuestFixedCombats_;
    std::vector<WorldEntry<Player>> removedMapPlayerNpcs_;
    std::vector<ModifiedChestEntry> modifiedMapChests_;
    std::vector<ModifiedChestEntry> modifiedQuestChests_;

    static inline System::Random random_;
};

} // namespace RolePlaying

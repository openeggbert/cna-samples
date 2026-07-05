#pragma once

// ContentLoader.hpp -- RolePlayingGame's XML data loader (see missing.md's
// "XML data loader" section for the full rationale). The XNA original parses
// its 281 XML data files via the stock content pipeline's IntermediateSerializer
// (element name == C# property name, by reflection) at build time, producing
// compiled .xnb files that RolePlayingGameData's *Reader classes then read
// positionally. CNA has no content pipeline for arbitrary custom types, and
// hand-translating 281 files into C++ construction code (this project's usual
// approach for a handful of files, e.g. DynamicMenu/NinjAcademy) does not scale
// here -- so this loader reads the XML files directly at runtime, matching each
// property by NAME (not position) via Xml::XmlNode, which is actually simpler
// and more robust than replicating the *Reader classes' positional reads.

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "System/Random.hpp"

#include "../Xml/XmlNode.hpp"
#include "AnimatingSprite.hpp"
#include "Characters/CharacterClass.hpp"
#include "Characters/Monster.hpp"
#include "Characters/Player.hpp"
#include "Characters/QuestNpc.hpp"
#include "ContentEntry.hpp"
#include "Direction.hpp"
#include "GameStartDescription.hpp"
#include "Gear/Armor.hpp"
#include "Gear/Item.hpp"
#include "Gear/Weapon.hpp"
#include "Int32Range.hpp"
#include "Map/Chest.hpp"
#include "Map/FixedCombat.hpp"
#include "Map/Inn.hpp"
#include "Map/Map.hpp"
#include "Map/Portal.hpp"
#include "Map/RandomCombat.hpp"
#include "Map/Store.hpp"
#include "MapEntry.hpp"
#include "Quests/Quest.hpp"
#include "Quests/QuestLine.hpp"
#include "Spell.hpp"
#include "StatisticsRange.hpp"
#include "StatisticsValue.hpp"
#include "WeightedContentEntry.hpp"
#include "WorldEntry.hpp"

namespace RolePlayingGameData {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Graphics::Texture2D;
using RolePlaying::Xml::XmlNode;

class ContentLoader {
public:
    explicit ContentLoader(ContentManager& content) : content_(content) {}

    // ---- Gear ----

    std::shared_ptr<Gear> LoadGear(const std::string& path) {
        if (auto it = gearCache_.find(path); it != gearCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        std::string type = asset->attributes["Type"];
        std::shared_ptr<Gear> gear;
        if (type == "RolePlayingGameData.Armor") gear = ReadArmor(asset.get(), path);
        else if (type == "RolePlayingGameData.Weapon") gear = ReadWeapon(asset.get(), path);
        else if (type == "RolePlayingGameData.Item") gear = ReadItem(asset.get(), path);
        else throw std::runtime_error("ContentLoader: unknown Gear type '" + type + "' in " + path);
        gearCache_[path] = gear;
        return gear;
    }

    std::shared_ptr<Armor> LoadArmor(const std::string& path) {
        return std::dynamic_pointer_cast<Armor>(LoadGear(path));
    }
    std::shared_ptr<Weapon> LoadWeapon(const std::string& path) {
        return std::dynamic_pointer_cast<Weapon>(LoadGear(path));
    }
    std::shared_ptr<Item> LoadItem(const std::string& path) {
        return std::dynamic_pointer_cast<Item>(LoadGear(path));
    }

    // ---- Spells / Character classes ----

    std::shared_ptr<Spell> LoadSpell(const std::string& name) {
        std::string path = "Spells/" + name;
        if (auto it = spellCache_.find(path); it != spellCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        auto spell = std::make_shared<Spell>();
        spell->SetAssetName(path);
        spell->Name = ReadString(asset.get(), "Name");
        spell->Description = ReadString(asset.get(), "Description");
        spell->MagicPointCost = ReadInt(asset.get(), "MagicPointCost");
        spell->IconTextureName = ReadString(asset.get(), "IconTextureName");
        spell->IconTexture = LoadTexture("Textures/Spells/" + spell->IconTextureName);
        spell->IsOffensive = ReadBool(asset.get(), "IsOffensive");
        spell->TargetDuration = ReadInt(asset.get(), "TargetDuration");
        auto range = ReadStatisticsRange(asset->Child("TargetEffectRange"));
        spell->InitialTargetEffectRange = range;
        spell->SetTargetEffectRangeDirect(range);
        spell->AdjacentTargets = ReadInt(asset.get(), "AdjacentTargets");
        spell->LevelingProgression = ReadStatisticsValue(asset->Child("LevelingProgression"));
        spell->CreatingCueName = ReadString(asset.get(), "CreatingCueName");
        spell->TravelingCueName = ReadString(asset.get(), "TravelingCueName");
        spell->ImpactCueName = ReadString(asset.get(), "ImpactCueName");
        spell->BlockCueName = ReadString(asset.get(), "BlockCueName");
        spell->SpellSprite = ReadAnimatingSprite(asset.get(), "SpellSprite");
        CenterOverlaySourceOffset(spell->SpellSprite);
        spell->Overlay = ReadAnimatingSprite(asset.get(), "Overlay");
        CenterOverlaySourceOffset(spell->Overlay);
        spell->SetLevel(1);
        spellCache_[path] = spell;
        return spell;
    }

    std::shared_ptr<CharacterClass> LoadCharacterClass(const std::string& name) {
        std::string path = "CharacterClasses/" + name;
        if (auto it = classCache_.find(path); it != classCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        auto cls = std::make_shared<CharacterClass>();
        cls->SetAssetName(path);
        cls->Name = ReadString(asset.get(), "Name");
        cls->InitialStatistics = ReadStatisticsValue(asset->Child("InitialStatistics"));
        if (auto* ls = asset->Child("LevelingStatistics")) {
            auto& s = cls->LevelingStatistics;
            s.HealthPointsIncrease = ReadInt(ls, "HealthPointsIncrease");
            s.LevelsPerHealthPointsIncrease = ReadInt(ls, "LevelsPerHealthPointsIncrease");
            s.MagicPointsIncrease = ReadInt(ls, "MagicPointsIncrease");
            s.LevelsPerMagicPointsIncrease = ReadInt(ls, "LevelsPerMagicPointsIncrease");
            s.PhysicalOffenseIncrease = ReadInt(ls, "PhysicalOffenseIncrease");
            s.LevelsPerPhysicalOffenseIncrease = ReadInt(ls, "LevelsPerPhysicalOffenseIncrease");
            s.PhysicalDefenseIncrease = ReadInt(ls, "PhysicalDefenseIncrease");
            s.LevelsPerPhysicalDefenseIncrease = ReadInt(ls, "LevelsPerPhysicalDefenseIncrease");
            s.MagicalOffenseIncrease = ReadInt(ls, "MagicalOffenseIncrease");
            s.LevelsPerMagicalOffenseIncrease = ReadInt(ls, "LevelsPerMagicalOffenseIncrease");
            s.MagicalDefenseIncrease = ReadInt(ls, "MagicalDefenseIncrease");
            s.LevelsPerMagicalDefenseIncrease = ReadInt(ls, "LevelsPerMagicalDefenseIncrease");
        }
        if (auto* entries = asset->Child("LevelEntries")) {
            for (auto* item : entries->Children("Item")) {
                CharacterLevelDescription desc;
                desc.ExperiencePoints = ReadInt(item, "ExperiencePoints");
                desc.SpellContentNames = ReadStringList(item, "SpellContentNames");
                for (auto& spellName : desc.SpellContentNames) desc.Spells.push_back(LoadSpell(spellName));
                cls->LevelEntries.push_back(std::move(desc));
            }
        }
        cls->BaseExperienceValue = ReadInt(asset.get(), "BaseExperienceValue");
        cls->BaseGoldValue = ReadInt(asset.get(), "BaseGoldValue");
        classCache_[path] = cls;
        return cls;
    }

    // ---- Characters ----

    std::shared_ptr<Monster> LoadMonster(const std::string& name) {
        std::string path = "Characters/Monsters/" + name;
        if (auto it = monsterCache_.find(path); it != monsterCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        auto monster = std::make_shared<Monster>();
        ReadFightingCharacterBase(*monster, asset.get(), path);
        monster->DefendPercentage = ReadInt(asset.get(), "DefendPercentage");
        if (auto* drops = asset->Child("GearDrops")) {
            for (auto* item : drops->Children("Item")) {
                GearDrop drop;
                drop.GearName = ReadString(item, "GearName");
                drop.SetDropPercentage(ReadInt(item, "DropPercentage"));
                monster->GearDrops.push_back(drop);
            }
        }
        monsterCache_[path] = monster;
        return monster;
    }

    std::shared_ptr<Player> LoadPlayer(const std::string& name) {
        std::string path = "Characters/Players/" + name;
        if (auto it = playerCache_.find(path); it != playerCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        auto player = std::make_shared<Player>();
        ReadFightingCharacterBase(*player, asset.get(), path);
        player->Gold = ReadInt(asset.get(), "Gold");
        player->IntroductionDialogue = ReadString(asset.get(), "IntroductionDialogue");
        player->JoinAcceptedDialogue = ReadString(asset.get(), "JoinAcceptedDialogue");
        player->JoinRejectedDialogue = ReadString(asset.get(), "JoinRejectedDialogue");
        player->ActivePortraitTextureName = ReadString(asset.get(), "ActivePortraitTextureName");
        player->ActivePortraitTexture = LoadTexture("Textures/Characters/Portraits/" + player->ActivePortraitTextureName);
        player->InactivePortraitTextureName = ReadString(asset.get(), "InactivePortraitTextureName");
        player->InactivePortraitTexture = LoadTexture("Textures/Characters/Portraits/" + player->InactivePortraitTextureName);
        player->UnselectablePortraitTextureName = ReadString(asset.get(), "UnselectablePortraitTextureName");
        player->UnselectablePortraitTexture =
            LoadTexture("Textures/Characters/Portraits/" + player->UnselectablePortraitTextureName);
        playerCache_[path] = player;
        return player;
    }

    std::shared_ptr<QuestNpc> LoadQuestNpc(const std::string& name) {
        std::string path = "Characters/QuestNpcs/" + name;
        if (auto it = questNpcCache_.find(path); it != questNpcCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        auto npc = std::make_shared<QuestNpc>();
        ReadCharacterBase(*npc, asset.get());
        npc->IntroductionDialogue = ReadString(asset.get(), "IntroductionDialogue");
        questNpcCache_[path] = npc;
        return npc;
    }

    // ---- Maps ----

    std::shared_ptr<Map> LoadMap(const std::string& name) {
        std::string path = "Maps/" + name;
        auto asset = LoadAsset(path);
        auto map = std::make_shared<Map>();
        map->SetAssetName(path);
        map->Name = ReadString(asset.get(), "Name");
        map->MapDimensions = ReadPoint(asset.get(), "MapDimensions");
        map->TileSize = ReadPoint(asset.get(), "TileSize");
        map->SpawnMapPosition = ReadPoint(asset.get(), "SpawnMapPosition");

        map->TextureName = ReadString(asset.get(), "TextureName");
        map->Texture = LoadTexture("Textures/Maps/NonCombat/" + map->TextureName);
        map->TilesPerRow = map->Texture ? map->Texture->getWidthProperty() / map->TileSize.X : 1;

        map->CombatTextureName = ReadString(asset.get(), "CombatTextureName");
        map->CombatTexture = LoadTexture("Textures/Maps/Combat/" + map->CombatTextureName);

        map->MusicCueName = ReadString(asset.get(), "MusicCueName");
        map->CombatMusicCueName = ReadString(asset.get(), "CombatMusicCueName");

        map->BaseLayer = ReadIntArray(asset.get(), "BaseLayer");
        map->FringeLayer = ReadIntArray(asset.get(), "FringeLayer");
        map->ObjectLayer = ReadIntArray(asset.get(), "ObjectLayer");
        map->CollisionLayer = ReadIntArray(asset.get(), "CollisionLayer");

        if (auto* portals = asset->Child("Portals")) {
            for (auto* item : portals->Children("Item")) {
                auto portal = std::make_shared<Portal>();
                portal->Name = ReadString(item, "Name");
                portal->LandingMapPosition = ReadPoint(item, "LandingMapPosition");
                portal->DestinationMapContentName = ReadString(item, "DestinationMapContentName");
                portal->DestinationMapPortalName = ReadString(item, "DestinationMapPortalName");
                map->Portals.push_back(portal);
            }
        }

        if (auto* entries = asset->Child("PortalEntries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<MapEntry<Portal>>();
                ReadMapEntryBase(*entry, item);
                for (auto& p : map->Portals) {
                    if (p->Name == entry->ContentName) { entry->Content = p; break; }
                }
                map->PortalEntries.push_back(entry);
            }
        }

        if (auto* entries = asset->Child("ChestEntries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<MapEntry<Chest>>();
                ReadMapEntryBase(*entry, item);
                entry->Content = LoadChest(entry->ContentName)->Clone();
                map->ChestEntries.push_back(entry);
            }
        }

        if (auto* entries = asset->Child("FixedCombatEntries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<MapEntry<FixedCombat>>();
                ReadMapEntryBase(*entry, item);
                entry->Content = LoadFixedCombat(entry->ContentName);
                if (!entry->Content->Entries.empty() && entry->Content->Entries[0]->Content &&
                    entry->Content->Entries[0]->Content->MapSprite) {
                    entry->MapSprite = entry->Content->Entries[0]->Content->MapSprite->Clone();
                    entry->MapSprite->PlayAnimation("Idle", entry->EntryDirection);
                    entry->MapSprite->UpdateAnimation(4.0f * (float)random_.NextDouble());
                }
                map->FixedCombatEntries.push_back(entry);
            }
        }

        map->RandomCombatData = ReadRandomCombat(asset->Child("RandomCombat"));

        if (auto* entries = asset->Child("QuestNpcEntries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<MapEntry<QuestNpc>>();
                ReadMapEntryBase(*entry, item);
                entry->Content = LoadQuestNpc(entry->ContentName);
                entry->Content->MapPosition = entry->MapPosition;
                entry->Content->CharacterDirection = entry->EntryDirection;
                map->QuestNpcEntries.push_back(entry);
            }
        }

        if (auto* entries = asset->Child("PlayerNpcEntries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<MapEntry<Player>>();
                ReadMapEntryBase(*entry, item);
                entry->Content = LoadPlayer(entry->ContentName)->Clone();
                entry->Content->MapPosition = entry->MapPosition;
                entry->Content->CharacterDirection = entry->EntryDirection;
                map->PlayerNpcEntries.push_back(entry);
            }
        }

        if (auto* entries = asset->Child("InnEntries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<MapEntry<Inn>>();
                ReadMapEntryBase(*entry, item);
                entry->Content = LoadInn(entry->ContentName);
                map->InnEntries.push_back(entry);
            }
        }

        if (auto* entries = asset->Child("StoreEntries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<MapEntry<Store>>();
                ReadMapEntryBase(*entry, item);
                entry->Content = LoadStore(entry->ContentName);
                map->StoreEntries.push_back(entry);
            }
        }

        return map;
    }

    std::shared_ptr<Chest> LoadChest(const std::string& name) {
        std::string path = "Maps/Chests/" + name;
        if (auto it = chestCache_.find(path); it != chestCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        auto chest = std::make_shared<Chest>();
        chest->SetName(ReadString(asset.get(), "Name"));
        chest->Gold = ReadInt(asset.get(), "Gold");
        if (auto* entries = asset->Child("Entries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<ContentEntry<Gear>>();
                entry->ContentName = ReadString(item, "ContentName");
                entry->Count = ReadInt(item, "Count", 1);
                entry->Content = LoadGear("Gear/" + entry->ContentName);
                chest->Entries.push_back(entry);
            }
        }
        chest->TextureName = ReadString(asset.get(), "TextureName");
        chest->Texture = LoadTexture("Textures/Chests/" + chest->TextureName);
        chestCache_[path] = chest;
        return chest;
    }

    std::shared_ptr<FixedCombat> LoadFixedCombat(const std::string& name) {
        std::string path = "Maps/FixedCombats/" + name;
        if (auto it = fixedCombatCache_.find(path); it != fixedCombatCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        auto fixedCombat = std::make_shared<FixedCombat>();
        fixedCombat->SetName(ReadString(asset.get(), "Name"));
        if (auto* entries = asset->Child("Entries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<ContentEntry<Monster>>();
                entry->ContentName = ReadString(item, "ContentName");
                entry->Count = ReadInt(item, "Count", 1);
                entry->Content = LoadMonster(entry->ContentName);
                fixedCombat->Entries.push_back(entry);
            }
        }
        fixedCombatCache_[path] = fixedCombat;
        return fixedCombat;
    }

    std::shared_ptr<Inn> LoadInn(const std::string& name) {
        std::string path = "Maps/Inns/" + name;
        if (auto it = innCache_.find(path); it != innCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        auto inn = std::make_shared<Inn>();
        inn->SetName(ReadString(asset.get(), "Name"));
        inn->ChargePerPlayer = ReadInt(asset.get(), "ChargePerPlayer");
        inn->WelcomeMessage = ReadString(asset.get(), "WelcomeMessage");
        inn->PaidMessage = ReadString(asset.get(), "PaidMessage");
        inn->NotEnoughGoldMessage = ReadString(asset.get(), "NotEnoughGoldMessage");
        inn->ShopkeeperTextureName = ReadString(asset.get(), "ShopkeeperTextureName");
        inn->ShopkeeperTexture = LoadTexture("Textures/Characters/Portraits/" + inn->ShopkeeperTextureName);
        innCache_[path] = inn;
        return inn;
    }

    std::shared_ptr<Store> LoadStore(const std::string& name) {
        std::string path = "Maps/Stores/" + name;
        if (auto it = storeCache_.find(path); it != storeCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        auto store = std::make_shared<Store>();
        store->SetName(ReadString(asset.get(), "Name"));
        store->BuyMultiplier = ReadFloat(asset.get(), "BuyMultiplier", 1.0f);
        store->SellMultiplier = ReadFloat(asset.get(), "SellMultiplier", 1.0f);
        if (auto* categories = asset->Child("StoreCategories")) {
            for (auto* item : categories->Children("Item")) {
                StoreCategory category;
                category.Name = ReadString(item, "Name");
                category.AvailableContentNames = ReadStringList(item, "AvailableContentNames");
                for (auto& gearName : category.AvailableContentNames)
                    category.AvailableGear.push_back(LoadGear("Gear/" + gearName));
                store->StoreCategories.push_back(std::move(category));
            }
        }
        store->WelcomeMessage = ReadString(asset.get(), "WelcomeMessage");
        store->ShopkeeperTextureName = ReadString(asset.get(), "ShopkeeperTextureName");
        store->ShopkeeperTexture = LoadTexture("Textures/Characters/Portraits/" + store->ShopkeeperTextureName);
        storeCache_[path] = store;
        return store;
    }

    // ---- Quests ----

    std::shared_ptr<Quest> LoadQuest(const std::string& name) {
        std::string path = "Quests/" + name;
        if (auto it = questCache_.find(path); it != questCache_.end()) return it->second;
        auto asset = LoadAsset(path);
        auto quest = std::make_shared<Quest>();
        quest->SetAssetName(path);
        quest->Name = ReadString(asset.get(), "Name");
        quest->Description = ReadString(asset.get(), "Description");
        quest->ObjectiveMessage = ReadString(asset.get(), "ObjectiveMessage");
        quest->CompletionMessage = ReadString(asset.get(), "CompletionMessage");

        if (auto* reqs = asset->Child("GearRequirements")) {
            for (auto* item : reqs->Children("Item")) {
                auto req = std::make_shared<QuestRequirement<Gear>>();
                req->ContentName = ReadString(item, "ContentName");
                req->Count = ReadInt(item, "Count", 1);
                req->Content = LoadGear("Gear/" + req->ContentName);
                quest->GearRequirements.push_back(req);
            }
        }
        if (auto* reqs = asset->Child("MonsterRequirements")) {
            for (auto* item : reqs->Children("Item")) {
                auto req = std::make_shared<QuestRequirement<Monster>>();
                req->ContentName = ReadString(item, "ContentName");
                req->Count = ReadInt(item, "Count", 1);
                req->Content = LoadMonster(req->ContentName);
                quest->MonsterRequirements.push_back(req);
            }
        }

        if (auto* entries = asset->Child("FixedCombatEntries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<WorldEntry<FixedCombat>>();
                ReadMapEntryBase(*entry, item);
                entry->MapContentName = ReadString(item, "MapContentName");
                entry->Content = LoadFixedCombat(entry->ContentName);
                if (!entry->Content->Entries.empty() && entry->Content->Entries[0]->Content &&
                    entry->Content->Entries[0]->Content->MapSprite) {
                    entry->MapSprite = entry->Content->Entries[0]->Content->MapSprite->Clone();
                    entry->MapSprite->PlayAnimation("Idle", entry->EntryDirection);
                    entry->MapSprite->UpdateAnimation(4.0f * (float)random_.NextDouble());
                }
                quest->FixedCombatEntries.push_back(entry);
            }
        }
        if (auto* entries = asset->Child("ChestEntries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<WorldEntry<Chest>>();
                ReadMapEntryBase(*entry, item);
                entry->MapContentName = ReadString(item, "MapContentName");
                entry->Content = LoadChest(entry->ContentName)->Clone();
                quest->ChestEntries.push_back(entry);
            }
        }

        quest->DestinationMapContentName = ReadString(asset.get(), "DestinationMapContentName");
        quest->DestinationNpcContentName = ReadString(asset.get(), "DestinationNpcContentName");
        quest->DestinationObjectiveMessage = ReadString(asset.get(), "DestinationObjectiveMessage");
        quest->ExperienceReward = ReadInt(asset.get(), "ExperienceReward");
        quest->GoldReward = ReadInt(asset.get(), "GoldReward");
        quest->GearRewardContentNames = ReadStringList(asset.get(), "GearRewardContentNames");
        for (auto& gearName : quest->GearRewardContentNames)
            quest->GearRewards.push_back(LoadGear("Gear/" + gearName));

        questCache_[path] = quest;
        return quest;
    }

    std::shared_ptr<QuestLine> LoadQuestLine(const std::string& name) {
        std::string path = "Quests/QuestLines/" + name;
        auto asset = LoadAsset(path);
        auto line = std::make_shared<QuestLine>();
        line->SetAssetName(path);
        line->Name = ReadString(asset.get(), "Name");
        line->QuestContentNames = ReadStringList(asset.get(), "QuestContentNames");
        for (auto& questName : line->QuestContentNames) line->Quests.push_back(LoadQuest(questName));
        return line;
    }

    std::shared_ptr<GameStartDescription> LoadGameStartDescription(const std::string& name) {
        auto asset = LoadAsset(name);
        auto desc = std::make_shared<GameStartDescription>();
        desc->MapContentName = ReadString(asset.get(), "MapContentName");
        desc->PlayerContentNames = ReadStringList(asset.get(), "PlayerContentNames");
        desc->QuestLineContentName = ReadString(asset.get(), "QuestLineContentName");
        return desc;
    }

    std::shared_ptr<Texture2D> LoadTexture(const std::string& relativePathNoExt) {
        if (relativePathNoExt.empty() || relativePathNoExt.back() == '/') return nullptr;
        std::string resolved = ResolveCaseInsensitive(ToTexturePath(relativePathNoExt), ".png");
        return std::make_shared<Texture2D>(content_.Load<Texture2D>(resolved));
    }

private:
    // A handful of the original's data files reference an asset name whose
    // case doesn't match the actual shipped filename (e.g. XML text
    // "Warrior1InActive" vs. the real "Warrior1Inactive.png") -- harmless on
    // Windows' case-insensitive filesystem, fatal on this case-sensitive one.
    // Rather than hand-patch every mismatch as it's discovered, resolve
    // case-insensitively against the real directory listing when the exact
    // path doesn't exist. See missing.md.
    std::string ResolveCaseInsensitive(const std::string& relativePathNoExt, const std::string& extension) {
        std::string root = content_.getRootDirectoryProperty();
        std::filesystem::path exact = std::filesystem::path(root) / (relativePathNoExt + extension);
        if (std::filesystem::exists(exact)) return relativePathNoExt;

        std::filesystem::path relative(relativePathNoExt);
        std::filesystem::path dir = std::filesystem::path(root) / relative.parent_path();
        std::string wantedStem = relative.filename().string();
        std::string wantedLower = ToLower(wantedStem);

        if (!std::filesystem::exists(dir)) return relativePathNoExt;
        for (auto& entry : std::filesystem::directory_iterator(dir)) {
            std::string name = entry.path().filename().string();
            std::string stem = entry.path().stem().string();
            if (ToLower(stem) == wantedLower) {
                return (relative.parent_path() / stem).generic_string();
            }
        }
        return relativePathNoExt;
    }

    static std::string ToLower(const std::string& s) {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    // ---- Low-level XML reading ----

    std::shared_ptr<XmlNode> LoadAsset(const std::string& relativePathNoExt) {
        // Content names in the original XML are Windows paths (e.g.
        // "Items\MinorHealingPotion") -- normalize to '/' for this
        // case-sensitive filesystem (see missing.md).
        std::string normalized = ResolveCaseInsensitive(ToTexturePath(relativePathNoExt), ".xml");
        std::string diskPath = content_.getRootDirectoryProperty() + "/" + normalized + ".xml";
        std::ifstream file(diskPath, std::ios::binary);
        if (!file) throw std::runtime_error("ContentLoader: cannot open " + diskPath);
        std::ostringstream contents;
        contents << file.rdbuf();
        auto root = RolePlaying::Xml::ParseDocument(contents.str());
        auto* asset = root->Child("Asset");
        if (!asset) throw std::runtime_error("ContentLoader: no <Asset> in " + diskPath);
        return std::shared_ptr<XmlNode>(root, asset);
    }

    static std::string ToTexturePath(const std::string& contentPath) {
        std::string result = contentPath;
        std::replace(result.begin(), result.end(), '\\', '/');
        return result;
    }

    static std::string ReadString(XmlNode* parent, const char* tag, const std::string& def = "") {
        auto* c = parent->Child(tag);
        return c ? c->Text() : def;
    }
    static int ReadInt(XmlNode* parent, const char* tag, int def = 0) {
        auto* c = parent->Child(tag);
        return c && !c->Text().empty() ? std::stoi(c->Text()) : def;
    }
    static float ReadFloat(XmlNode* parent, const char* tag, float def = 0.0f) {
        auto* c = parent->Child(tag);
        return c && !c->Text().empty() ? std::stof(c->Text()) : def;
    }
    static bool ReadBool(XmlNode* parent, const char* tag, bool def = false) {
        auto* c = parent->Child(tag);
        if (!c || c->Text().empty()) return def;
        return c->Text() == "true";
    }
    // [Flags] enums (e.g. Item::ItemUsage) are serialized by the stock
    // IntermediateSerializer as comma-separated symbolic names (e.g.
    // "Combat,NonCombat"), not as an integer -- unlike the compiled binary
    // ContentTypeReader.Read(), which reads an already-resolved int. A
    // numeric value ("0") is also accepted since a few shipped XML files use
    // it directly. See missing.md.
    static int ReadItemUsage(XmlNode* parent, const char* tag, int def) {
        auto* c = parent->Child(tag);
        if (!c || c->Text().empty()) return def;
        const std::string& text = c->Text();
        if (std::all_of(text.begin(), text.end(), [](unsigned char ch) { return std::isdigit(ch); }))
            return std::stoi(text);
        int value = 0;
        std::istringstream iss(text);
        std::string flag;
        while (std::getline(iss, flag, ',')) {
            size_t start = flag.find_first_not_of(" \t");
            size_t end = flag.find_last_not_of(" \t");
            if (start == std::string::npos) continue;
            std::string trimmed = flag.substr(start, end - start + 1);
            if (trimmed == "Combat") value |= RolePlayingGameData::Item::Combat;
            else if (trimmed == "NonCombat") value |= RolePlayingGameData::Item::NonCombat;
        }
        return value != 0 ? value : def;
    }
    static Point ReadPoint(XmlNode* parent, const char* tag) {
        auto* c = parent->Child(tag);
        if (!c) return Point();
        std::istringstream iss(c->Text());
        int x = 0, y = 0;
        iss >> x >> y;
        return Point(x, y);
    }
    static Int32Range ReadInt32Range(XmlNode* node) {
        Int32Range r;
        if (!node) return r;
        r.Minimum = ReadInt(node, "Minimum");
        r.Maximum = ReadInt(node, "Maximum");
        return r;
    }
    static StatisticsValue ReadStatisticsValue(XmlNode* node) {
        StatisticsValue v;
        if (!node) return v;
        v.HealthPoints = ReadInt(node, "HealthPoints");
        v.MagicPoints = ReadInt(node, "MagicPoints");
        v.PhysicalOffense = ReadInt(node, "PhysicalOffense");
        v.PhysicalDefense = ReadInt(node, "PhysicalDefense");
        v.MagicalOffense = ReadInt(node, "MagicalOffense");
        v.MagicalDefense = ReadInt(node, "MagicalDefense");
        return v;
    }
    static StatisticsRange ReadStatisticsRange(XmlNode* node) {
        StatisticsRange r;
        if (!node) return r;
        r.HealthPointsRange = ReadInt32Range(node->Child("HealthPointsRange"));
        r.MagicPointsRange = ReadInt32Range(node->Child("MagicPointsRange"));
        r.PhysicalOffenseRange = ReadInt32Range(node->Child("PhysicalOffenseRange"));
        r.PhysicalDefenseRange = ReadInt32Range(node->Child("PhysicalDefenseRange"));
        r.MagicalOffenseRange = ReadInt32Range(node->Child("MagicalOffenseRange"));
        r.MagicalDefenseRange = ReadInt32Range(node->Child("MagicalDefenseRange"));
        return r;
    }
    static std::vector<std::string> ReadStringList(XmlNode* parent, const char* tag) {
        std::vector<std::string> result;
        auto* c = parent->Child(tag);
        if (!c) return result;
        for (auto* item : c->Children("Item")) result.push_back(item->Text());
        return result;
    }
    static std::vector<int> ReadIntArray(XmlNode* parent, const char* tag) {
        std::vector<int> result;
        auto* c = parent->Child(tag);
        if (!c) return result;
        std::istringstream iss(c->Text());
        int value;
        while (iss >> value) result.push_back(value);
        return result;
    }

    std::shared_ptr<AnimatingSprite> ReadAnimatingSprite(XmlNode* parent, const char* tag) {
        auto* node = parent->Child(tag);
        if (!node) return nullptr;
        auto sprite = std::make_shared<AnimatingSprite>();
        sprite->TextureName = ReadString(node, "TextureName");
        sprite->Texture = LoadTexture("Textures/" + ToTexturePath(sprite->TextureName));
        sprite->SetFrameDimensions(ReadPoint(node, "FrameDimensions"));
        sprite->FramesPerRow = ReadInt(node, "FramesPerRow", 1);
        if (node->Child("SourceOffset")) sprite->SourceOffset = ReadVector2(node, "SourceOffset");
        if (auto* anims = node->Child("Animations")) {
            for (auto* item : anims->Children("Item")) {
                auto anim = std::make_shared<Animation>();
                anim->Name = ReadString(item, "Name");
                anim->StartingFrame = ReadInt(item, "StartingFrame");
                anim->EndingFrame = ReadInt(item, "EndingFrame");
                anim->Interval = ReadInt(item, "Interval");
                anim->IsLoop = ReadBool(item, "IsLoop");
                sprite->Animations.push_back(anim);
            }
        }
        return sprite;
    }

    static Vector2 ReadVector2(XmlNode* parent, const char* tag) {
        auto* c = parent->Child(tag);
        if (!c) return Vector2::Zero;
        std::istringstream iss(c->Text());
        float x = 0, y = 0;
        iss >> x >> y;
        return Vector2(x, y);
    }

    static void CenterOverlaySourceOffset(const std::shared_ptr<AnimatingSprite>& sprite) {
        if (!sprite) return;
        sprite->SourceOffset = Vector2((float)sprite->FrameDimensions().X / 2.0f, (float)sprite->FrameDimensions().Y);
    }

    void PopulateGearBase(Gear& gear, XmlNode* asset, const std::string& assetPath) {
        gear.SetAssetName(assetPath);
        gear.Name = ReadString(asset, "Name");
        gear.Description = ReadString(asset, "Description");
        gear.GoldValue = ReadInt(asset, "GoldValue");
        gear.IsDroppable = ReadBool(asset, "IsDroppable");
        gear.MinimumCharacterLevel = ReadInt(asset, "MinimumCharacterLevel");
        gear.SupportedClasses = ReadStringList(asset, "SupportedClasses");
        gear.IconTextureName = ReadString(asset, "IconTextureName");
        gear.IconTexture = LoadTexture("Textures/Gear/" + gear.IconTextureName);
    }
    void PopulateEquipmentBase(Equipment& equipment, XmlNode* asset, const std::string& assetPath) {
        PopulateGearBase(equipment, asset, assetPath);
        equipment.OwnerBuffStatistics = ReadStatisticsValue(asset->Child("OwnerBuffStatistics"));
    }

    std::shared_ptr<Armor> ReadArmor(XmlNode* asset, const std::string& assetPath) {
        auto armor = std::make_shared<Armor>();
        PopulateEquipmentBase(*armor, asset, assetPath);
        armor->Slot = ArmorSlotFromString(ReadString(asset, "Slot"));
        armor->OwnerHealthDefenseRange = ReadInt32Range(asset->Child("OwnerHealthDefenseRange"));
        armor->OwnerMagicDefenseRange = ReadInt32Range(asset->Child("OwnerMagicDefenseRange"));
        return armor;
    }
    std::shared_ptr<Weapon> ReadWeapon(XmlNode* asset, const std::string& assetPath) {
        auto weapon = std::make_shared<Weapon>();
        PopulateEquipmentBase(*weapon, asset, assetPath);
        weapon->TargetDamageRange = ReadInt32Range(asset->Child("TargetDamageRange"));
        weapon->SwingCueName = ReadString(asset, "SwingCueName");
        weapon->HitCueName = ReadString(asset, "HitCueName");
        weapon->BlockCueName = ReadString(asset, "BlockCueName");
        weapon->Overlay = ReadAnimatingSprite(asset, "Overlay");
        CenterOverlaySourceOffset(weapon->Overlay);
        return weapon;
    }
    std::shared_ptr<Item> ReadItem(XmlNode* asset, const std::string& assetPath) {
        auto item = std::make_shared<Item>();
        PopulateGearBase(*item, asset, assetPath);
        item->Usage = ReadItemUsage(asset, "Usage", Item::Combat | Item::NonCombat);
        item->IsOffensive = ReadBool(asset, "IsOffensive");
        item->TargetDuration = ReadInt(asset, "TargetDuration");
        item->TargetEffectRange = ReadStatisticsRange(asset->Child("TargetEffectRange"));
        item->AdjacentTargets = ReadInt(asset, "AdjacentTargets");
        item->UsingCueName = ReadString(asset, "UsingCueName");
        item->TravelingCueName = ReadString(asset, "TravelingCueName");
        item->ImpactCueName = ReadString(asset, "ImpactCueName");
        item->BlockCueName = ReadString(asset, "BlockCueName");
        item->CreationSprite = ReadAnimatingSprite(asset, "CreationSprite");
        CenterOverlaySourceOffset(item->CreationSprite);
        item->SpellSprite = ReadAnimatingSprite(asset, "SpellSprite");
        CenterOverlaySourceOffset(item->SpellSprite);
        item->Overlay = ReadAnimatingSprite(asset, "Overlay");
        CenterOverlaySourceOffset(item->Overlay);
        return item;
    }

    void ReadCharacterBase(Character& character, XmlNode* asset) {
        character.SetName(ReadString(asset, "Name"));
        character.MapIdleAnimationInterval = ReadInt(asset, "MapIdleAnimationInterval", 200);
        character.MapSprite = ReadAnimatingSprite(asset, "MapSprite");
        if (character.MapSprite) {
            character.MapSprite->SourceOffset =
                Vector2(character.MapSprite->SourceOffset.X - 32, character.MapSprite->SourceOffset.Y - 32);
        }
        character.AddStandardCharacterIdleAnimations();
        character.MapWalkingAnimationInterval = ReadInt(asset, "MapWalkingAnimationInterval", 80);
        character.WalkingSprite = ReadAnimatingSprite(asset, "WalkingSprite");
        if (character.WalkingSprite) {
            character.WalkingSprite->SourceOffset =
                Vector2(character.WalkingSprite->SourceOffset.X - 32, character.WalkingSprite->SourceOffset.Y - 32);
        }
        character.AddStandardCharacterWalkingAnimations();
        character.ResetAnimation(false);
        character.ShadowTexture = LoadTexture("Textures/Characters/CharacterShadow");
    }

    void ReadFightingCharacterBase(FightingCharacter& fc, XmlNode* asset, const std::string& assetPath) {
        ReadCharacterBase(fc, asset);
        fc.SetAssetName(assetPath);
        fc.CharacterClassContentName = ReadString(asset, "CharacterClassContentName");
        fc.SetCharacterLevel(ReadInt(asset, "CharacterLevel", 1));
        fc.InitialEquipmentContentNames = ReadStringList(asset, "InitialEquipmentContentNames");
        if (auto* inv = asset->Child("Inventory")) {
            for (auto* item : inv->Children("Item")) {
                auto entry = std::make_shared<ContentEntry<Gear>>();
                entry->ContentName = ReadString(item, "ContentName");
                entry->Count = ReadInt(item, "Count", 1);
                entry->Content = LoadGear("Gear/" + entry->ContentName);
                fc.Inventory.push_back(entry);
            }
        }
        fc.CombatAnimationInterval = ReadInt(asset, "CombatAnimationInterval", 100);
        fc.CombatSprite = ReadAnimatingSprite(asset, "CombatSprite");
        fc.AddStandardCharacterCombatAnimations();
        fc.ResetAnimation(false);

        fc.SetCharacterClass(LoadCharacterClass(fc.CharacterClassContentName));
        for (auto& gearName : fc.InitialEquipmentContentNames) {
            auto equipment = std::dynamic_pointer_cast<Equipment>(LoadGear("Gear/" + gearName));
            if (equipment) fc.Equip(equipment);
        }
        fc.RecalculateEquipmentStatistics();
        fc.RecalculateTotalTargetDamageRange();
        fc.RecalculateTotalDefenseRanges();
    }

    template <typename T>
    void ReadMapEntryBase(MapEntry<T>& entry, XmlNode* item) {
        entry.ContentName = ReadString(item, "ContentName");
        entry.Count = ReadInt(item, "Count", 1);
        entry.MapPosition = ReadPoint(item, "MapPosition");
        entry.EntryDirection = DirectionFromString(ReadString(item, "Direction", "South"));
    }

    std::shared_ptr<RandomCombat> ReadRandomCombat(XmlNode* node) {
        auto rc = std::make_shared<RandomCombat>();
        if (!node) return rc;
        rc->CombatProbability = ReadInt(node, "CombatProbability");
        rc->FleeProbability = ReadInt(node, "FleeProbability");
        rc->MonsterCountRange = ReadInt32Range(node->Child("MonsterCountRange"));
        if (auto* entries = node->Child("Entries")) {
            for (auto* item : entries->Children("Item")) {
                auto entry = std::make_shared<WeightedContentEntry<Monster>>();
                entry->ContentName = ReadString(item, "ContentName");
                entry->Count = ReadInt(item, "Count", 1);
                entry->Weight = ReadInt(item, "Weight", 1);
                entry->Content = LoadMonster(entry->ContentName);
                rc->Entries.push_back(entry);
            }
        }
        return rc;
    }

    ContentManager& content_;
    System::Random random_;

    std::map<std::string, std::shared_ptr<Gear>> gearCache_;
    std::map<std::string, std::shared_ptr<Spell>> spellCache_;
    std::map<std::string, std::shared_ptr<CharacterClass>> classCache_;
    std::map<std::string, std::shared_ptr<Monster>> monsterCache_;
    std::map<std::string, std::shared_ptr<Player>> playerCache_;
    std::map<std::string, std::shared_ptr<QuestNpc>> questNpcCache_;
    std::map<std::string, std::shared_ptr<Chest>> chestCache_;
    std::map<std::string, std::shared_ptr<FixedCombat>> fixedCombatCache_;
    std::map<std::string, std::shared_ptr<Inn>> innCache_;
    std::map<std::string, std::shared_ptr<Store>> storeCache_;
    std::map<std::string, std::shared_ptr<Quest>> questCache_;
};

} // namespace RolePlayingGameData

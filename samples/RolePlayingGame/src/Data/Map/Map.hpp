#pragma once

// Map.hpp -- C++ port of RolePlayingGameData/Map/Map.cs.

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"

#include "../Characters/Player.hpp"
#include "../Characters/QuestNpc.hpp"
#include "../ContentObject.hpp"
#include "../MapEntry.hpp"
#include "Chest.hpp"
#include "FixedCombat.hpp"
#include "Inn.hpp"
#include "Portal.hpp"
#include "RandomCombat.hpp"
#include "Store.hpp"

namespace RolePlayingGameData {

using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;

// One section of the world, and all of the data in it.
class Map : public ContentObject {
public:
    std::string Name;

    Point MapDimensions;
    Point TileSize;
    int TilesPerRow = 0;

    // A valid spawn position for this map.
    Point SpawnMapPosition;

    std::string TextureName;
    std::shared_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> Texture;
    std::string CombatTextureName;
    std::shared_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> CombatTexture;

    std::string MusicCueName;
    std::string CombatMusicCueName;

    std::vector<int> BaseLayer;
    std::vector<int> FringeLayer;
    std::vector<int> ObjectLayer;
    std::vector<int> CollisionLayer;

    int GetBaseLayerValue(Point mapPosition) const { return LayerValue(BaseLayer, mapPosition); }
    int GetFringeLayerValue(Point mapPosition) const { return LayerValue(FringeLayer, mapPosition); }
    int GetObjectLayerValue(Point mapPosition) const { return LayerValue(ObjectLayer, mapPosition); }
    int GetCollisionLayerValue(Point mapPosition) const { return LayerValue(CollisionLayer, mapPosition); }

    Rectangle GetBaseLayerSourceRectangle(Point mapPosition) const { return LayerSourceRectangle(BaseLayer, mapPosition); }
    Rectangle GetFringeLayerSourceRectangle(Point mapPosition) const { return LayerSourceRectangle(FringeLayer, mapPosition); }
    Rectangle GetObjectLayerSourceRectangle(Point mapPosition) const { return LayerSourceRectangle(ObjectLayer, mapPosition); }

    bool IsBlocked(Point mapPosition) const {
        if (mapPosition.X < 0 || mapPosition.X >= MapDimensions.X || mapPosition.Y < 0 ||
            mapPosition.Y >= MapDimensions.Y)
            return true;
        return GetCollisionLayerValue(mapPosition) != 0;
    }

    std::vector<std::shared_ptr<Portal>> Portals;
    std::vector<std::shared_ptr<MapEntry<Portal>>> PortalEntries;

    std::shared_ptr<MapEntry<Portal>> FindPortal(const std::string& name) const {
        for (auto& entry : PortalEntries) {
            if (entry->ContentName == name) return entry;
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<MapEntry<Chest>>> ChestEntries;
    std::vector<std::shared_ptr<MapEntry<FixedCombat>>> FixedCombatEntries;
    std::shared_ptr<RandomCombat> RandomCombatData;
    std::vector<std::shared_ptr<MapEntry<QuestNpc>>> QuestNpcEntries;
    std::vector<std::shared_ptr<MapEntry<Player>>> PlayerNpcEntries;
    std::vector<std::shared_ptr<MapEntry<Inn>>> InnEntries;
    std::vector<std::shared_ptr<MapEntry<Store>>> StoreEntries;

    std::shared_ptr<Map> Clone() const {
        auto map = std::make_shared<Map>();
        map->SetAssetName(AssetName());
        map->BaseLayer = BaseLayer;
        for (auto& chestEntry : ChestEntries) {
            auto mapEntry = std::make_shared<MapEntry<Chest>>();
            mapEntry->Content = chestEntry->Content ? chestEntry->Content->Clone() : nullptr;
            mapEntry->ContentName = chestEntry->ContentName;
            mapEntry->Count = chestEntry->Count;
            mapEntry->EntryDirection = chestEntry->EntryDirection;
            mapEntry->MapPosition = chestEntry->MapPosition;
            map->ChestEntries.push_back(mapEntry);
        }
        map->CollisionLayer = CollisionLayer;
        map->CombatMusicCueName = CombatMusicCueName;
        map->CombatTexture = CombatTexture;
        map->CombatTextureName = CombatTextureName;
        map->FixedCombatEntries = FixedCombatEntries;
        map->FringeLayer = FringeLayer;
        map->InnEntries = InnEntries;
        map->MapDimensions = MapDimensions;
        map->MusicCueName = MusicCueName;
        map->Name = Name;
        map->ObjectLayer = ObjectLayer;
        map->PlayerNpcEntries = PlayerNpcEntries;
        map->Portals = Portals;
        map->PortalEntries = PortalEntries;
        map->QuestNpcEntries = QuestNpcEntries;
        map->RandomCombatData = std::make_shared<RandomCombat>();
        if (RandomCombatData) {
            map->RandomCombatData->CombatProbability = RandomCombatData->CombatProbability;
            map->RandomCombatData->Entries = RandomCombatData->Entries;
            map->RandomCombatData->FleeProbability = RandomCombatData->FleeProbability;
            map->RandomCombatData->MonsterCountRange = RandomCombatData->MonsterCountRange;
        }
        map->SpawnMapPosition = SpawnMapPosition;
        map->StoreEntries = StoreEntries;
        map->Texture = Texture;
        map->TextureName = TextureName;
        map->TileSize = TileSize;
        map->TilesPerRow = TilesPerRow;
        return map;
    }

private:
    int LayerValue(const std::vector<int>& layer, Point mapPosition) const {
        if (mapPosition.X < 0 || mapPosition.X >= MapDimensions.X || mapPosition.Y < 0 ||
            mapPosition.Y >= MapDimensions.Y)
            throw std::out_of_range("mapPosition");
        return layer[mapPosition.Y * MapDimensions.X + mapPosition.X];
    }

    Rectangle LayerSourceRectangle(const std::vector<int>& layer, Point mapPosition) const {
        if (mapPosition.X < 0 || mapPosition.X >= MapDimensions.X || mapPosition.Y < 0 ||
            mapPosition.Y >= MapDimensions.Y)
            return Rectangle::Empty;
        int value = LayerValue(layer, mapPosition);
        if (value < 0) return Rectangle::Empty;
        return Rectangle((value % TilesPerRow) * TileSize.X, (value / TilesPerRow) * TileSize.Y, TileSize.X,
                          TileSize.Y);
    }
};

} // namespace RolePlayingGameData

#pragma once
#include <algorithm>
#include <optional>
#include <vector>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "MapData.hpp"

namespace Pathfinding {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

enum class MapTileType { MapEmpty, MapBarrier, MapStart, MapExit };

class Map {
    std::optional<Texture2D> tileTexture_;
    Vector2 tileSquareCenter_;
    std::optional<Texture2D> dotTexture_;
    Vector2 dotTextureCenter_;
    std::optional<Texture2D> barrierTexture_;


    std::vector<MapData>           maps_;
    std::vector<std::vector<MapTileType>> mapTiles_; // [col][row]
    int currentMap_    = 0;
    int numberColumns_ = 0;
    int numberRows_    = 0;

    float tileSize_ = 0.0f;
    float scale_    = 0.0f;
    bool  mapReload_= false;

    Point startTile_;
    Point endTile_;

public:
    float TileSize() const { return tileSize_; }
    float Scale()    const { return scale_; }
    Point StartTile()const { return startTile_; }
    Point EndTile()  const { return endTile_; }
    bool  MapReload()const { return mapReload_; }
    void  SetMapReload(bool v){ mapReload_ = v; }

    void LoadContent(Content::ContentManager& content) {
        tileTexture_.emplace(content.Load<Texture2D>("whiteTile"));
        barrierTexture_.emplace(content.Load<Texture2D>("barrier"));
        dotTexture_.emplace(content.Load<Texture2D>("dot"));

        dotTextureCenter_ = Vector2(
            (float)(dotTexture_->getWidthProperty()  / 2),
            (float)(dotTexture_->getHeightProperty() / 2));

        std::string root = content.getRootDirectoryProperty();
        maps_.push_back(ParseMapXML(root + "/Map1.xml"));
        maps_.push_back(ParseMapXML(root + "/Map2.xml"));
        maps_.push_back(ParseMapXML(root + "/Map3.xml"));
        maps_.push_back(ParseMapXML(root + "/Map4.xml"));

        ReloadMap();
        mapReload_ = true;
    }

    // Draw assumes spriteBatch is already begun
    void Draw(SpriteBatch& spriteBatch) {
        const Color tileColor1(  0,   0, 128, 255); // Navy
        const Color tileColor2(173, 216, 230, 255); // LightBlue
        const Color startColor(  0, 128,   0, 255); // Green
        const Color exitColor (255,   0,   0, 255); // Red

        for (int i = 0; i < numberRows_; i++) {
            for (int j = 0; j < numberColumns_; j++) {
                Vector2 tilePos = MapToWorld(j, i, false);
                Color currentColor = ((i + j) % 2 == 1) ? tileColor1 : tileColor2;

                spriteBatch.Draw(*tileTexture_, tilePos, std::nullopt,
                    currentColor, 0.0f, Vector2::Zero, scale_,
                    SpriteEffects::None, 0.0f);

                switch (mapTiles_[j][i]) {
                    case MapTileType::MapBarrier:
                        spriteBatch.Draw(*barrierTexture_, tilePos, std::nullopt,
                            Color(255,255,255,255), 0.0f, Vector2::Zero, scale_,
                            SpriteEffects::None, 0.25f);
                        break;
                    case MapTileType::MapStart:
                        spriteBatch.Draw(*dotTexture_, tilePos + tileSquareCenter_,
                            std::nullopt, startColor, 0.0f, dotTextureCenter_, scale_,
                            SpriteEffects::None, 0.25f);
                        break;
                    case MapTileType::MapExit:
                        spriteBatch.Draw(*dotTexture_, tilePos + tileSquareCenter_,
                            std::nullopt, exitColor, 0.0f, dotTextureCenter_, scale_,
                            SpriteEffects::None, 0.25f);
                        break;
                    default: break;
                }
            }
        }
    }

    Vector2 MapToWorld(int col, int row, bool centered) const {
        if (!InMap(col, row)) return Vector2::Zero;
        Vector2 pos((float)(col * tileSize_), (float)(row * tileSize_));
        if (centered) pos = pos + tileSquareCenter_;
        return pos;
    }

    Vector2 MapToWorld(Point location, bool centered) const {
        return MapToWorld(location.X, location.Y, centered);
    }

    static int StepDistance(Point a, Point b) {
        return std::abs(a.X - b.X) + std::abs(a.Y - b.Y);
    }

    int StepDistanceToEnd(Point point) const {
        return StepDistance(point, endTile_);
    }

    std::vector<Point> OpenMapTiles(Point loc) const {
        std::vector<Point> result;
        if (IsOpen(loc.X, loc.Y + 1)) result.push_back(Point(loc.X, loc.Y + 1));
        if (IsOpen(loc.X, loc.Y - 1)) result.push_back(Point(loc.X, loc.Y - 1));
        if (IsOpen(loc.X + 1, loc.Y)) result.push_back(Point(loc.X + 1, loc.Y));
        if (IsOpen(loc.X - 1, loc.Y)) result.push_back(Point(loc.X - 1, loc.Y));
        return result;
    }

    void UpdateMapViewport(const Rectangle& safeArea) {
        tileSize_ = std::min(
            (float)safeArea.Height / (float)numberRows_,
            (float)safeArea.Width  / (float)numberColumns_);
        scale_           = tileSize_ / (float)tileTexture_->getHeightProperty();
        tileSquareCenter_= Vector2(tileSize_ / 2.0f, tileSize_ / 2.0f);
    }

    void CycleMap() {
        currentMap_ = (currentMap_ + 1) % (int)maps_.size();
        mapReload_  = true;
    }

    void ReloadMap() {
        numberColumns_ = maps_[currentMap_].NumberColumns;
        numberRows_    = maps_[currentMap_].NumberRows;

        mapTiles_.assign(numberColumns_, std::vector<MapTileType>(numberRows_, MapTileType::MapEmpty));

        startTile_ = maps_[currentMap_].Start;
        mapTiles_[startTile_.X][startTile_.Y] = MapTileType::MapStart;

        endTile_ = maps_[currentMap_].End;
        mapTiles_[endTile_.X][endTile_.Y] = MapTileType::MapExit;

        for (const Point& b : maps_[currentMap_].Barriers)
            mapTiles_[b.X][b.Y] = MapTileType::MapBarrier;

        mapReload_ = false;
    }

private:
    bool InMap(int col, int row) const {
        return row >= 0 && row < numberRows_ && col >= 0 && col < numberColumns_;
    }
    bool IsOpen(int col, int row) const {
        return InMap(col, row) && mapTiles_[col][row] != MapTileType::MapBarrier;
    }
};

} // namespace Pathfinding

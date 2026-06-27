#pragma once
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <vector>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Map.hpp"

namespace Pathfinding {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

enum class SearchStatus { Stopped, Searching, NoPath, PathFound };
enum class SearchMethod { BreadthFirst, BestFirst, AStar, Max };

class PathFinder {
    struct SearchNode {
        Point Position;
        int   DistanceToGoal    = 0;
        int   DistanceTraveled  = 0;
        SearchNode() = default;
        SearchNode(Point p, int dtg, int dt)
            : Position(p), DistanceToGoal(dtg), DistanceTraveled(dt) {}
    };

    static constexpr float searchNodeDrawScale = 0.75f;

    std::optional<Texture2D> nodeTexture_;
    Vector2 nodeTextureCenter_;

    float timeSinceLastSearchStep_ = 0.0f;
    std::vector<SearchNode> openList_;
    std::vector<SearchNode> closedList_;
    std::map<std::pair<int,int>, std::pair<int,int>> paths_;
    Map*  map_  = nullptr;
    float scale_= 0.0f;

    SearchStatus searchStatus_ = SearchStatus::Stopped;
    SearchMethod searchMethod_ = SearchMethod::BestFirst;
    float timeStep_            = 0.5f;
    int   totalSearchSteps_    = 0;

public:
    SearchStatus GetSearchStatus() const { return searchStatus_; }
    SearchMethod GetSearchMethod() const { return searchMethod_; }
    float TimeStep()  const { return timeStep_; }
    void  SetTimeStep(float v) { timeStep_ = v; }
    int   TotalSearchSteps() const { return totalSearchSteps_; }

    bool IsSearching() const { return searchStatus_ == SearchStatus::Searching; }
    void SetIsSearching(bool v) {
        if (searchStatus_ == SearchStatus::Searching)      searchStatus_ = SearchStatus::Stopped;
        else if (searchStatus_ == SearchStatus::Stopped)   searchStatus_ = SearchStatus::Searching;
    }

    void SetScale(float v) { scale_ = v * searchNodeDrawScale; }

    void Initialize(Map& m) {
        map_ = &m;
        searchStatus_ = SearchStatus::Stopped;
        openList_.clear();
        closedList_.clear();
        paths_.clear();
    }

    void LoadContent(Content::ContentManager& content) {
        nodeTexture_.emplace(content.Load<Texture2D>("dot"));
        nodeTextureCenter_ = Vector2(
            (float)(nodeTexture_->getWidthProperty()  / 2),
            (float)(nodeTexture_->getHeightProperty() / 2));
    }

    void Update(const GameTime& gameTime) {
        if (searchStatus_ == SearchStatus::Searching) {
            timeSinceLastSearchStep_ +=
                (float)gameTime.getElapsedGameTimeProperty().getTotalSecondsProperty();
            if (timeSinceLastSearchStep_ >= timeStep_) {
                DoSearchStep();
                timeSinceLastSearchStep_ = 0.0f;
            }
        }
    }

    // Draw assumes spriteBatch is already begun
    void Draw(SpriteBatch& spriteBatch) {
        if (searchStatus_ == SearchStatus::PathFound) return;
        const Color openColor  (  0, 128,   0, 255); // Green
        const Color closedColor(255,   0,   0, 255); // Red
        for (const SearchNode& node : openList_) {
            spriteBatch.Draw(*nodeTexture_,
                map_->MapToWorld(node.Position, true), std::nullopt,
                openColor, 0.0f, nodeTextureCenter_, scale_,
                SpriteEffects::None, 0.0f);
        }
        for (const SearchNode& node : closedList_) {
            spriteBatch.Draw(*nodeTexture_,
                map_->MapToWorld(node.Position, true), std::nullopt,
                closedColor, 0.0f, nodeTextureCenter_, scale_,
                SpriteEffects::None, 0.0f);
        }
    }

    void Reset() {
        searchStatus_      = SearchStatus::Stopped;
        totalSearchSteps_  = 0;
        SetScale(map_->Scale());
        openList_.clear();
        closedList_.clear();
        paths_.clear();
        openList_.push_back(SearchNode(map_->StartTile(),
            Map::StepDistance(map_->StartTile(), map_->EndTile()), 0));
    }

    void NextSearchType() {
        searchMethod_ = static_cast<SearchMethod>(
            ((int)searchMethod_ + 1) % (int)SearchMethod::Max);
    }

    std::list<Point> FinalPath() const {
        std::list<Point> path;
        if (searchStatus_ == SearchStatus::PathFound) {
            Point cur = map_->EndTile();
            path.push_front(cur);
            auto key = std::make_pair(cur.X, cur.Y);
            while (paths_.count(key)) {
                auto& prev = paths_.at(key);
                cur = Point(prev.first, prev.second);
                path.push_front(cur);
                key = prev;
            }
        }
        return path;
    }

private:
    void DoSearchStep() {
        SearchNode newNode;
        if (!SelectNodeToVisit(newNode)) {
            searchStatus_ = SearchStatus::NoPath;
            return;
        }

        Point cur = newNode.Position;
        for (const Point& pt : map_->OpenMapTiles(cur)) {
            SearchNode tile(pt, map_->StepDistanceToEnd(pt),
                            newNode.DistanceTraveled + 1);
            auto key = std::make_pair(pt.X, pt.Y);
            if (!InList(openList_, pt) && !InList(closedList_, pt)) {
                openList_.push_back(tile);
                paths_[key] = std::make_pair(cur.X, cur.Y);
            }
        }
        if (cur == map_->EndTile())
            searchStatus_ = SearchStatus::PathFound;

        openList_.erase(std::remove_if(openList_.begin(), openList_.end(),
            [&](const SearchNode& n){ return n.Position == newNode.Position; }),
            openList_.end());
        closedList_.push_back(newNode);
    }

    static bool InList(const std::vector<SearchNode>& list, Point pt) {
        for (const auto& n : list)
            if (n.Position == pt) return true;
        return false;
    }

    bool SelectNodeToVisit(SearchNode& result) {
        if (openList_.empty()) return false;
        totalSearchSteps_++;

        float smallest = std::numeric_limits<float>::infinity();
        bool  found    = false;

        switch (searchMethod_) {
            case SearchMethod::BreadthFirst:
                result = openList_.front();
                return true;

            case SearchMethod::BestFirst:
                for (const SearchNode& n : openList_) {
                    float d = (float)n.DistanceToGoal;
                    if (d < smallest) { smallest = d; result = n; found = true; }
                }
                return found;

            case SearchMethod::AStar:
                for (const SearchNode& n : openList_) {
                    float d = (float)(n.DistanceTraveled + n.DistanceToGoal);
                    if (d < smallest ||
                        (d == smallest && n.DistanceTraveled > result.DistanceTraveled)) {
                        smallest = d; result = n; found = true;
                    }
                }
                return found;

            default: return false;
        }
    }

    static float Heuristic(const SearchNode& n) {
        return (float)(n.DistanceTraveled + n.DistanceToGoal);
    }
};

} // namespace Pathfinding

#pragma once

// TileEngine.hpp -- C++ port of TileEngine/TileEngine.cs.

#include <cmath>
#include <memory>

#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/Viewport.hpp"
#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Point.hpp"
#include "Microsoft/Xna/Framework/Rectangle.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"

#include "../Data/Map/Map.hpp"
#include "../Data/Map/Portal.hpp"
#include "../Data/MapEntry.hpp"
#include "../InputManager.hpp"
#include "PlayerPosition.hpp"

namespace RolePlaying {

using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Point;
using Microsoft::Xna::Framework::Rectangle;
using Microsoft::Xna::Framework::Vector2;
using Microsoft::Xna::Framework::Graphics::SpriteBatch;
using Microsoft::Xna::Framework::Graphics::Viewport;

// Static class for a tileable map.
class TileEngine {
public:
    static std::shared_ptr<RolePlayingGameData::Map> Map() { return map_; }

    static Vector2 GetScreenPosition(Point mapPosition) {
        return Vector2(mapOriginPosition_.X + mapPosition.X * map_->TileSize.X,
                        mapOriginPosition_.Y + mapPosition.Y * map_->TileSize.Y);
    }

    // Set the map in use by the tile engine. portalEntry may be null.
    static void SetMap(const std::shared_ptr<RolePlayingGameData::Map>& newMap,
                        const std::shared_ptr<RolePlayingGameData::MapEntry<RolePlayingGameData::Portal>>& portalEntry) {
        map_ = newMap;
        mapOriginPosition_ = Vector2::Zero;

        if (!portalEntry) {
            partyLeaderPosition_.TilePosition = map_->SpawnMapPosition;
            partyLeaderPosition_.TileOffset = Vector2::Zero;
            partyLeaderPosition_.PositionDirection = RolePlayingGameData::Direction::South;
        } else {
            partyLeaderPosition_.TilePosition = portalEntry->MapPosition;
            partyLeaderPosition_.TileOffset = Vector2::Zero;
            partyLeaderPosition_.PositionDirection = portalEntry->EntryDirection;
            autoPartyLeaderMovement_ = Vector2::Multiply(
                Vector2((float)map_->TileSize.X, (float)map_->TileSize.Y),
                Vector2((float)(portalEntry->Content->LandingMapPosition.X - partyLeaderPosition_.TilePosition.X),
                        (float)(portalEntry->Content->LandingMapPosition.Y - partyLeaderPosition_.TilePosition.Y)));
        }
    }

    static Viewport CurrentViewport() { return viewport_; }
    static void SetViewport(Viewport v) {
        viewport_ = v;
        viewportCenter_ = Vector2(v.getXProperty() + v.getWidthProperty() / 2.0f,
                                  v.getYProperty() + v.getHeightProperty() / 2.0f);
    }

    static PlayerPosition& PartyLeaderPosition() { return partyLeaderPosition_; }

    // Update the tile engine. Calls out to Session for random-combat/encounter
    // checks -- Session.hpp forward-declares and defines those calls at the
    // bottom of its own header (mutual dependency: Session needs TileEngine,
    // TileEngine::Update needs Session).
    static void Update(const GameTime& gameTime);

    static bool CheckVisibility(Rectangle screenRectangle) {
        return screenRectangle.X > viewport_.getXProperty() - screenRectangle.Width &&
               screenRectangle.Y > viewport_.getYProperty() - screenRectangle.Height &&
               screenRectangle.X < viewport_.getXProperty() + viewport_.getWidthProperty() &&
               screenRectangle.Y < viewport_.getYProperty() + viewport_.getHeightProperty();
    }

    static void DrawLayers(SpriteBatch& spriteBatch, bool drawBase, bool drawFringe, bool drawObject) {
        if (!drawBase && !drawFringe && !drawObject) return;

        Rectangle destinationRectangle(0, 0, map_->TileSize.X, map_->TileSize.Y);

        for (int y = 0; y < map_->MapDimensions.Y; y++) {
            for (int x = 0; x < map_->MapDimensions.X; x++) {
                destinationRectangle.X = (int)mapOriginPosition_.X + x * map_->TileSize.X;
                destinationRectangle.Y = (int)mapOriginPosition_.Y + y * map_->TileSize.Y;

                if (CheckVisibility(destinationRectangle)) {
                    Point mapPosition(x, y);
                    if (drawBase) {
                        Rectangle src = map_->GetBaseLayerSourceRectangle(mapPosition);
                        if (src != Rectangle::Empty)
                            spriteBatch.Draw(*map_->Texture, destinationRectangle, src,
                                              Microsoft::Xna::Framework::Color(255, 255, 255, 255));
                    }
                    if (drawFringe) {
                        Rectangle src = map_->GetFringeLayerSourceRectangle(mapPosition);
                        if (src != Rectangle::Empty)
                            spriteBatch.Draw(*map_->Texture, destinationRectangle, src,
                                              Microsoft::Xna::Framework::Color(255, 255, 255, 255));
                    }
                    if (drawObject) {
                        Rectangle src = map_->GetObjectLayerSourceRectangle(mapPosition);
                        if (src != Rectangle::Empty)
                            spriteBatch.Draw(*map_->Texture, destinationRectangle, src,
                                              Microsoft::Xna::Framework::Color(255, 255, 255, 255));
                    }
                }
            }
        }
    }

    static constexpr float PartyLeaderMovementSpeed = 3.0f;

private:
    static bool CanPartyLeaderMoveUp() {
        if (partyLeaderPosition_.TileOffset.Y > -MovementCollisionTolerance) return true;
        auto& p = partyLeaderPosition_.TilePosition;
        if (partyLeaderPosition_.TileOffset.X < -MovementCollisionTolerance) {
            if (map_->IsBlocked(Point(p.X - 1, p.Y - 1))) return false;
        } else if (partyLeaderPosition_.TileOffset.X > MovementCollisionTolerance) {
            if (map_->IsBlocked(Point(p.X + 1, p.Y - 1))) return false;
        }
        return !map_->IsBlocked(Point(p.X, p.Y - 1));
    }
    static bool CanPartyLeaderMoveDown() {
        if (partyLeaderPosition_.TileOffset.Y < MovementCollisionTolerance) return true;
        auto& p = partyLeaderPosition_.TilePosition;
        if (partyLeaderPosition_.TileOffset.X < -MovementCollisionTolerance) {
            if (map_->IsBlocked(Point(p.X - 1, p.Y + 1))) return false;
        } else if (partyLeaderPosition_.TileOffset.X > MovementCollisionTolerance) {
            if (map_->IsBlocked(Point(p.X + 1, p.Y + 1))) return false;
        }
        return !map_->IsBlocked(Point(p.X, p.Y + 1));
    }
    static bool CanPartyLeaderMoveLeft() {
        if (partyLeaderPosition_.TileOffset.X > -MovementCollisionTolerance) return true;
        auto& p = partyLeaderPosition_.TilePosition;
        if (partyLeaderPosition_.TileOffset.Y < -MovementCollisionTolerance) {
            if (map_->IsBlocked(Point(p.X - 1, p.Y - 1))) return false;
        } else if (partyLeaderPosition_.TileOffset.Y > MovementCollisionTolerance) {
            if (map_->IsBlocked(Point(p.X - 1, p.Y + 1))) return false;
        }
        return !map_->IsBlocked(Point(p.X - 1, p.Y));
    }
    static bool CanPartyLeaderMoveRight() {
        if (partyLeaderPosition_.TileOffset.X < MovementCollisionTolerance) return true;
        auto& p = partyLeaderPosition_.TilePosition;
        if (partyLeaderPosition_.TileOffset.Y < -MovementCollisionTolerance) {
            if (map_->IsBlocked(Point(p.X + 1, p.Y - 1))) return false;
        } else if (partyLeaderPosition_.TileOffset.Y > MovementCollisionTolerance) {
            if (map_->IsBlocked(Point(p.X + 1, p.Y + 1))) return false;
        }
        return !map_->IsBlocked(Point(p.X + 1, p.Y));
    }

    static Vector2 UpdatePartyLeaderAutoMovement() {
        if (autoPartyLeaderMovement_ == Vector2::Zero) return Vector2::Zero;
        Vector2 direction = Vector2::Normalize(autoPartyLeaderMovement_);
        Vector2 movement = Vector2::Multiply(direction, PartyLeaderMovementSpeed);
        movement.X = (movement.X < 0 ? -1.0f : (movement.X > 0 ? 1.0f : 0.0f)) *
                     Microsoft::Xna::Framework::MathHelper::Min(std::abs(movement.X), std::abs(autoPartyLeaderMovement_.X));
        movement.Y = (movement.Y < 0 ? -1.0f : (movement.Y > 0 ? 1.0f : 0.0f)) *
                     Microsoft::Xna::Framework::MathHelper::Min(std::abs(movement.Y), std::abs(autoPartyLeaderMovement_.Y));
        autoPartyLeaderMovement_ = autoPartyLeaderMovement_ - movement;
        return movement;
    }

    static Vector2 UpdateUserMovement() {
        Vector2 desired = Vector2::Zero;
        if (InputManager::IsActionPressed(InputManager::Action::MoveCharacterUp) && CanPartyLeaderMoveUp())
            desired.Y -= PartyLeaderMovementSpeed;
        if (InputManager::IsActionPressed(InputManager::Action::MoveCharacterDown) && CanPartyLeaderMoveDown())
            desired.Y += PartyLeaderMovementSpeed;
        if (InputManager::IsActionPressed(InputManager::Action::MoveCharacterLeft) && CanPartyLeaderMoveLeft())
            desired.X -= PartyLeaderMovementSpeed;
        if (InputManager::IsActionPressed(InputManager::Action::MoveCharacterRight) && CanPartyLeaderMoveRight())
            desired.X += PartyLeaderMovementSpeed;
        return desired;
    }

    // Returns true if the character can move into the tile (calls out to
    // Session::EncounterTile -- defined out-of-line, see Update()).
    static bool MoveIntoTile(Point mapPosition);

    static constexpr int MovementCollisionTolerance = 12;

    static inline std::shared_ptr<RolePlayingGameData::Map> map_;
    static inline Vector2 mapOriginPosition_{};
    static inline Viewport viewport_{};
    static inline Vector2 viewportCenter_{};
    static inline PlayerPosition partyLeaderPosition_{};
    static inline Vector2 autoPartyLeaderMovement_{};

    friend class PlayerPosition;
};

inline Vector2 PlayerPosition::ScreenPosition() const {
    return TileEngine::GetScreenPosition(TilePosition) + TileOffset;
}

inline void PlayerPosition::CalculateMovement(Vector2 movement, Point& tilePosition, Vector2& tileOffset) {
    tileOffset = tileOffset + movement;
    auto tileSize = TileEngine::Map()->TileSize;
    while (tileOffset.X > tileSize.X / 2.0f) { tilePosition.X++; tileOffset.X -= tileSize.X; }
    while (tileOffset.X < -tileSize.X / 2.0f) { tilePosition.X--; tileOffset.X += tileSize.X; }
    while (tileOffset.Y > tileSize.Y / 2.0f) { tilePosition.Y++; tileOffset.Y -= tileSize.Y; }
    while (tileOffset.Y < -tileSize.Y / 2.0f) { tilePosition.Y--; tileOffset.Y += tileSize.Y; }
}

} // namespace RolePlaying

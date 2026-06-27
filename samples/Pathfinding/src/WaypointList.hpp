#pragma once
#include <deque>
#include <optional>
#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector4.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp"
#include "Microsoft/Xna/Framework/Graphics/SpriteEffects.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"

namespace Pathfinding {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class WaypointList {
    static constexpr float waypointNodeDrawScale = 0.75f;

    std::deque<Vector2> data_;
    float scale_ = 1.0f;

    std::optional<Texture2D> waypointTexture_;
    Vector2 waypointCenter_;

public:
    void SetScale(float v) { scale_ = v * waypointNodeDrawScale; }

    void LoadContent(Content::ContentManager& content) {
        waypointTexture_.emplace(content.Load<Texture2D>("dot"));
        waypointCenter_ = Vector2(
            (float)(waypointTexture_->getWidthProperty()  / 2),
            (float)(waypointTexture_->getHeightProperty() / 2));
    }

    // Draw assumes spriteBatch is already begun
    void Draw(SpriteBatch& spriteBatch) {
        if (!waypointTexture_.has_value()) return;
        int numberPoints = (int)data_.size() - 1;
        if (numberPoints == 0) numberPoints = 1;

        float i = 0.0f;
        for (const Vector2& loc : data_) {
            float lerpAmt = i / (float)numberPoints;
            Color drawColor(Vector4::Lerp(
                Color(255, 0,   0, 255).ToVector4(),
                Color(  0, 0, 255, 255).ToVector4(),
                lerpAmt));

            spriteBatch.Draw(*waypointTexture_, loc, std::nullopt, drawColor,
                0.0f, waypointCenter_, scale_, SpriteEffects::None, 0.0f);
            i += 1.0f;
        }
    }

    void Enqueue(const Vector2& v) { data_.push_back(v); }
    void Dequeue()                 { data_.pop_front(); }
    Vector2 Peek() const           { return data_.front(); }
    void Clear()                   { data_.clear(); }
    int Count() const              { return (int)data_.size(); }
};

} // namespace Pathfinding

#pragma once

// PlanePrimitiveTextured.hpp — C++ port of PlanePrimitiveTextured.cs (XNA 4.0
// SoccerPitch sample). A single flat quad with default (0,0)-(1,1) UVs, used
// for the non-tiled pitch-line overlay texture.

#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp"

#include "ProceduralPrimitive.hpp"

namespace SoccerPitch {

using Microsoft::Xna::Framework::Graphics::VertexPositionNormalTexture;

class PlanePrimitiveTextured : public ProceduralPrimitive<VertexPositionNormalTexture> {
public:
    static constexpr float DefaultPlaneSize = 1.0f;

    explicit PlanePrimitiveTextured(GraphicsDevice& device, float size = DefaultPlaneSize) {
        VertexPositionNormalTexture vertex;
        vertex.Normal = Vector3(0.0f, 1.0f, 0.0f);

        Vector3 side1(1.0f, 0.0f, 0.0f);
        Vector3 side2(0.0f, 0.0f, 1.0f);

        // Six indices (two triangles) per face.
        AddIndex(CurrentVertex() + 0);
        AddIndex(CurrentVertex() + 2);
        AddIndex(CurrentVertex() + 1);

        AddIndex(CurrentVertex() + 0);
        AddIndex(CurrentVertex() + 3);
        AddIndex(CurrentVertex() + 2);

        // Four vertices for the face.
        float halfSize = size / 2.0f;

        vertex.Position = (-side1 + side2) * halfSize;
        vertex.TextureCoordinate = Vector2(0.0f, 0.0f);
        AddVertex(vertex);

        vertex.Position = (side1 + side2) * halfSize;
        vertex.TextureCoordinate = Vector2(0.0f, 1.0f);
        AddVertex(vertex);

        vertex.Position = (side1 - side2) * halfSize;
        vertex.TextureCoordinate = Vector2(1.0f, 1.0f);
        AddVertex(vertex);

        vertex.Position = (-side1 - side2) * halfSize;
        vertex.TextureCoordinate = Vector2(1.0f, 0.0f);
        AddVertex(vertex);

        InitializePrimitive(device);
    }
};

} // namespace SoccerPitch

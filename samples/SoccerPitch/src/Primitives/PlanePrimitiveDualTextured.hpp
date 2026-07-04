#pragma once

// PlanePrimitiveDualTextured.hpp — C++ port of PlanePrimitiveDualTextured.cs
// (XNA 4.0 SoccerPitch sample).
//
// Adaptation note (see missing.md): the original vertex format
// (VertexPositionNormalDualTexture) carries two INDEPENDENT UV channels, so
// the base and detail textures can tile at different rates (the whole point
// of DualTextureEffect here — a coarser base color plus a finer detail
// texture). CNA's DualTextureEffect/EasyGL shader only supports a single
// shared UV for both textures (`FragColor = texture(uTexture, vUV) *
// texture(uTexture2, vUV)`), so this port uses the built-in
// VertexPositionNormalTexture (single UV) and one shared tiling factor
// instead of two. Both textures tile at the same rate — a visual
// simplification, not a functional gap in the rest of the sample.

#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp"

#include "ProceduralPrimitive.hpp"

namespace SoccerPitch {

using Microsoft::Xna::Framework::Graphics::VertexPositionNormalTexture;

class PlanePrimitiveDualTextured : public ProceduralPrimitive<VertexPositionNormalTexture> {
public:
    // Constructs a plane with a single shared tiling factor for both textures
    // drawn by DualTextureEffect (see the adaptation note above).
    PlanePrimitiveDualTextured(GraphicsDevice& device, float size, Vector2 tiling) {
        VertexPositionNormalTexture vertex;
        vertex.Normal = Vector3(0.0f, 1.0f, 0.0f);

        Vector3 side1(1.0f, 0.0f, 0.0f);
        Vector3 side2(0.0f, 0.0f, 1.0f);

        AddIndex(CurrentVertex() + 0);
        AddIndex(CurrentVertex() + 2);
        AddIndex(CurrentVertex() + 1);

        AddIndex(CurrentVertex() + 0);
        AddIndex(CurrentVertex() + 3);
        AddIndex(CurrentVertex() + 2);

        float halfSize = size / 2.0f;

        vertex.Position = (-side1 + side2) * halfSize;
        vertex.TextureCoordinate = Vector2(0.0f, 0.0f);
        AddVertex(vertex);

        vertex.Position = (side1 + side2) * halfSize;
        vertex.TextureCoordinate = Vector2(0.0f, tiling.Y);
        AddVertex(vertex);

        vertex.Position = (side1 - side2) * halfSize;
        vertex.TextureCoordinate = Vector2(tiling.X, tiling.Y);
        AddVertex(vertex);

        vertex.Position = (-side1 - side2) * halfSize;
        vertex.TextureCoordinate = Vector2(tiling.X, 0.0f);
        AddVertex(vertex);

        InitializePrimitive(device);
    }
};

} // namespace SoccerPitch

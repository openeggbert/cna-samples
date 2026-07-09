#pragma once

// NOXNA helper class -- ported from HeightmapCollisionPipeline.TerrainProcessor
// (TerrainProcessor.cs) and HeightmapCollisionPipeline.HeightMapInfoContent
// (HeightMapInfoContent.cs), both content-BUILD-time classes in the C# original. In XNA, an
// asset-build step (a custom [ContentProcessor] chaining to the stock ModelProcessor) turns
// terrain.bmp into a compiled Model + an attached HeightMapInfo (via Model.Tag), which the
// game then simply loads with Content.Load<Model>("terrain").
//
// CNA has no build-time, pluggable ContentProcessor extensibility (DEFERRED.md item #18) and
// no Model.Tag equivalent, so this port instead builds the terrain mesh and its HeightMapInfo
// directly at runtime, from the same terrain.bmp heightmap texture, replicating
// TerrainProcessor.Process()'s exact algorithm (terrainScale=30, terrainBumpiness=640,
// texCoordScale=0.1 -- TerrainProcessor.cs's own default property values) and
// HeightMapInfoContent's height formula. This is the same adaptation this repo's
// GeneratedGeometry sample already established for its own (structurally identical)
// TerrainProcessor -- see samples/GeneratedGeometry/missing.md and
// samples/GeneratedGeometry/src/Terrain.hpp.
//
// A second, independent reason to build the terrain this way rather than via
// Content.Load<Model>: this heightmap is 257x257 = 66049 vertices, which exceeds the 65535
// limit of a 16-bit index buffer. Real XNA's ModelProcessor would automatically select
// IndexElementSize.ThirtyTwoBits for a mesh this large, but CNA's `.model.json`
// ModelTypeReader (ContentManager.cpp) hardcodes 16-bit indices for every mesh it reads --
// confirmed by direct source read. Building the VertexBuffer/IndexBuffer directly here uses
// the real IndexBuffer(..., IndexElementSize::ThirtyTwoBits, ...) constructor instead,
// sidestepping that limitation entirely.
//
// A side benefit: because this bypasses `.model.json` (which has no per-mesh texture field --
// DEFERRED.md item #6's addendum, the "flat white" finding repeated across PickingSample/
// TrianglePicking/etc.), this terrain's BasicEffect gets a *real* bound Texture2D
// (rocks.bmp), so it renders textured and shaded, not flat white like every Content.Load<Model>
// mesh in this sample set so far. See missing.md.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

#include "Microsoft/Xna/Framework/Color.hpp"
#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/BufferUsage.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexElementSize.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp"

#include "HeightMapInfo.hpp"

namespace HeightmapCollisionSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Content;
using namespace Microsoft::Xna::Framework::Graphics;

class Terrain {
public:
    void Load(ContentManager& content, GraphicsDevice& device) {
        Texture2D heightmap = content.Load<Texture2D>("terrain");
        rocksTexture_ = content.Load<Texture2D>("rocks");

        const int w = heightmap.getWidthProperty();
        const int h = heightmap.getHeightProperty();

        // terrain.bmp is an 8-bit grayscale bitmap (R == G == B for every pixel), matching
        // what TerrainProcessor.cs's input.ConvertBitmapType(typeof(PixelBitmapContent<float>))
        // produces: a single 0..1 float per pixel.
        std::vector<Color> pixels(static_cast<std::size_t>(w) * static_cast<std::size_t>(h),
                                   Color(0, 0, 0, 255));
        heightmap.GetData(pixels.data(), w * h);

        std::vector<float> heightfield(pixels.size());
        for (std::size_t i = 0; i < pixels.size(); ++i)
            heightfield[i] = pixels[i].getRProperty() / 255.0f;

        auto sample = [&](int x, int y) -> float {
            x = std::clamp(x, 0, w - 1);
            y = std::clamp(y, 0, h - 1);
            return heightfield[static_cast<std::size_t>(x + y * w)];
        };

        // heights[x,y] = (bitmap.GetPixel(x,y) - 1) * terrainBumpiness -- exactly
        // HeightMapInfoContent's constructor formula. These are also exactly the values used
        // for each vertex's Y coordinate below (TerrainProcessor.Process()'s position.Y),
        // computed once here and shared by both the mesh and the HeightMapInfo.
        std::vector<float> heights(heightfield.size());
        for (std::size_t i = 0; i < heightfield.size(); ++i)
            heights[i] = (heightfield[i] - 1.0f) * TerrainBumpiness;

        heightMapInfo_ = HeightMapInfo(heights, w, h, TerrainScale);

        std::vector<VertexPositionNormalTexture> verts(heights.size());
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                // position the vertices so that the heightfield is centered around x=0,z=0 --
                // TerrainProcessor.Process()'s exact position formula.
                Vector3 position;
                position.X = TerrainScale * (static_cast<float>(x) - (static_cast<float>(w - 1) / 2.0f));
                position.Z = TerrainScale * (static_cast<float>(y) - (static_cast<float>(h - 1) / 2.0f));
                position.Y = heights[static_cast<std::size_t>(x + y * w)];

                // TerrainProcessor.cs leaves normal generation to the stock ModelProcessor
                // it chains to (MeshHelper.CalculateNormals, run over the un-normaled mesh
                // MeshBuilder produced) -- CNA has no equivalent build-time step to lean on,
                // so this port computes an equivalent smooth per-vertex normal directly from
                // the heightfield's gradient (central differences), the same technique this
                // repo's own GeneratedGeometry::Terrain::Build() already established for the
                // structurally identical original TerrainProcessor.
                Vector3 tangentX(2.0f * TerrainScale,
                                  (sample(x + 1, y) - sample(x - 1, y)) * TerrainBumpiness,
                                  0.0f);
                Vector3 tangentZ(0.0f,
                                  (sample(x, y + 1) - sample(x, y - 1)) * TerrainBumpiness,
                                  2.0f * TerrainScale);
                Vector3 normal = Vector3::Cross(tangentZ, tangentX);
                normal.Normalize();

                verts[static_cast<std::size_t>(x + y * w)] = VertexPositionNormalTexture(
                    position, normal,
                    Vector2(static_cast<float>(x) * TexCoordScale, static_cast<float>(y) * TexCoordScale));
            }
        }

        // Two CCW triangles per quad, matching AddVertex()'s triangle winding in
        // TerrainProcessor.cs exactly.
        std::vector<std::uint32_t> indices;
        indices.reserve(static_cast<std::size_t>(w - 1) * static_cast<std::size_t>(h - 1) * 6);
        for (int y = 0; y < h - 1; ++y) {
            for (int x = 0; x < w - 1; ++x) {
                auto idx = [&](int xi, int yi) -> std::uint32_t {
                    return static_cast<std::uint32_t>(xi + yi * w);
                };
                indices.push_back(idx(x,     y));
                indices.push_back(idx(x + 1, y));
                indices.push_back(idx(x + 1, y + 1));

                indices.push_back(idx(x,     y));
                indices.push_back(idx(x + 1, y + 1));
                indices.push_back(idx(x,     y + 1));
            }
        }

        vertexCount_    = static_cast<int>(verts.size());
        primitiveCount_ = static_cast<int>(indices.size()) / 3;

        vertexBuffer_ = std::make_unique<VertexBuffer>(device, vertexCount_);
        vertexBuffer_->SetData(verts.data(), vertexCount_);

        indexBuffer_ = std::make_unique<IndexBuffer>(device, IndexElementSize::ThirtyTwoBits,
                                                      static_cast<int>(indices.size()), BufferUsage::None);
        indexBuffer_->SetData(indices.data(), static_cast<int>(indices.size()));

        effect_ = std::make_unique<BasicEffect>(device);
        effect_->setTextureEnabledProperty(true);
        effect_->setTextureProperty(&rocksTexture_);
        // TerrainProcessor.cs's BasicMaterialContent.SpecularColor = new Vector3(.4f,.4f,.4f).
        // NOXNA note: EasyGL's lit shader has no specular term at all (the same gap this
        // repo's GeneratedGeometry::missing.md already flagged for this exact value) -- kept
        // for source fidelity even though it currently has no visible effect.
        effect_->setSpecularColorProperty(Vector3(0.4f, 0.4f, 0.4f));
    }

    [[nodiscard]] const HeightMapInfo& GetHeightMapInfo() const { return heightMapInfo_; }

    // Mirrors HeightmapCollisionGame.DrawModel()'s per-frame effect setup, applied here to
    // the terrain's own BasicEffect (World is always identity -- the terrain never moves).
    void Draw(const Matrix& view, const Matrix& projection) {
        effect_->World      = Matrix::getIdentityProperty();
        effect_->View       = view;
        effect_->Projection = projection;

        effect_->EnableDefaultLighting();
        effect_->setPreferPerPixelLightingProperty(true);

        // Set the fog to match the black background color.
        effect_->setFogEnabledProperty(true);
        effect_->setFogColorProperty(Vector3::Zero);
        effect_->setFogStartProperty(1000.0f);
        effect_->setFogEndProperty(3200.0f);

        auto& device = effect_->getGraphicsDeviceInternal();
        // NOXNA: the mesh's winding, viewed from this sample's fixed above/behind camera,
        // needs CullNone to stay visible -- the same adjustment already established by
        // GeneratedGeometry's own (structurally identical) terrain.
        device.setRasterizerStateProperty(RasterizerState::CullNone);
        device.SetVertexBuffer(vertexBuffer_.get());
        device.setIndicesProperty(indexBuffer_.get());

        for (auto& pass : effect_->getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            device.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_, 0, primitiveCount_);
        }

        device.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);
    }

private:
    static constexpr float TerrainScale     = 30.0f;
    static constexpr float TerrainBumpiness = 640.0f;
    static constexpr float TexCoordScale    = 0.1f;

    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer>  indexBuffer_;
    std::unique_ptr<BasicEffect>  effect_;
    Texture2D      rocksTexture_;
    HeightMapInfo  heightMapInfo_;
    int vertexCount_    = 0;
    int primitiveCount_ = 0;
};

} // namespace HeightmapCollisionSample

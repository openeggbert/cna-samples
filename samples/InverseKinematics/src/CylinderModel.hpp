#pragma once

// NOXNA helper -- loads the cylinder mesh directly into a VertexBuffer/IndexBuffer +
// BasicEffect, bypassing Content.Load<Model>("cylinder") entirely.
//
// XNA's original just calls `Content.Load<Model>("cylinder")` and iterates
// `cylinderModel.Meshes`. The straightforward CNA port of that (see this sample's own
// git history) builds and loads fine and reports the expected single mesh/one
// BasicEffect via `Model::getMeshesProperty()`/`ModelMesh::getEffectsPropertyMutable()`
// -- but the model never actually appears on screen (confirmed live: neither the 20-link
// chain nor even a single full-scale, identity-world, unlit, un-culled copy of the same
// mesh renders -- a plain hand-built triangle at the same scale/distance and even
// PickingSample's own already-shipped, already-working "Cylinder" model both failed the
// same way when loaded through this exact code path in this sample, ruling out this
// mesh's own data as the cause).
//
// Root-caused by direct source read + a standalone sizeof() probe: every CNA vertex
// struct (VertexPositionColor, VertexPositionTexture, VertexPositionColorTexture,
// VertexPositionNormalTexture) publicly inherits from the polymorphic `IVertexType`
// (a virtual destructor + a pure virtual method), so each one carries an 8-byte vtable
// pointer the XNA-original "clean" sizes never had. Confirmed live:
//   sizeof(VertexPositionColor)         = 40 (not 16)
//   sizeof(VertexPositionTexture)       = 32 (not 20)
//   sizeof(VertexPositionColorTexture)  = 56 (not 24)
//   sizeof(VertexPositionNormalTexture) = 40 (not 32)
// `ModelTypeReader::Read()` (cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp)
// picks which typed `VertexBuffer::SetData` overload to call by comparing the
// `.model.json`-declared "vertexStride" (always one of the *clean* values 16/20/24/32,
// since every conversion tool in this repo writes the tightly-packed XNA layout) against
// `sizeof(...)` of these now-vtable-inflated structs. None of the clean values match
// their *intended* struct's real (inflated) size -- but "vertexStride": 32 (every
// Model-based sample in this whole repo's own asset, meant for
// VertexPositionNormalTexture) *accidentally* equals `sizeof(VertexPositionTexture)`
// (also 32, coincidentally) instead, so the reader silently dispatches to the *wrong*
// overload and reinterpret_casts the raw file bytes as an array of (vtable-shifted)
// VertexPositionTexture objects -- reading every vertex's Position/TextureCoordinate
// fields from the wrong byte offsets in the source buffer. The result is corrupted
// geometry for every stride-32 `.model.json` in this repo, not just this one -- this is
// almost certainly the true root cause behind the "near-plane-clipping-family" symptom
// (thin line / full invisibility) tracked since CameraShake, not an actual clipping bug;
// see DEFERRED.md item #26 and missing.md for the full writeup.
//
// Workaround (same shape as HeightmapCollision's Terrain.hpp/GeneratedGeometry's
// Terrain.hpp, both already established precedent for bypassing a `.model.json` gap by
// building a VertexBuffer/IndexBuffer/BasicEffect directly at runtime instead of through
// Content.Load<Model>): read the already-converted cylinder_verts.bin/cylinder_idx.bin
// (produced once, offline, by tools/obj2model.py -- unchanged) directly here and
// construct real, normally-initialized C++ `VertexPositionNormalTexture` objects (no
// reinterpret_cast on a raw byte blob), then upload them through the *same* typed
// `VertexBuffer::SetData(const VertexPositionNormalTexture*, count)` overload
// `ModelTypeReader` was trying to reach -- confirmed live this works correctly (proven
// first by HeightmapCollision's own terrain, which already goes through this exact
// typed overload for an unrelated reason and renders correctly).

#include <Microsoft/Xna/Framework/Content/ContentManager.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace InverseKinematicsSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class CylinderModel {
public:
    void Load(Content::ContentManager& content, GraphicsDevice& device) {
        const std::string root = content.getRootDirectoryProperty();
        std::vector<VertexPositionNormalTexture> verts =
            ReadVertices(root + "/cylinder_verts.bin");
        std::vector<std::uint16_t> indices = ReadIndices(root + "/cylinder_idx.bin");

        vertexCount_    = static_cast<int>(verts.size());
        primitiveCount_ = static_cast<int>(indices.size()) / 3;

        vertexBuffer_ = std::make_unique<VertexBuffer>(device, vertexCount_);
        vertexBuffer_->SetData(verts.data(), vertexCount_);

        indexBuffer_ = std::make_unique<IndexBuffer>(device, static_cast<int>(indices.size()));
        indexBuffer_->SetData(indices.data(), static_cast<int>(indices.size()));

        effect_ = std::make_unique<BasicEffect>(device);
        effect_->EnableDefaultLighting();
        effect_->setPreferPerPixelLightingProperty(true);
    }

    // Draws one instance of the cylinder mesh with the given per-instance transform and
    // color, sharing view/projection across every instance -- mirrors what
    // IKSample.cs's DrawBones() does per bone in the chain.
    void DrawInstance(const Matrix& world, const Matrix& view, const Matrix& projection,
                       const Vector3& diffuseColor) {
        effect_->World = world;
        effect_->View = view;
        effect_->Projection = projection;
        effect_->setDiffuseColorProperty(diffuseColor);

        auto& device = effect_->getGraphicsDeviceInternal();
        device.SetVertexBuffer(vertexBuffer_.get());
        device.setIndicesProperty(indexBuffer_.get());

        for (auto& pass : effect_->getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            device.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_, 0,
                                         primitiveCount_);
        }
    }

private:
    static std::vector<VertexPositionNormalTexture> ReadVertices(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("CylinderModel: failed to open " + path);
        }
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        constexpr std::streamsize floatsPerVertex = 8; // px,py,pz, nx,ny,nz, u,v
        constexpr std::streamsize bytesPerVertex = floatsPerVertex * static_cast<std::streamsize>(sizeof(float));
        if (size < 0 || size % bytesPerVertex != 0) {
            throw std::runtime_error("CylinderModel: unexpected vertex file size: " + path);
        }

        std::vector<float> raw(static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(float))));
        if (!raw.empty() && !file.read(reinterpret_cast<char*>(raw.data()), size)) {
            throw std::runtime_error("CylinderModel: failed to read " + path);
        }

        const std::size_t count = raw.size() / static_cast<std::size_t>(floatsPerVertex);
        std::vector<VertexPositionNormalTexture> verts(count);
        for (std::size_t i = 0; i < count; ++i) {
            const float* v = &raw[i * static_cast<std::size_t>(floatsPerVertex)];
            verts[i] = VertexPositionNormalTexture(
                Vector3(v[0], v[1], v[2]),
                Vector3(v[3], v[4], v[5]),
                Vector2(v[6], v[7]));
        }
        return verts;
    }

    static std::vector<std::uint16_t> ReadIndices(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("CylinderModel: failed to open " + path);
        }
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size < 0 || size % static_cast<std::streamsize>(sizeof(std::uint16_t)) != 0) {
            throw std::runtime_error("CylinderModel: unexpected index file size: " + path);
        }

        std::vector<std::uint16_t> indices(
            static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(std::uint16_t))));
        if (!indices.empty() &&
            !file.read(reinterpret_cast<char*>(indices.data()), size)) {
            throw std::runtime_error("CylinderModel: failed to read " + path);
        }
        return indices;
    }

    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer>  indexBuffer_;
    std::unique_ptr<BasicEffect>  effect_;
    int vertexCount_    = 0;
    int primitiveCount_ = 0;
};

} // namespace InverseKinematicsSample

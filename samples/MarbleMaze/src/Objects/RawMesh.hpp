#pragma once

// RawMesh.hpp (NOXNA helper) — loads one already-converted _verts.bin/_idx.bin pair
// (produced offline by tools/obj2model.py, see missing.md) directly into a
// VertexBuffer/IndexBuffer, bypassing Content.Load<Model>()/ModelTypeReader
// entirely.
//
// XNA's original loads the marble/maze via Content.Load<Model>(...) and iterates
// Model.Meshes/Effects (DrawableComponent3D.cs). A straightforward CNA port of that
// (Content.Load<Model>, using this repo's standard assimp-export + tools/obj2model.py
// conversion pipeline) is expected to hit DEFERRED.md item #26 -- ModelTypeReader::Read()
// (ContentManager.cpp) picks a typed VertexBuffer::SetData overload by comparing the
// .model.json's declared (clean, XNA-sized) "vertexStride" against sizeof() of CNA's own
// vertex structs, but every one of those structs now inherits from the polymorphic
// IVertexType, inflating sizeof() past the clean size the format declares --
// "vertexStride": 32 (what every converter in this repo emits) accidentally collides with
// the inflated sizeof(VertexPositionTexture) (also 32), so the reader always uploads
// through the *wrong* typed overload, corrupting position/texcoord data. Confirmed FOUR
// times on other samples' own assets (InverseKinematics' cylinder, ChaseCamera's ship and
// ground) before this sample was ported -- see samples/ChaseCamera/src/RawModel.hpp and
// samples/InverseKinematics/src/CylinderModel.hpp for the first two write-ups, and this
// sample's own missing.md for whether the same bypass was empirically confirmed necessary
// here too.
//
// Workaround (same shape as RawModel.hpp/CylinderModel.hpp/Terrain.hpp): read the
// vertex/index sidecars directly and construct real, normally-initialized C++
// VertexPositionNormalTexture objects field-by-field (not a reinterpret_cast on a raw byte
// blob), then upload through the same typed VertexBuffer::SetData overload
// ModelTypeReader was trying (and failing) to reach.
//
// Unlike ChaseCamera's RawModel.hpp (one mesh = one texture = one owned BasicEffect),
// this sample's Maze needs 6 separate submeshes sharing ONE BasicEffect (texture swapped
// per submesh before each draw call, mirroring how XNA's ModelProcessor would have built
// one BasicEffect per ModelMeshPart, all configured identically by Maze::Draw() except for
// each part's own pre-existing Texture). So RawMesh itself owns no effect/texture -- it is
// just the VertexBuffer/IndexBuffer pair plus the CPU-side vertex positions (needed to
// rebuild MarbleMazeProcessor's per-mesh flattened world-space triangle lists for collision
// -- see Maze.hpp) -- callers own and configure their own BasicEffect(s) and call Draw()
// with it.

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp"

namespace MarbleMazeSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class RawMesh {
public:
    // contentRoot: ContentManager::getRootDirectoryProperty(). meshBaseName: the
    // "<basename>" prefix tools/obj2model.py used for "<...>_verts.bin"/"<...>_idx.bin".
    void Load(const std::string& contentRoot, const std::string& meshBaseName, GraphicsDevice& device) {
        std::vector<VertexPositionNormalTexture> verts = ReadVertices(contentRoot + "/" + meshBaseName + "_verts.bin");
        indices_ = ReadIndices(contentRoot + "/" + meshBaseName + "_idx.bin");

        positions_.reserve(verts.size());
        for (const auto& v : verts) positions_.push_back(v.Position);

        vertexCount_ = static_cast<int>(verts.size());
        primitiveCount_ = static_cast<int>(indices_.size()) / 3;

        vertexBuffer_ = std::make_unique<VertexBuffer>(device, vertexCount_);
        vertexBuffer_->SetData(verts.data(), vertexCount_);

        indexBuffer_ = std::make_unique<IndexBuffer>(device, static_cast<int>(indices_.size()));
        indexBuffer_->SetData(indices_.data(), static_cast<int>(indices_.size()));
    }

    // Per-vertex positions (deduplicated, as stored in the vertex buffer -- NOT
    // triangle-expanded).
    const std::vector<Vector3>& Positions() const { return positions_; }

    // Expands the indexed mesh into a flat, triangle-ordered position list (every
    // consecutive 3 entries = one triangle) -- exactly what
    // MarbleMazeProcessor.FindVertices() flattens into Model.Tag in the C# original,
    // reconstructed here from the same vertex/index data already loaded for rendering
    // instead of needing a separate picking/collision sidecar file.
    std::vector<Vector3> ExpandTrianglePositions() const {
        std::vector<Vector3> out;
        out.reserve(indices_.size());
        for (std::uint16_t i : indices_) out.push_back(positions_[i]);
        return out;
    }

    // Binds this mesh's buffers to the device and issues the draw call through the
    // given (already-configured -- World/View/Projection/Texture/lighting) effect.
    void Draw(BasicEffect& effect, GraphicsDevice& device) const {
        device.SetVertexBuffer(vertexBuffer_.get());
        device.setIndicesProperty(indexBuffer_.get());

        for (auto& pass : effect.getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            device.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_, 0, primitiveCount_);
        }
    }

private:
    static std::vector<VertexPositionNormalTexture> ReadVertices(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("RawMesh: failed to open " + path);
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        constexpr std::streamsize floatsPerVertex = 8; // px,py,pz, nx,ny,nz, u,v
        constexpr std::streamsize bytesPerVertex = floatsPerVertex * static_cast<std::streamsize>(sizeof(float));
        if (size < 0 || size % bytesPerVertex != 0)
            throw std::runtime_error("RawMesh: unexpected vertex file size: " + path);

        std::vector<float> raw(static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(float))));
        if (!raw.empty() && !file.read(reinterpret_cast<char*>(raw.data()), size))
            throw std::runtime_error("RawMesh: failed to read " + path);

        const std::size_t count = raw.size() / static_cast<std::size_t>(floatsPerVertex);
        std::vector<VertexPositionNormalTexture> verts(count);
        for (std::size_t i = 0; i < count; ++i) {
            const float* v = &raw[i * static_cast<std::size_t>(floatsPerVertex)];
            verts[i] = VertexPositionNormalTexture(Vector3(v[0], v[1], v[2]), Vector3(v[3], v[4], v[5]),
                                                    Vector2(v[6], v[7]));
        }
        return verts;
    }

    static std::vector<std::uint16_t> ReadIndices(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("RawMesh: failed to open " + path);
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size < 0 || size % static_cast<std::streamsize>(sizeof(std::uint16_t)) != 0)
            throw std::runtime_error("RawMesh: unexpected index file size: " + path);

        std::vector<std::uint16_t> indices(static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(std::uint16_t))));
        if (!indices.empty() && !file.read(reinterpret_cast<char*>(indices.data()), size))
            throw std::runtime_error("RawMesh: failed to read " + path);
        return indices;
    }

    std::vector<Vector3> positions_;
    std::vector<std::uint16_t> indices_;
    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer> indexBuffer_;
    int vertexCount_ = 0;
    int primitiveCount_ = 0;
};

} // namespace MarbleMazeSample

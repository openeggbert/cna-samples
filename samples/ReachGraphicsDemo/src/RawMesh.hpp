#pragma once

// RawMesh.hpp (NOXNA helper) -- loads one already-converted _verts.bin/_idx.bin pair
// (produced offline by tools/fbx_ascii2model.py / tools/obj2model.py, see missing.md)
// directly into a VertexBuffer/IndexBuffer, bypassing Content.Load<Model>()/
// ModelTypeReader entirely.
//
// XNA's original loads "grid"/"model"/"saucer" via Content.Load<Model>(...) and
// iterates Model.Meshes/Effects. A straightforward CNA port of that hits DEFERRED.md
// item #26: ModelTypeReader::Read() (ContentManager.cpp) picks a typed
// VertexBuffer::SetData overload by comparing the .model.json's declared (clean,
// XNA-sized) "vertexStride" against sizeof() of CNA's own vertex structs -- but every
// one of those structs now inherits from the polymorphic IVertexType, inflating
// sizeof() past the clean size the format declares -- "vertexStride": 32 (what every
// converter in this repo emits) accidentally collides with the inflated
// sizeof(VertexPositionTexture) (also 32), so the reader always uploads through the
// *wrong* typed overload, corrupting position/texcoord data. Confirmed repeatedly on
// other samples' own assets (InverseKinematics' cylinder, ChaseCamera's ship/ground,
// MarbleMaze's maze/marble) before this sample was ported -- see
// samples/ChaseCamera/src/RawModel.hpp / samples/MarbleMaze/src/Objects/RawMesh.hpp for
// the first two write-ups, and this sample's own missing.md for the account here.
//
// Workaround (same shape as RawModel.hpp/RawMesh.hpp/CylinderModel.hpp/Terrain.hpp):
// read the vertex/index sidecars directly and construct real, normally-initialized C++
// VertexPositionNormalTexture objects field-by-field (not a reinterpret_cast on a raw
// byte blob), then upload through the same typed VertexBuffer::SetData overload
// ModelTypeReader was trying (and failing) to reach.
//
// Like MarbleMaze's own RawMesh.hpp (and unlike ChaseCamera's RawModel.hpp), this class
// owns no effect/texture of its own -- BasicDemo's grid, DualDemo's model (7 submeshes,
// 2 different base textures), and EnvmapDemo's saucer all need to share/reconfigure a
// single effect across one or more RawMesh instances, so callers own and configure
// their own effect and call Draw() with it.

#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Microsoft/Xna/Framework/Vector2.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Graphics/Effect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp"
#include "Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class RawMesh {
public:
    // contentRoot: ContentManager::getRootDirectoryProperty(). meshBaseName: the
    // "<basename>_<meshname>" prefix tools/fbx_ascii2model.py / tools/obj2model.py used
    // for "<...>_verts.bin"/"<...>_idx.bin" (see each converted <Name>.model.json's own
    // "vertices"/"indices" fields for the exact names on disk).
    void Load(const std::string& contentRoot, const std::string& meshBaseName, GraphicsDevice& device) {
        std::vector<VertexPositionNormalTexture> verts = ReadVertices(contentRoot + "/" + meshBaseName + "_verts.bin");
        std::vector<std::uint16_t> indices = ReadIndices(contentRoot + "/" + meshBaseName + "_idx.bin");

        vertexCount_ = static_cast<int>(verts.size());
        primitiveCount_ = static_cast<int>(indices.size()) / 3;

        vertexBuffer_ = std::make_unique<VertexBuffer>(device, vertexCount_);
        vertexBuffer_->SetData(verts.data(), vertexCount_);

        indexBuffer_ = std::make_unique<IndexBuffer>(device, static_cast<int>(indices.size()));
        indexBuffer_->SetData(indices.data(), static_cast<int>(indices.size()));
    }

    // Binds this mesh's buffers to the device and issues the draw call through the
    // given (already-configured -- World/View/Projection/Texture/lighting) effect.
    void Draw(Effect& effect, GraphicsDevice& device) const {
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

    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer> indexBuffer_;
    int vertexCount_ = 0;
    int primitiveCount_ = 0;
};

} // namespace ReachGraphicsDemoSample

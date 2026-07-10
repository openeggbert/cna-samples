#pragma once

// RawMeshPosTex.hpp (NOXNA helper) -- like RawMesh.hpp, loads an already-converted
// _verts.bin/_idx.bin pair directly into a VertexBuffer/IndexBuffer, bypassing
// Content.Load<Model>()/ModelTypeReader (DEFERRED.md item #26). Unlike RawMesh.hpp
// (which uploads VertexPositionNormalTexture, for BasicEffect/EnvironmentMapEffect
// meshes that need lighting), this variant uploads plain VertexPositionTexture
// (Position + UV only, the vertex's own normal field is read from the sidecar file but
// discarded) -- required specifically for DualDemo's DualTextureEffect meshes.
//
// Root cause (confirmed live via screenshot): CNA's EasyGL backend's own
// "dual+textured3D" GLSL program (EnsureDualTextured3DProgram(),
// EasyGLGraphicsBackend.cpp) hardcodes a 2-attribute vertex layout --
// `layout(location=0) in vec3 aPos; layout(location=1) in vec2 aUV;` -- with NO normal
// attribute in between. Uploading VertexPositionNormalTexture (Position + Normal + UV,
// where UV is declared at attribute location 2, not 1) against this shader makes
// `aUV` silently read from whatever is actually bound at location 1 -- the mesh's own
// Normal.xy -- instead of the real per-vertex UV. Confirmed live: with the wrong (real)
// vertex format, every one of DualDemo's 7 submeshes rendered as a single flat/uniform
// color per mesh (each face's constant per-face normal producing a constant "fake UV",
// landing on one arbitrary texel) instead of the real tiled texture detail; switching
// to this Position+UV-only upload (matching the shader's actual attribute layout)
// fixed it completely. Not a DEFERRED.md item on its own -- see missing.md for why this
// is filed as a new, narrower CNA gap (DualTextureEffect's own vertex-layout
// requirement is undocumented/unlike BasicEffect's, and any future
// Content.Load<Model>-based DualTextureEffect sample would hit the same mismatch).

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
#include "Microsoft/Xna/Framework/Graphics/VertexPositionTexture.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class RawMeshPosTex {
public:
    void Load(const std::string& contentRoot, const std::string& meshBaseName, GraphicsDevice& device) {
        std::vector<VertexPositionTexture> verts = ReadVertices(contentRoot + "/" + meshBaseName + "_verts.bin");
        std::vector<std::uint16_t> indices = ReadIndices(contentRoot + "/" + meshBaseName + "_idx.bin");

        vertexCount_ = static_cast<int>(verts.size());
        primitiveCount_ = static_cast<int>(indices.size()) / 3;

        vertexBuffer_ = std::make_unique<VertexBuffer>(device, vertexCount_);
        vertexBuffer_->SetData(verts.data(), vertexCount_);

        indexBuffer_ = std::make_unique<IndexBuffer>(device, static_cast<int>(indices.size()));
        indexBuffer_->SetData(indices.data(), static_cast<int>(indices.size()));
    }

    void Draw(Effect& effect, GraphicsDevice& device) const {
        device.SetVertexBuffer(vertexBuffer_.get());
        device.setIndicesProperty(indexBuffer_.get());

        for (auto& pass : effect.getCurrentTechniqueProperty()->getPassesProperty()) {
            pass.Apply();
            device.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_, 0, primitiveCount_);
        }
    }

private:
    // Reads the standard 8-floats-per-vertex sidecar (px,py,pz, nx,ny,nz, u,v) but
    // keeps only position + UV, discarding the normal.
    static std::vector<VertexPositionTexture> ReadVertices(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("RawMeshPosTex: failed to open " + path);
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        constexpr std::streamsize floatsPerVertex = 8; // px,py,pz, nx,ny,nz, u,v
        constexpr std::streamsize bytesPerVertex = floatsPerVertex * static_cast<std::streamsize>(sizeof(float));
        if (size < 0 || size % bytesPerVertex != 0)
            throw std::runtime_error("RawMeshPosTex: unexpected vertex file size: " + path);

        std::vector<float> raw(static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(float))));
        if (!raw.empty() && !file.read(reinterpret_cast<char*>(raw.data()), size))
            throw std::runtime_error("RawMeshPosTex: failed to read " + path);

        const std::size_t count = raw.size() / static_cast<std::size_t>(floatsPerVertex);
        std::vector<VertexPositionTexture> verts(count);
        for (std::size_t i = 0; i < count; ++i) {
            const float* v = &raw[i * static_cast<std::size_t>(floatsPerVertex)];
            verts[i] = VertexPositionTexture(Vector3(v[0], v[1], v[2]), Vector2(v[6], v[7]));
        }
        return verts;
    }

    static std::vector<std::uint16_t> ReadIndices(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("RawMeshPosTex: failed to open " + path);
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size < 0 || size % static_cast<std::streamsize>(sizeof(std::uint16_t)) != 0)
            throw std::runtime_error("RawMeshPosTex: unexpected index file size: " + path);

        std::vector<std::uint16_t> indices(static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(std::uint16_t))));
        if (!indices.empty() && !file.read(reinterpret_cast<char*>(indices.data()), size))
            throw std::runtime_error("RawMeshPosTex: failed to read " + path);
        return indices;
    }

    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer> indexBuffer_;
    int vertexCount_ = 0;
    int primitiveCount_ = 0;
};

} // namespace ReachGraphicsDemoSample

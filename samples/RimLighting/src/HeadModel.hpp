#pragma once

// NOXNA helper -- loads head.fbx's already-converted head_pasted__polySurface14_verts.bin/
// _idx.bin buffers directly into a VertexBuffer/IndexBuffer, bypassing Content.Load<Model>
// entirely.
//
// XNA's original (Game1.cs's LoadContent()) just calls Content.Load<Model>("head") and
// iterates model.Meshes/Effects (each mesh's effect defaulting to a real
// EnvironmentMapEffect, since head.fbx's own RimLightingContent.contentproj sets
// <ProcessorParameters_DefaultEffect>EnvironmentMapEffect</ProcessorParameters_DefaultEffect>
// on the ModelProcessor). Per this session's own established DEFERRED.md item #26 (every
// stride-32 .model.json in this repo hits a real ModelTypeReader vertex-stride/vtable
// mismatch that silently corrupts position/texcoord data -- confirmed on 5+ other assets
// across 5 other samples this session), this port goes straight to the same RawMesh/
// RawModel-style bypass established by ChaseCamera/MarbleMaze/InverseKinematics rather
// than trying Content.Load<Model> first.
//
// A second, separate reason this mesh specifically needs a bypass regardless of item #26:
// head.fbx's single mesh node ("Model::pasted__polySurface14") is parented under a Null
// "Model::group" node that carries a REAL, non-identity transform (Lcl Translation
// ~(0,-14.48,0.16), Lcl Rotation (0,180,0) degrees -- confirmed by direct read of
// head.fbx). In real XNA this parent-bone transform is applied every frame via
// Model.CopyAbsoluteBoneTransformsTo() + "effect.World = transforms[mesh.ParentBone.Index]
// * world" (Game1.cs:200/214) -- CNA's own tools/fbx_ascii2model.py only ever bakes a
// mesh's OWN local Model-node transform (there being no other converted sample with a
// meaningfully-transformed parent Null node to expose this gap before). Since the group
// node has no animation Take referencing it (confirmed via grep -- a static, constant
// transform), this port's own one-off conversion script (not committed to tools/; see
// missing.md for the exact commands) bakes the group's transform directly into the
// mesh's vertex positions/normals at conversion time instead -- mathematically identical
// to what CopyAbsoluteBoneTransformsTo() would produce every single frame for a static
// (non-animated) 2-node hierarchy like this one.

#include <Microsoft/Xna/Framework/Content/ContentManager.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <cstdint>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace RimLightingSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class HeadModel {
public:
    // meshBaseName: the "<basename>_<meshname>" prefix the one-off conversion script used
    // for "<...>_verts.bin"/"<...>_idx.bin" (see head.model.json's own "vertices"/
    // "indices" fields for the exact names on disk).
    void Load(Content::ContentManager& content, GraphicsDevice& device, const std::string& meshBaseName) {
        const std::string root = content.getRootDirectoryProperty();
        std::vector<VertexPositionNormalTexture> verts = ReadVertices(root + "/" + meshBaseName + "_verts.bin");
        std::vector<std::uint16_t> indices = ReadIndices(root + "/" + meshBaseName + "_idx.bin");

        vertexCount_ = static_cast<int>(verts.size());
        primitiveCount_ = static_cast<int>(indices.size()) / 3;

        vertexBuffer_ = std::make_unique<VertexBuffer>(device, vertexCount_);
        vertexBuffer_->SetData(verts.data(), vertexCount_);

        indexBuffer_ = std::make_unique<IndexBuffer>(device, static_cast<int>(indices.size()));
        indexBuffer_->SetData(indices.data(), static_cast<int>(indices.size()));
    }

    // Binds this mesh's buffers and issues the draw call. The caller is responsible for
    // configuring and Apply()-ing the effect beforehand (mirrors Game1.cs's own
    // "foreach (EffectPass pass in effect.CurrentTechnique.Passes) { pass.Apply(); ... }"
    // shape, just with the effect owned by the caller instead of a ModelMeshPart).
    void Draw(GraphicsDevice& device) const {
        device.SetVertexBuffer(vertexBuffer_.get());
        device.setIndicesProperty(indexBuffer_.get());
        device.DrawIndexedPrimitives(PrimitiveType::TriangleList, 0, 0, vertexCount_, 0, primitiveCount_);
    }

private:
    static std::vector<VertexPositionNormalTexture> ReadVertices(const std::string& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("HeadModel: failed to open " + path);
        }
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        constexpr std::streamsize floatsPerVertex = 8; // px,py,pz, nx,ny,nz, u,v
        constexpr std::streamsize bytesPerVertex = floatsPerVertex * static_cast<std::streamsize>(sizeof(float));
        if (size < 0 || size % bytesPerVertex != 0) {
            throw std::runtime_error("HeadModel: unexpected vertex file size: " + path);
        }

        std::vector<float> raw(static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(float))));
        if (!raw.empty() && !file.read(reinterpret_cast<char*>(raw.data()), size)) {
            throw std::runtime_error("HeadModel: failed to read " + path);
        }

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
        if (!file) {
            throw std::runtime_error("HeadModel: failed to open " + path);
        }
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size < 0 || size % static_cast<std::streamsize>(sizeof(std::uint16_t)) != 0) {
            throw std::runtime_error("HeadModel: unexpected index file size: " + path);
        }

        std::vector<std::uint16_t> indices(static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(std::uint16_t))));
        if (!indices.empty() && !file.read(reinterpret_cast<char*>(indices.data()), size)) {
            throw std::runtime_error("HeadModel: failed to read " + path);
        }
        return indices;
    }

    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer> indexBuffer_;
    int vertexCount_ = 0;
    int primitiveCount_ = 0;
};

} // namespace RimLightingSample

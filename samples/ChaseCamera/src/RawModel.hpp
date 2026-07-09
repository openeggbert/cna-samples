#pragma once

// NOXNA helper -- loads a single-mesh model's already-converted _verts.bin/_idx.bin buffers
// directly into a VertexBuffer/IndexBuffer + BasicEffect, bypassing Content.Load<Model>(...)
// entirely, and binding a real Texture2D to the effect directly.
//
// XNA's original (ChaseCameraGame.cs's DrawModel()) just calls Content.Load<Model>("Ship")/
// Content.Load<Model>("Ground") and iterates Model.Meshes/Effects. The straightforward CNA
// port of that (Content.Load<Model>("Ship")/Content.Load<Model>("Ground"), converted from
// Ship.fbx/Ground.x via this repo's standard tools/fbx_ascii2model.py and
// assimp-export+tools/obj2model.py pipelines) builds and loads without error but renders
// **nothing at all** -- confirmed live in this sample: a temporary debug build using
// Content.Load<Model> + the exact same DrawModel()/BoneIndexOf() pattern PickingSample/
// TrianglePicking/HeightmapCollision already established rendered a blank CornflowerBlue
// screen (only the 2D HUD text visible) for 6+ seconds, no crash, at this sample's own
// ~4000-unit initial camera distance (DesiredPositionOffset = (0, 2000, 3500) -- even farther
// than Graphics3D's ~3523-unit spaceship, which showed the identical "fully invisible"
// symptom).
//
// This is the same DEFERRED.md item #26 bug InverseKinematics found and worked around
// (CylinderModel.hpp): every CNA vertex struct (VertexPositionColor/VertexPositionTexture/
// VertexPositionColorTexture/VertexPositionNormalTexture) now publicly inherits from the
// polymorphic IVertexType, adding an 8-byte vtable pointer that inflates each struct's real
// sizeof() past the "clean" XNA size every conversion tool in this repo declares as
// "vertexStride" (32, for VertexPositionNormalTexture). "vertexStride": 32 *accidentally*
// equals the inflated sizeof(VertexPositionTexture) (also 32, by coincidence), so
// ModelTypeReader::Read() (cna/src/Microsoft/Xna/Framework/Content/ContentManager.cpp) always
// dispatches to the *wrong* typed VertexBuffer::SetData overload and reinterpret_casts the
// raw (vtable-free) file bytes as vtable-shifted VertexPositionTexture objects -- reading
// Position/TextureCoordinate from the wrong byte offsets and corrupting every stride-32
// .model.json's vertex data repo-wide, not just this sample's. See DEFERRED.md item #26 and
// samples/InverseKinematics/missing.md for the full mechanism and the first confirmation;
// this sample (Ship_p1_wedge_geo1: 32458 vertices; Ground: 6 vertices) is a THIRD and FOURTH
// independent confirmation, on two more independently-converted assets (one FBX, one X-file),
// at a much larger vertex count than InverseKinematics' 418-vertex cylinder -- see missing.md.
//
// Workaround (same shape as InverseKinematics' CylinderModel.hpp / HeightmapCollision's and
// GeneratedGeometry's Terrain.hpp): read the already-converted _verts.bin/_idx.bin directly
// and construct real, normally-initialized C++ VertexPositionNormalTexture objects
// (field-by-field, not a reinterpret_cast on a raw byte blob), then upload them through the
// same typed VertexBuffer::SetData(const VertexPositionNormalTexture*, count) overload
// ModelTypeReader was trying (and failing) to reach. A side benefit, following Terrain.hpp's
// own precedent: since this bypasses `.model.json` (which has no per-mesh texture field --
// DEFERRED.md item #6's addendum), the BasicEffect here gets a *real* bound Texture2D, so
// both models render textured and shaded, not flat white like every Content.Load<Model> mesh
// in this repo's lighting-sample series so far.

#include <Microsoft/Xna/Framework/Content/ContentManager.hpp>
#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/IndexBuffer.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexBuffer.hpp>
#include <Microsoft/Xna/Framework/Graphics/VertexPositionNormalTexture.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ChaseCameraSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class RawModel {
public:
    // meshBaseName: the "<basename>_<meshname>" prefix tools/fbx_ascii2model.py /
    // tools/obj2model.py used for "<...>_verts.bin"/"<...>_idx.bin" (see each converted
    // <Name>.model.json's own "vertices"/"indices" fields for the exact names on disk).
    // textureName: the Content-relative name of the PNG to bind (loaded and owned here).
    void Load(Content::ContentManager& content, GraphicsDevice& device,
              const std::string& meshBaseName, const std::string& textureName) {
        const std::string root = content.getRootDirectoryProperty();
        std::vector<VertexPositionNormalTexture> verts =
            ReadVertices(root + "/" + meshBaseName + "_verts.bin");
        std::vector<std::uint16_t> indices = ReadIndices(root + "/" + meshBaseName + "_idx.bin");

        vertexCount_    = static_cast<int>(verts.size());
        primitiveCount_ = static_cast<int>(indices.size()) / 3;

        vertexBuffer_ = std::make_unique<VertexBuffer>(device, vertexCount_);
        vertexBuffer_->SetData(verts.data(), vertexCount_);

        indexBuffer_ = std::make_unique<IndexBuffer>(device, static_cast<int>(indices.size()));
        indexBuffer_->SetData(indices.data(), static_cast<int>(indices.size()));

        texture_ = content.Load<Texture2D>(textureName);

        effect_ = std::make_unique<BasicEffect>(device);
        effect_->setTextureEnabledProperty(true);
        effect_->setTextureProperty(&texture_.value());
    }

    // Draws the model with the given world/view/projection, mirroring
    // ChaseCameraGame.cs's DrawModel() effect setup exactly (EnableDefaultLighting() every
    // frame, camera's own View/Projection).
    void Draw(const Matrix& world, const Matrix& view, const Matrix& projection) {
        effect_->EnableDefaultLighting();
        effect_->World = world;
        effect_->View = view;
        effect_->Projection = projection;

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
            throw std::runtime_error("RawModel: failed to open " + path);
        }
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        constexpr std::streamsize floatsPerVertex = 8; // px,py,pz, nx,ny,nz, u,v
        constexpr std::streamsize bytesPerVertex = floatsPerVertex * static_cast<std::streamsize>(sizeof(float));
        if (size < 0 || size % bytesPerVertex != 0) {
            throw std::runtime_error("RawModel: unexpected vertex file size: " + path);
        }

        std::vector<float> raw(static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(float))));
        if (!raw.empty() && !file.read(reinterpret_cast<char*>(raw.data()), size)) {
            throw std::runtime_error("RawModel: failed to read " + path);
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
            throw std::runtime_error("RawModel: failed to open " + path);
        }
        const std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size < 0 || size % static_cast<std::streamsize>(sizeof(std::uint16_t)) != 0) {
            throw std::runtime_error("RawModel: unexpected index file size: " + path);
        }

        std::vector<std::uint16_t> indices(
            static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(std::uint16_t))));
        if (!indices.empty() &&
            !file.read(reinterpret_cast<char*>(indices.data()), size)) {
            throw std::runtime_error("RawModel: failed to read " + path);
        }
        return indices;
    }

    std::unique_ptr<VertexBuffer> vertexBuffer_;
    std::unique_ptr<IndexBuffer>  indexBuffer_;
    std::unique_ptr<BasicEffect>  effect_;
    std::optional<Texture2D>      texture_;
    int vertexCount_    = 0;
    int primitiveCount_ = 0;
};

} // namespace ChaseCameraSample

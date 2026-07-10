#pragma once

// NOXNA helper -- loads BasicDemo.cs/AlphaDemo.cs's shared "grid" model (grid.x, a
// single 1280x1280 quad textured with a repeating checker pattern) via RawMesh.hpp
// instead of Content.Load<Model>("grid"), for the same DEFERRED.md item #26 reason
// documented in RawMesh.hpp's own header comment. Mirrors the established
// EnableDefaultLighting()-every-frame convention from
// samples/ChaseCamera/src/RawModel.hpp (replicating what real XNA's
// Model.Draw(world,view,projection) convenience overload does NOT do itself -- FNA's
// own Model.cs confirms it only ever sets World/View/Projection -- but what the content
// pipeline's ModelProcessor already baked into the compiled BasicEffect's lighting
// state by default for any mesh with vertex normals).
//
// grid.x is converted via `assimp export` (X has no ASCII-FBX-style parser in
// tools/fbx_ascii2model.py, so it goes through assimp -> OBJ -> tools/obj2model.py, the
// same pipeline used for ChaseCamera's Ground.x) + tools/obj2model.py. Confirmed live
// (screenshot) this hits the exact same `assimp export`-introduced triangle-winding
// inversion already documented in samples/ChaseCamera/missing.md and
// samples/MarbleMaze/missing.md: with the default RasterizerState::CullCounterClockwise
// (set by BasicDemo.cs's/AlphaDemo.cs's own Draw(), matching the C# original), the grid
// was entirely back-face-culled and invisible; forcing RasterizerState::CullNone before
// this mesh's own draw call (and only this mesh's -- the tank continues to use whatever
// cull mode the caller already set) makes it render correctly. Kept as a permanent,
// documented per-asset adjustment, the third confirmed sighting of this exact
// `assimp`-specific quirk in this repo.

#include <memory>
#include <optional>
#include <string>

#include "Microsoft/Xna/Framework/Matrix.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "RawMesh.hpp"

namespace ReachGraphicsDemoSample {

using namespace Microsoft::Xna::Framework;
using namespace Microsoft::Xna::Framework::Graphics;

class GridModel {
public:
    void Load(Content::ContentManager& content, GraphicsDevice& device) {
        mesh_.Load(content.getRootDirectoryProperty(), "grid", device);
        texture_ = content.Load<Texture2D>("checker");

        effect_ = std::make_unique<BasicEffect>(device);
        effect_->setTextureEnabledProperty(true);
        effect_->setTextureProperty(&texture_.value());
    }

    void Draw(const Matrix& world, const Matrix& view, const Matrix& projection) {
        effect_->World = world;
        effect_->View = view;
        effect_->Projection = projection;
        effect_->EnableDefaultLighting();

        auto& device = effect_->getGraphicsDeviceInternal();
        // See the header comment above: grid.x's assimp-exported winding is inverted
        // relative to CNA's default CullCounterClockwise state.
        device.setRasterizerStateProperty(RasterizerState::CullNone);
        mesh_.Draw(*effect_, device);
    }

private:
    RawMesh mesh_;
    std::optional<Texture2D> texture_;
    std::unique_ptr<BasicEffect> effect_;
};

} // namespace ReachGraphicsDemoSample

#pragma once

// Maze.hpp — C++ port of Objects/Maze.cs (XNA 4.0 MarbleMaze sample).
//
// The C# original loads one Content.Load<Model>("maze1") built by a custom
// MarbleMazeProcessor (MarbleMazePipeline/MarbleMazeProcessor.cs) that (a) chains to
// the stock ModelProcessor for rendering and (b) additionally walks the node tree at
// content-BUILD time, flattening every mesh's own absolute-transformed triangle
// positions into a Dictionary<string, List<Vector3>> stashed on Model.Tag -- keyed by
// mesh name ("Floor", "floorSides", "walls"; a 4th mesh, "topWall", is rendered but
// never read back for collision -- confirmed by reading Maze.cs itself).
//
// CNA has neither a Model.Tag equivalent nor build-time custom-ContentProcessor
// extensibility (DEFERRED.md item #18), and -- per this sample's own missing.md --
// Content.Load<Model> additionally hits DEFERRED.md item #26's vertex-corruption bug,
// so this port bypasses Content.Load<Model> entirely (RawMesh.hpp, NOXNA) and
// reconstructs both halves of what MarbleMazeProcessor did, at runtime, from the same
// converted mesh data:
//   - Rendering: maze1.FBX's 6 named mesh nodes (confirmed via `assimp info`:
//     walls/Floor/Floor/Floor/topWall/floorSides -- the 3 "Floor" nodes are 3
//     differently-textured sub-parts, not 3 copies) were each converted to their own
//     _verts.bin/_idx.bin via `assimp export` + tools/obj2model.py (one-off conversion,
//     not folded into tools/ -- see missing.md), and are drawn here as 6 separate
//     RawMesh parts, each with its own bound Texture2D, sharing one BasicEffect (texture
//     swapped per part before each draw -- mirrors what one BasicEffect per
//     ModelMeshPart would have looked like, since Maze.cs's own Draw() configures every
//     part identically except for its pre-existing Texture).
//   - Collision: MarbleMazeProcessor.FindVertices() flattens ALL of a named mesh
//     node's geometry (every sub-part sharing that name) into ONE triangle-position
//     list per name. Reproduced here by concatenating RawMesh::ExpandTrianglePositions()
//     across the matching parts: Ground = Floor_section1 + bridge + Floor_section2
//     (matches tagData["Floor"] combining all 3 XNA "Floor"-named sub-parts); Walls =
//     the "walls" part alone; FloorSides = the "floorSides" part alone. topWall is
//     rendered but excluded from collision, exactly like the original.
//   - Start/Finish/checkpoint bone positions: maze1.FBX's "Start"/"Finish"/
//     "spawnPt1".."spawnPt4" nodes carry no geometry (pure transform markers, matching
//     Model.Bones[...].Transform.Translation in the original) -- extracted once, offline,
//     via a one-off pyassimp script that walks the FBX's node hierarchy and prints each
//     marker node's accumulated world transform (see missing.md), then hardcoded below
//     as constants, since RawMesh's binary sidecars have no channel for bone-only nodes.
//     Values confirmed self-consistent with the exported mesh geometry's own
//     -450..450 X / -165..395 Z bounding box (`assimp info` on maze1.FBX).
//
// All triangle lists / marker positions are in the FBX's baked "model root" space
// (assimp's export already applies each mesh node's absolute transform), matching
// exactly what MarbleMazeProcessor's own `Vector3.Transform(vertex, mesh.AbsoluteTransform)`
// produced -- i.e. the maze's *unrotated* rest pose. Physics/collision in this sample
// deliberately happens entirely in that unrotated frame (see Marble.hpp) -- only
// rendering additionally multiplies by the runtime tilt (Maze::Rotation, driven by
// GameplayScreen's keyboard input), exactly matching the C# original.

#include <vector>

#include "Microsoft/Xna/Framework/Game.hpp"
#include "Microsoft/Xna/Framework/GameTime.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"
#include "Microsoft/Xna/Framework/Content/ContentManager.hpp"
#include "Microsoft/Xna/Framework/Graphics/BasicEffect.hpp"
#include "Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp"
#include "Microsoft/Xna/Framework/Graphics/RasterizerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/SamplerState.hpp"
#include "Microsoft/Xna/Framework/Graphics/Texture2D.hpp"

#include "DrawableComponent3D.hpp"
#include "RawMesh.hpp"
#include "../Misc/TriangleSphereCollisionDetection.hpp"

namespace MarbleMazeSample {

using Microsoft::Xna::Framework::Game;
using Microsoft::Xna::Framework::GameTime;
using Microsoft::Xna::Framework::Vector3;
using Microsoft::Xna::Framework::Content::ContentManager;
using Microsoft::Xna::Framework::Graphics::BasicEffect;
using Microsoft::Xna::Framework::Graphics::GraphicsDevice;
using Microsoft::Xna::Framework::Graphics::SamplerState;
using Microsoft::Xna::Framework::Graphics::Texture2D;

class Maze : public DrawableComponent3D {
public:
    // Checkpoints, in order: Start, then spawnPt1..spawnPt4 (matches
    // Model.Bones traversal order in the C# original, confirmed via the FBX's own
    // node order). Ported as a plain vector (traversed by index in GameplayScreen)
    // instead of C#'s LinkedList<Vector3> -- forward-only traversal either way.
    std::vector<Vector3> Checkpoints;

    Vector3 StartPosition = Vector3(-399.409424f, 7.840668f, -101.592316f);
    Vector3 End = Vector3(401.618652f, 7.840668f, 231.827820f);

    explicit Maze(Game& game) : DrawableComponent3D(game) { preferPerPixelLighting_ = false; }

    void GetCollisionDetails(const BoundingSphere& boundingSphere, IntersectDetails& details, bool light) {
        details.IntersectWithGround = TriangleSphereCollisionDetection::IsSphereCollideWithTriangles(
            ground_, boundingSphere, details.IntersectedGroundTriangle, true);
        details.IntersectWithWalls = TriangleSphereCollisionDetection::IsSphereCollideWithTriangles(
            walls_, boundingSphere, details.IntersectedWallTriangle, light);
        details.IntersectWithFloorSides = TriangleSphereCollisionDetection::IsSphereCollideWithTriangles(
            floorSides_, boundingSphere, details.IntersectedFloorSidesTriangle, true);
    }

    void Draw(const GameTime& gameTime) override {
        (void)gameTime;
        auto& device = getGraphicsDeviceProperty();
        SamplerState originalSampler = device.getSamplerStatesProperty()[0];
        device.getSamplerStatesProperty()[0] = SamplerState::LinearWrap;

        effect_->EnableDefaultLighting();
        effect_->setPreferPerPixelLightingProperty(preferPerPixelLighting_);
        effect_->Projection = CameraPtr->Projection;
        effect_->View = CameraPtr->View;
        effect_->World = FinalWorldTransforms;

        // NOXNA: `assimp export maze1.FBX maze1.obj` re-emits every one of this
        // mesh's triangles wound the opposite way from CNA's default
        // RasterizerState::CullCounterClockwise -- confirmed live (with
        // CullCounterClockwise, every part but "walls" was fully invisible;
        // switching to CullNone made the whole maze appear with no other
        // change). Same class of per-asset winding accommodation ChaseCamera's
        // Ground.x needed (see missing.md) -- not a DEFERRED.md item, just the
        // second confirmed instance of this same assimp-export quirk.
        device.setRasterizerStateProperty(RasterizerState::CullNone);
        for (auto& part : parts_) {
            effect_->setTextureProperty(&part.texture);
            part.mesh.Draw(*effect_, device);
        }
        device.setRasterizerStateProperty(RasterizerState::CullCounterClockwise);

        device.getSamplerStatesProperty()[0] = originalSampler;
    }

protected:
    void LoadContent() override {
        auto& content = getGameProperty().getContentProperty();
        auto& device = getGraphicsDeviceProperty();
        const std::string root = content.getRootDirectoryProperty();

        effect_ = std::make_unique<BasicEffect>(device);
        effect_->setTextureEnabledProperty(true);

        struct PartSpec {
            const char* mesh;
            const char* texture;
        };
        static const PartSpec specs[] = {
            {"Models/maze_walls", "Textures/walllevel1"},   {"Models/maze_floor1", "Textures/floor_section1"},
            {"Models/maze_bridge", "Textures/bridge"},      {"Models/maze_floor2", "Textures/floor_section2"},
            {"Models/maze_topwall", "Textures/topwall_color"}, {"Models/maze_floorsides", "Textures/cliff_sides"},
        };
        for (const auto& spec : specs) {
            Part part;
            part.mesh.Load(root, spec.mesh, device);
            part.texture = content.Load<Texture2D>(spec.texture);
            parts_.push_back(std::move(part));
        }

        // parts_[0]=walls [1]=floor1 [2]=bridge [3]=floor2 [4]=topwall [5]=floorSides
        walls_ = parts_[0].mesh.ExpandTrianglePositions();
        ground_ = parts_[1].mesh.ExpandTrianglePositions();
        {
            auto bridgeTris = parts_[2].mesh.ExpandTrianglePositions();
            ground_.insert(ground_.end(), bridgeTris.begin(), bridgeTris.end());
            auto floor2Tris = parts_[3].mesh.ExpandTrianglePositions();
            ground_.insert(ground_.end(), floor2Tris.begin(), floor2Tris.end());
        }
        floorSides_ = parts_[5].mesh.ExpandTrianglePositions();

        // Checkpoints: Start, then spawnPt1..spawnPt4 -- see the file header comment
        // for how these bone-marker positions were extracted.
        Checkpoints.push_back(StartPosition);
        Checkpoints.push_back(Vector3(-324.481323f, 7.840668f, -110.205505f));
        Checkpoints.push_back(Vector3(-147.593994f, 7.840668f, 49.440582f));
        Checkpoints.push_back(Vector3(324.908081f, 7.840668f, 246.306458f));
        Checkpoints.push_back(Vector3(106.850037f, 7.840668f, 3.451569f));
    }

    void CalculateCollisions() override {
        // Nothing to do -- Maze doesn't collide with itself.
    }
    void CalculateVelocityAndPosition(GameTime&) override {
        // Nothing to do -- Maze doesn't move.
    }
    void CalculateFriction() override {
        // Nothing to do -- Maze is not affected by friction.
    }
    void CalculateAcceleration() override {
        // Nothing to do -- Maze doesn't move.
    }

private:
    struct Part {
        RawMesh mesh;
        Texture2D texture;
    };
    std::vector<Part> parts_;
    std::unique_ptr<BasicEffect> effect_;

    std::vector<Vector3> ground_;
    std::vector<Vector3> walls_;
    std::vector<Vector3> floorSides_;
};

} // namespace MarbleMazeSample

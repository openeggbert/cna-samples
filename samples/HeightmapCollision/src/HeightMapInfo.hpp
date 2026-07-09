#pragma once

// Ported from HeightmapCollision.HeightMapInfo (HeightMapInfo.cs). In the C# original this
// class is populated at *runtime* by reading a HeightMapInfoContent object that a custom
// content-pipeline processor (HeightmapCollisionPipeline.TerrainProcessor /
// HeightMapInfoContent, both content-BUILD-time classes) attached to the terrain Model's
// Tag property when the game's Content project was built. CNA has neither a Model.Tag
// equivalent nor custom-ContentProcessor extensibility (DEFERRED.md item #18), so this port
// constructs a HeightMapInfo directly from the same heightmap data used to build the terrain
// mesh (see Terrain.hpp) instead of reading it back off a loaded Model's Tag.
//
// This class itself -- unlike TerrainProcessor/HeightMapInfoContent -- is ordinary runtime
// game logic in the original (not content-pipeline code), so it is ported here essentially
// unmodified: same fields, same IsOnHeightmap/GetHeight algorithm (bilinear interpolation
// over a 2D height grid), just with the heights array flattened to a 1D std::vector<float>
// (row-major: heights[x + z * width]) in place of C#'s native float[,].

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "Microsoft/Xna/Framework/MathHelper.hpp"
#include "Microsoft/Xna/Framework/Vector3.hpp"

namespace HeightmapCollisionSample {

using namespace Microsoft::Xna::Framework;

/// HeightMapInfo is a collection of data about the heightmap. It includes information about
/// how high the terrain is, and how far apart each vertex is. It also has several functions
/// to get information about the heightmap, including its height at different points, and
/// whether a point is on the heightmap. It is the runtime equivalent of HeightMapInfoContent.
class HeightMapInfo {
public:
    HeightMapInfo() = default;

    // heights is a flattened, row-major [width x height] grid: heights[x + z * width].
    // TerrainScale is the distance between each entry in the heights grid -- e.g. if
    // terrainScale is 30, heights[0,0] and heights[1,0] are 30 units apart.
    HeightMapInfo(std::vector<float> heights, int width, int height, float terrainScale)
        : terrainScale_(terrainScale), heights_(std::move(heights)), width_(width) {
        heightmapWidth_  = static_cast<float>(width  - 1) * terrainScale;
        heightmapHeight_ = static_cast<float>(height - 1) * terrainScale;

        heightmapPosition_.X = -static_cast<float>((width  - 1) / 2) * terrainScale;
        heightmapPosition_.Z = -static_cast<float>((height - 1) / 2) * terrainScale;
    }

    // This function takes in a position, and tells whether or not the position is on the
    // heightmap.
    [[nodiscard]] bool IsOnHeightmap(const Vector3& position) const {
        // first we'll figure out where on the heightmap "position" is...
        Vector3 positionOnHeightmap = position - heightmapPosition_;

        // ... and then check to see if that value goes outside the bounds of the heightmap.
        return (positionOnHeightmap.X > 0 &&
                positionOnHeightmap.X < heightmapWidth_ &&
                positionOnHeightmap.Z > 0 &&
                positionOnHeightmap.Z < heightmapHeight_);
    }

    // This function takes in a position, and returns the heightmap's height at that point.
    // Be careful -- this function will read out of bounds if position isn't on the
    // heightmap! (Same caveat as the C# original -- see the accompanying .htm doc.)
    [[nodiscard]] float GetHeight(const Vector3& position) const {
        // the first thing we need to do is figure out where on the heightmap "position" is.
        // This'll make the math much simpler later.
        Vector3 positionOnHeightmap = position - heightmapPosition_;

        // we'll use integer division to figure out where in the "heights" grid
        // positionOnHeightmap is. Remember that integer division always rounds down, so that
        // the result of these divisions is the indices of the "upper left" of the 4 corners
        // of that cell.
        int left = static_cast<int>(positionOnHeightmap.X) / static_cast<int>(terrainScale_);
        int top  = static_cast<int>(positionOnHeightmap.Z) / static_cast<int>(terrainScale_);

        // next, we'll use fmod to find out how far away we are from the upper left corner of
        // the cell. fmod will give us a value from 0 to terrainScale, which we then divide by
        // terrainScale to normalize 0 to 1.
        float xNormalized = std::fmod(positionOnHeightmap.X, terrainScale_) / terrainScale_;
        float zNormalized = std::fmod(positionOnHeightmap.Z, terrainScale_) / terrainScale_;

        // Now that we've calculated the indices of the corners of our cell, and where we are
        // in that cell, we'll use bilinear interpolation to calculate our height. First,
        // calculate the heights on the bottom and top edge of our cell by interpolating from
        // the left and right sides.
        float topHeight = MathHelper::Lerp(
            At(left, top), At(left + 1, top), xNormalized);

        float bottomHeight = MathHelper::Lerp(
            At(left, top + 1), At(left + 1, top + 1), xNormalized);

        // next, interpolate between those two values to calculate the height at our position.
        return MathHelper::Lerp(topHeight, bottomHeight, zNormalized);
    }

private:
    [[nodiscard]] float At(int x, int z) const {
        return heights_[static_cast<std::size_t>(x + z * width_)];
    }

    float terrainScale_ = 1.0f;
    std::vector<float> heights_;
    int width_ = 0;
    Vector3 heightmapPosition_;
    float heightmapWidth_  = 0.0f;
    float heightmapHeight_ = 0.0f;
};

} // namespace HeightmapCollisionSample

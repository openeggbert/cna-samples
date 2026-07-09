#pragma once

// Loads the flat, triangle-expanded, position-only vertex list that
// tools/fbx_ascii2model.py's optional --picking output writes alongside a model's
// regular .model.json/_verts.bin/_idx.bin files (see that tool's docstring).
//
// XNA's original TrianglePickingSample gets this same data (a Vector3[] with 3
// entries per triangle, already baked into model space) from a Dictionary stored on
// Model.Tag, itself populated at *content-build time* by a custom ContentProcessor
// (TrianglePickingPipeline.TrianglePickingProcessor -- see
// /rv/tmp/XNAGameStudio/Samples/TrianglePickingSample_4_0/TrianglePickingPipeline/
// TrianglePickingProcessor.cs). CNA has no Model.Tag/custom-ContentProcessor
// extensibility point (DEFERRED.md item #18) and, separately, no way to read vertex/
// index data back out of a VertexBuffer/IndexBuffer at runtime either -- both classes
// only expose SetData(), never GetData() (confirmed by reading
// cna/include/Microsoft/Xna/Framework/Graphics/{VertexBuffer,IndexBuffer}.hpp in
// full: every overload is a SetData/SetDataRaw/SetDataWithOptions upload path, there
// is no readback method of any kind) -- so recovering this data from an already-loaded
// CNA Model is not possible at all, not just inconvenient. See missing.md for the full
// writeup (a candidate new DEFERRED.md item, since this is a distinct gap from item
// #18's "no custom ContentProcessor" framing).
//
// The fix used here follows this repo's own established asset story (DEFERRED.md item
// #18: "convert once, offline, into a static runtime format"): tools/fbx_ascii2model.py
// already parses every mesh's raw triangle/vertex data as part of producing
// .model.json's own _verts.bin/_idx.bin files, so it was extended with an optional
// --picking flag that, from that same already-parsed data, additionally emits one flat
// binary file per model: the index-buffer-expanded position list (float32 x,y,z, 3 per
// triangle, no normal/uv), across every mesh block in the source FBX, in mesh-then-
// triangle order -- exactly mirroring TrianglePickingProcessor.FindVertices()'s own
// recursive-node-then-triangle traversal order. This header just reads that file back.

#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace TrianglePicking {

inline std::vector<Microsoft::Xna::Framework::Vector3> LoadPickingVertices(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("TrianglePicking: failed to open picking vertex file: " + path);
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    constexpr std::streamsize floatsPerVertex = 3;
    constexpr std::streamsize bytesPerVertex = floatsPerVertex * static_cast<std::streamsize>(sizeof(float));

    if (size < 0 || size % bytesPerVertex != 0) {
        throw std::runtime_error("TrianglePicking: picking vertex file has unexpected size: " + path);
    }

    std::vector<float> raw(static_cast<std::size_t>(size / static_cast<std::streamsize>(sizeof(float))));
    if (!raw.empty() && !file.read(reinterpret_cast<char*>(raw.data()), size)) {
        throw std::runtime_error("TrianglePicking: failed to read picking vertex file: " + path);
    }

    std::vector<Microsoft::Xna::Framework::Vector3> vertices;
    vertices.reserve(raw.size() / 3);
    for (std::size_t i = 0; i + 2 < raw.size(); i += 3) {
        vertices.emplace_back(raw[i], raw[i + 1], raw[i + 2]);
    }
    return vertices;
}

} // namespace TrianglePicking

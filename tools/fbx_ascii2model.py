#!/usr/bin/env python3
"""
Convert an ASCII FBX 6.1 file to CNA .model.json + binary vertex/index buffers.

Vertex format: VertexPositionNormalTexture (stride 32)
  float x,y,z   (position,  12 bytes)
  float nx,ny,nz (normal,   12 bytes)
  float u,v      (texcoord,  8 bytes)

Index format: uint16 little-endian

Usage:
  python3 tools/fbx_ascii2model.py <input.fbx> <output_dir/basename> [--picking <output.bin>]
  e.g.: python3 tools/fbx_ascii2model.py Content/tank.fbx Content/tank
  produces: Content/tank.model.json
            Content/tank_<meshname>_verts.bin
            Content/tank_<meshname>_idx.bin

Optional --picking <output.bin>: also writes a flat, triangle-expanded, position-only
(x,y,z float32, no normal/uv) vertex list across *all* mesh blocks in the file, 9 floats
(3 vertices) per triangle, concatenated in mesh-then-triangle order. This mirrors what
XNA's TrianglePickingSample's own custom ContentProcessor (TrianglePickingProcessor.
FindVertices()) attaches to Model.Tag at content-build time for per-triangle
ray-intersection tests -- CNA has no Model.Tag/custom-ContentProcessor equivalent (see
DEFERRED.md item #18) and no VertexBuffer/IndexBuffer.GetData() to read the data back
from the GPU at runtime either (confirmed gap -- see DEFERRED.md), so this sidecar file
is generated once, offline, by this same conversion step instead, and read directly by
the C++ port at startup. Added for TrianglePicking (#048); no effect on any existing
2-argument caller.
"""

import sys, os, re, struct, json, math


def mat3_mul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def mat3_vec(m, v):
    return (
        m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2],
        m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2],
        m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2],
    )


def euler_deg_to_matrix(rx, ry, rz):
    """XYZ Euler angles (degrees) to a 3x3 rotation matrix, FBX's default eXYZ order
    (rotation composed as Rz * Ry * Rx, i.e. X applied first)."""
    rx, ry, rz = math.radians(rx), math.radians(ry), math.radians(rz)
    cx, sx = math.cos(rx), math.sin(rx)
    cy, sy = math.cos(ry), math.sin(ry)
    cz, sz = math.cos(rz), math.sin(rz)

    Rx = [[1, 0, 0], [0, cx, -sx], [0, sx, cx]]
    Ry = [[cy, 0, sy], [0, 1, 0], [-sy, 0, cy]]
    Rz = [[cz, -sz, 0], [sz, cz, 0], [0, 0, 1]]

    return mat3_mul(mat3_mul(Rz, Ry), Rx)


def parse_model_transform(lines, block_start, block_end):
    """Reads a Mesh Model node's PreRotation/PostRotation/LclRotation/LclTranslation/
    LclScaling properties (baked by DCC tools like 3ds Max to convert their native
    axis system, e.g. Z-up, into the FBX file's declared axis system). Returns
    (rotation_3x3, scale_xyz, translation_xyz); identity/no-op if all properties
    are absent or zero, matching the C# content pipeline's own node-transform bake."""

    def find_vec3(prop_name):
        pattern = re.compile(
            r'Property:\s*"' + re.escape(prop_name) + r'"\s*,\s*"[^"]*"\s*,\s*"[^"]*"\s*,'
            r'\s*([-\d.eE]+)\s*,\s*([-\d.eE]+)\s*,\s*([-\d.eE]+)')
        for i in range(block_start, block_end):
            m = pattern.search(lines[i])
            if m:
                return (float(m.group(1)), float(m.group(2)), float(m.group(3)))
        return (0.0, 0.0, 0.0)

    pre_rot   = find_vec3("PreRotation")
    post_rot  = find_vec3("PostRotation")
    lcl_rot   = find_vec3("Lcl Rotation")
    lcl_trans = find_vec3("Lcl Translation")
    lcl_scale = find_vec3("Lcl Scaling")
    if lcl_scale == (0.0, 0.0, 0.0):
        lcl_scale = (1.0, 1.0, 1.0)

    R = euler_deg_to_matrix(*pre_rot)
    if lcl_rot != (0.0, 0.0, 0.0):
        R = mat3_mul(R, euler_deg_to_matrix(*lcl_rot))
    if post_rot != (0.0, 0.0, 0.0):
        Rpost = euler_deg_to_matrix(*post_rot)
        Rpost_inv = [[Rpost[j][i] for j in range(3)] for i in range(3)]  # orthonormal: inverse == transpose
        R = mat3_mul(R, Rpost_inv)

    return R, lcl_scale, lcl_trans


def is_identity_transform(transform):
    R, S, T = transform
    identity_R = [[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]]
    close = lambda a, b: abs(a - b) < 1e-9
    return (all(close(R[i][j], identity_R[i][j]) for i in range(3) for j in range(3))
            and S == (1.0, 1.0, 1.0) and T == (0.0, 0.0, 0.0))


def transform_position(p, transform):
    R, S, T = transform
    scaled = (p[0] * S[0], p[1] * S[1], p[2] * S[2])
    rotated = mat3_vec(R, scaled)
    return (rotated[0] + T[0], rotated[1] + T[1], rotated[2] + T[2])


def transform_normal(n, transform):
    R, _S, _T = transform
    return mat3_vec(R, n)


def read_data_block(lines, start):
    """Read a comma-separated multi-line data block starting after line `start`.
    Returns (list_of_float_or_int_strings, next_line_index).
    The block ends when we see a line that doesn't look like continuation data."""
    tokens = []
    i = start
    while i < len(lines):
        line = lines[i].strip()
        # Stop at empty, closing brace, or a new key (word followed by colon or opening brace)
        if not line or line == '}' or (re.match(r'^[A-Za-z]', line) and ':' in line.split()[0]):
            break
        # Strip trailing semicolons/commas and split
        line = line.rstrip(';').rstrip(',')
        parts = [p.strip() for p in line.split(',') if p.strip()]
        tokens.extend(parts)
        i += 1
    return tokens, i


def parse_floats(lines, key_idx):
    """Parse the data block after lines[key_idx] as floats."""
    raw, next_i = read_data_block(lines, key_idx + 1)
    return [float(v) for v in raw if v], next_i


def parse_ints(lines, key_idx):
    """Parse the data block after lines[key_idx] as ints."""
    raw, next_i = read_data_block(lines, key_idx + 1)
    return [int(v) for v in raw if v], next_i


def parse_mesh_block(lines, block_start, block_end):
    """Extract positions, normals, poly_indices, uv_coords, uv_indices from a Mesh block.

    A mesh can have more than one LayerElementUV (e.g. a base diffuse-texture UV set
    plus a separate lightmap UV set, as seen in ReachGraphicsDemo's model.fbx) -- only
    the FIRST one found is kept (matches LayerElementUV index 0, the base texture UV;
    a naive "last UV: line wins" parse would silently pick up a later, unrelated UV
    layer instead, corrupting texture coordinates for any multi-UV-layer mesh). This
    repo's single-UV-channel vertex format (VertexPositionNormalTexture) has no way to
    carry a second UV set regardless, so any additional layer is intentionally dropped,
    not merged.
    """
    positions = []
    poly_indices = []
    normals = []
    uvs = []
    uv_indices = []
    uv_layer_seen = False

    i = block_start
    while i < block_end:
        line = lines[i].strip()

        if line.startswith('Vertices:'):
            # Data may be on the same line after the colon
            after = line[len('Vertices:'):].strip().rstrip(';').rstrip(',')
            if after:
                vals = [float(v.strip()) for v in after.split(',') if v.strip()]
                raw2, _ = read_data_block(lines, i + 1)
                vals += [float(v) for v in raw2 if v]
                positions = [(vals[j], vals[j+1], vals[j+2]) for j in range(0, len(vals)-2, 3)]
                i += 1
            else:
                floats, i = parse_floats(lines, i)
                positions = [(floats[j], floats[j+1], floats[j+2]) for j in range(0, len(floats)-2, 3)]

        elif line.startswith('PolygonVertexIndex:'):
            after = line[len('PolygonVertexIndex:'):].strip().rstrip(';').rstrip(',')
            if after:
                vals = [int(v.strip()) for v in after.split(',') if v.strip()]
                raw2, _ = read_data_block(lines, i + 1)
                vals += [int(v) for v in raw2 if v]
                poly_indices = vals
                i += 1
            else:
                poly_indices, i = parse_ints(lines, i)

        elif line.startswith('Normals:'):
            after = line[len('Normals:'):].strip().rstrip(';').rstrip(',')
            if after:
                vals = [float(v.strip()) for v in after.split(',') if v.strip()]
                raw2, _ = read_data_block(lines, i + 1)
                vals += [float(v) for v in raw2 if v]
                normals = [(vals[j], vals[j+1], vals[j+2]) for j in range(0, len(vals)-2, 3)]
                i += 1
            else:
                floats, i = parse_floats(lines, i)
                normals = [(floats[j], floats[j+1], floats[j+2]) for j in range(0, len(floats)-2, 3)]

        elif line.startswith('UV:') and not line.startswith('UVIndex:'):
            after = line[len('UV:'):].strip().rstrip(';').rstrip(',')
            if after:
                vals = [float(v.strip()) for v in after.split(',') if v.strip()]
                raw2, _ = read_data_block(lines, i + 1)
                vals += [float(v) for v in raw2 if v]
                parsed_uvs = [(vals[j], vals[j+1]) for j in range(0, len(vals)-1, 2)]
                i += 1
            else:
                floats, i = parse_floats(lines, i)
                parsed_uvs = [(floats[j], floats[j+1]) for j in range(0, len(floats)-1, 2)]
            if not uv_layer_seen:
                uvs = parsed_uvs

        elif line.startswith('UVIndex:'):
            after = line[len('UVIndex:'):].strip().rstrip(';').rstrip(',')
            if after:
                vals = [int(v.strip()) for v in after.split(',') if v.strip()]
                raw2, _ = read_data_block(lines, i + 1)
                vals += [int(v) for v in raw2 if v]
                parsed_uv_indices = vals
                i += 1
            else:
                parsed_uv_indices, i = parse_ints(lines, i)
            if not uv_layer_seen:
                uv_indices = parsed_uv_indices
            uv_layer_seen = True
        else:
            i += 1

    return positions, normals, poly_indices, uvs, uv_indices


def triangulate(poly_indices, normals, uv_indices):
    """
    Expand PolygonVertexIndex into per-polygon-vertex tuples (pos_idx, normal_idx, uv_idx).
    Negative index n → real index = (-n)-1, and marks end of polygon.
    Returns list of triangles: each = [(pos_idx, poly_vert_idx), ...] with 3 items.
    poly_vert_idx is the flat index into normals[] / uv_indices[].
    """
    triangles = []
    polygon = []   # list of (pos_idx, flat_idx)
    flat_idx = 0

    for raw in poly_indices:
        if raw < 0:
            pos_idx = (-raw) - 1
        else:
            pos_idx = raw
        polygon.append((pos_idx, flat_idx))
        flat_idx += 1

        if raw < 0:
            # End of polygon — fan triangulate
            for k in range(1, len(polygon) - 1):
                triangles.append([polygon[0], polygon[k], polygon[k+1]])
            polygon = []

    return triangles


def build_buffers(positions, normals, uv_coords, triangles, uv_indices):
    vertex_map = {}
    verts = []
    indices = []

    default_n  = (0.0, 1.0, 0.0)
    default_uv = (0.0, 0.0)

    for tri in triangles:
        for (pos_idx, flat_idx) in tri:
            key = (pos_idx, flat_idx)
            if key not in vertex_map:
                vertex_map[key] = len(verts)
                p  = positions[pos_idx] if pos_idx < len(positions) else (0,0,0)
                n  = normals[flat_idx]   if flat_idx < len(normals)   else default_n
                ui = uv_indices[flat_idx] if flat_idx < len(uv_indices) else -1
                uv = uv_coords[ui] if (0 <= ui < len(uv_coords)) else default_uv
                verts.append((p[0], p[1], p[2], n[0], n[1], n[2], uv[0], uv[1]))
            indices.append(vertex_map[key])

    return verts, indices


def write_buffers(verts, indices, vert_path, idx_path):
    with open(vert_path, 'wb') as f:
        for v in verts:
            f.write(struct.pack('<8f', *v))
    with open(idx_path, 'wb') as f:
        for idx in indices:
            if idx > 65535:
                print(f"  WARNING: index {idx} exceeds uint16, clamping")
                idx = 65535
            f.write(struct.pack('<H', idx))


def find_mesh_blocks(lines):
    """Find all Mesh blocks, return list of (name, start, end) line indices."""
    blocks = []
    mesh_re = re.compile(r'^\s*Model:\s*"Model::([^"]+)",\s*"Mesh"\s*\{')
    i = 0
    while i < len(lines):
        m = mesh_re.match(lines[i])
        if m:
            name = m.group(1).replace(' ', '_')
            # Find matching closing brace
            depth = 1
            j = i + 1
            while j < len(lines) and depth > 0:
                stripped = lines[j].strip()
                depth += stripped.count('{') - stripped.count('}')
                j += 1
            blocks.append((name, i, j))
        i += 1
    return blocks


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    args = sys.argv[1:]
    picking_path = None
    if '--picking' in args:
        pi = args.index('--picking')
        picking_path = args[pi + 1]
        del args[pi:pi + 2]

    fbx_path = args[0]
    out_base = args[1]
    out_dir  = os.path.dirname(out_base) or '.'
    basename = os.path.basename(out_base)
    os.makedirs(out_dir, exist_ok=True)

    with open(fbx_path, encoding='utf-8', errors='replace') as f:
        lines = f.readlines()

    blocks = find_mesh_blocks(lines)
    print(f"Found {len(blocks)} mesh blocks")

    mesh_entries = []
    picking_positions = []  # only populated/used when --picking is given

    for (name, bstart, bend) in blocks:
        positions, normals, poly_indices, uvs, uv_indices = parse_mesh_block(lines, bstart, bend)

        if not positions or not poly_indices:
            print(f"  {name}: skipped (no geometry)")
            continue

        transform = parse_model_transform(lines, bstart, bend)
        if not is_identity_transform(transform):
            print(f"  {name}: applying baked node transform (PreRotation/LclRotation/LclScaling/LclTranslation)")
            positions = [transform_position(p, transform) for p in positions]
            normals = [transform_normal(n, transform) for n in normals]

        triangles = triangulate(poly_indices, normals, uv_indices)
        verts, indices = build_buffers(positions, normals, uvs, triangles, uv_indices)

        if len(indices) > 65535:
            print(f"  WARNING: {name} has {len(indices)} indices (>65535), some may be clamped")

        vert_rel = f"{basename}_{name}_verts.bin"
        idx_rel  = f"{basename}_{name}_idx.bin"
        vert_path = os.path.join(out_dir, vert_rel)
        idx_path  = os.path.join(out_dir, idx_rel)

        write_buffers(verts, indices, vert_path, idx_path)

        tris = len(indices) // 3
        print(f"  {name}: {len(verts)} vertices, {tris} triangles")

        if picking_path is not None:
            picking_positions.extend(verts[idx][0:3] for idx in indices)

        mesh_entries.append({
            "name": name,
            "vertices": vert_rel,
            "indices":  idx_rel,
            "vertexStride": 32,
            "effect": "BasicEffect"
        })

    json_path = out_base + '.model.json'
    with open(json_path, 'w') as f:
        json.dump({"meshes": mesh_entries}, f, indent=2)

    print(f"Written: {json_path} ({len(mesh_entries)} meshes)")

    if picking_path is not None:
        os.makedirs(os.path.dirname(picking_path) or '.', exist_ok=True)
        with open(picking_path, 'wb') as f:
            for (x, y, z) in picking_positions:
                f.write(struct.pack('<3f', x, y, z))
        print(f"Written: {picking_path} "
              f"({len(picking_positions)} picking vertices, {len(picking_positions)//3} triangles)")


if __name__ == '__main__':
    main()

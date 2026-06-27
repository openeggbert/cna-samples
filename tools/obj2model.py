#!/usr/bin/env python3
"""
Convert a Wavefront OBJ file to CNA .model.json + binary vertex/index buffers.

Vertex format: VertexPositionNormalTexture (stride 32)
  float x, y, z      (position,  12 bytes)
  float nx, ny, nz   (normal,     12 bytes)
  float u, v         (texcoord,   8 bytes)

Index format: uint16 (2 bytes each)

Usage:
  python3 tools/obj2model.py <input.obj> <output_dir/basename>
  e.g.:  python3 tools/obj2model.py content/ground.obj Content/ground
  produces: Content/ground.model.json
            Content/ground_verts.bin
            Content/ground_idx.bin
"""

import sys
import os
import struct
import json

def parse_obj(path):
    positions = []
    normals   = []
    texcoords = []
    faces     = []   # list of triangles, each = [(vi, vti, vni), ...]

    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split()
            if parts[0] == 'v':
                positions.append(tuple(float(x) for x in parts[1:4]))
            elif parts[0] == 'vn':
                normals.append(tuple(float(x) for x in parts[1:4]))
            elif parts[0] == 'vt':
                texcoords.append(tuple(float(x) for x in parts[1:3]))
            elif parts[0] == 'f':
                # triangulate (fan), support v, v/vt, v//vn, v/vt/vn
                verts = []
                for token in parts[1:]:
                    spl = token.split('/')
                    vi  = int(spl[0]) - 1
                    vti = int(spl[1]) - 1 if len(spl) > 1 and spl[1] else -1
                    vni = int(spl[2]) - 1 if len(spl) > 2 and spl[2] else -1
                    verts.append((vi, vti, vni))
                # fan triangulation
                for i in range(1, len(verts) - 1):
                    faces.append([verts[0], verts[i], verts[i+1]])

    return positions, normals, texcoords, faces


def build_buffers(positions, normals, texcoords, faces):
    vertex_map = {}
    vertices   = []   # list of (px,py,pz, nx,ny,nz, u,v)
    indices    = []

    default_normal   = (0.0, 1.0, 0.0)
    default_texcoord = (0.0, 0.0)

    for tri in faces:
        for (vi, vti, vni) in tri:
            key = (vi, vti, vni)
            if key not in vertex_map:
                vertex_map[key] = len(vertices)
                p  = positions[vi]
                n  = normals[vni]   if vni >= 0 and vni < len(normals)   else default_normal
                tc = texcoords[vti] if vti >= 0 and vti < len(texcoords) else default_texcoord
                vertices.append((p[0], p[1], p[2], n[0], n[1], n[2], tc[0], tc[1]))
            indices.append(vertex_map[key])

    return vertices, indices


def write_buffers(vertices, indices, out_base):
    vert_path = out_base + '_verts.bin'
    idx_path  = out_base + '_idx.bin'

    with open(vert_path, 'wb') as f:
        for v in vertices:
            f.write(struct.pack('<8f', *v))

    with open(idx_path, 'wb') as f:
        for i in indices:
            if i > 65535:
                raise ValueError(f"Index {i} exceeds uint16 range")
            f.write(struct.pack('<H', i))

    return vert_path, idx_path


def write_model_json(mesh_name, vert_rel, idx_rel, out_json_path):
    doc = {
        "meshes": [
            {
                "name": mesh_name,
                "vertices": vert_rel,
                "indices":  idx_rel,
                "vertexStride": 32,
                "effect": "BasicEffect"
            }
        ]
    }
    with open(out_json_path, 'w') as f:
        json.dump(doc, f, indent=2)


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    obj_path = sys.argv[1]
    out_base = sys.argv[2]          # e.g. "Content/ground"
    out_dir  = os.path.dirname(out_base) or '.'
    basename = os.path.basename(out_base)

    os.makedirs(out_dir, exist_ok=True)

    positions, normals, texcoords, faces = parse_obj(obj_path)
    vertices, indices = build_buffers(positions, normals, texcoords, faces)

    vert_path, idx_path = write_buffers(vertices, indices, out_base)
    json_path = out_base + '.model.json'

    # Relative paths for the JSON (relative to Content root = out_dir)
    vert_rel = basename + '_verts.bin'
    idx_rel  = basename + '_idx.bin'

    write_model_json(basename, vert_rel, idx_rel, json_path)

    print(f"Vertices: {len(vertices)}  Triangles: {len(indices)//3}")
    print(f"Written: {json_path}")
    print(f"         {vert_path}")
    print(f"         {idx_path}")


if __name__ == '__main__':
    main()

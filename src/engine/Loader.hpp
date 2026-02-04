#pragma once

#ifndef CGLTF_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION
#endif

#include "Builder.hpp"
#include "Renderer.hpp"
#include "Vertex.hpp"

#include <cgltf/cgltf.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

// Find an attribute accessor on a primitive (e.g. POSITION, NORMAL, TEXCOORD_0)
static const cgltf_accessor *FindAttr(const cgltf_primitive &prim,
                                      cgltf_attribute_type type,
                                      int index = 0) {
  for (cgltf_size i = 0; i < prim.attributes_count; i++) {
    const cgltf_attribute &a = prim.attributes[i];
    if (a.type == type && a.index == index)
      return a.data;
  }
  return nullptr;
}

// Read indices as uint32_t (cgltf handles component type conversion)
static void ReadIndicesU32(const cgltf_accessor *acc,
                           std::vector<uint32_t> &out) {
  out.resize(acc->count);
  for (cgltf_size i = 0; i < acc->count; i++) {
    out[i] = (uint32_t)cgltf_accessor_read_index(acc, i);
  }
}

// Read float vectors (cgltf converts normalized ints etc. to float for you)
static void ReadFloatN(const cgltf_accessor *acc, cgltf_size i, float *dst,
                       int n) {
  // cgltf_accessor_read_float writes up to acc->type's #components
  // We'll zero first then read.
  for (int k = 0; k < n; k++)
    dst[k] = 0.0f;
  cgltf_accessor_read_float(acc, i, dst, n);
}

static bool ExtractPrimitiveCPU(const cgltf_primitive &prim,
                                std::vector<Vertex> &outVertices,
                                std::vector<uint32_t> &outIndices) {
  // Start simple: triangles only
  if (prim.type != cgltf_primitive_type_triangles)
    return false;

  const cgltf_accessor *pos = FindAttr(prim, cgltf_attribute_type_position);
  if (!pos)
    return false;

  const cgltf_accessor *nor = FindAttr(prim, cgltf_attribute_type_normal);
  const cgltf_accessor *uv0 = FindAttr(prim, cgltf_attribute_type_texcoord, 0);
  const cgltf_accessor *col = FindAttr(prim, cgltf_attribute_type_color, 0);
  // const cgltf_accessor* tan = FindAttr(prim, cgltf_attribute_type_tangent);

  const cgltf_size vtxCount = pos->count;
  outVertices.resize(vtxCount);

  float tmp4[4] = {0, 0, 0, 1};

  for (cgltf_size i = 0; i < vtxCount; i++) {
    Vertex vertex{};

    // POSITION (required)
    ReadFloatN(pos, i, tmp4, 3);
    vertex.px = tmp4[0];
    vertex.py = tmp4[1];
    vertex.pz = tmp4[2];

    // NORMAL (optional)
    if (nor) {
      ReadFloatN(nor, i, tmp4, 3);
      vertex.nx = tmp4[0];
      vertex.ny = tmp4[1];
      vertex.nz = tmp4[2];
    } else {
      // Safe default if missing (you can also compute later)
      vertex.nx = 0.0f;
      vertex.ny = 1.0f;
      vertex.nz = 0.0f;
    }

    // TEXCOORD_0 (optional)
    if (uv0) {
      ReadFloatN(uv0, i, tmp4, 2);
      vertex.ux = tmp4[0];
      vertex.uy = tmp4[1];
      // If your renderer expects flipped V, do: v.v = 1.0f - v.v;
    } else {
      vertex.ux = 0.0f;
      vertex.uy = 0.0f;
    }

    // COLOR_0 (optional). glTF may store RGB or RGBA.
    if (col) {
      // cgltf will give floats and expand/convert normalized types.
      // But components can be 3 or 4; safest is read 4 and default alpha=1.
      tmp4[0] = tmp4[1] = tmp4[2] = 1.0f;
      tmp4[3] = 1.0f;
      cgltf_accessor_read_float(col, i, tmp4, 4);
      vertex.r = tmp4[0];
      vertex.g = tmp4[1];
      vertex.b = tmp4[2]; //  vertex.a = tmp4[3];
    } else {
      // Default white
      vertex.r = 1.0f;
      vertex.g = 1.0f;
      vertex.b = 1.0f; //  vertex.a = 1.0f;
    }

    outVertices[i] = vertex;
  }

  // INDICES (optional)
  if (prim.indices) {
    ReadIndicesU32(prim.indices, outIndices);
  } else {
    // Non-indexed: create a trivial index buffer 0..N-1
    outIndices.resize(vtxCount);
    for (uint32_t i = 0; i < (uint32_t)vtxCount; i++)
      outIndices[i] = i;
  }

  return true;
}

Model loadModel(Renderer &renderer, const char *filePath,
                VertexDescriptor vertexDescriptor) {
  Builder builder = Builder(vertexDescriptor);

  cgltf_options options{};
  cgltf_data *data = nullptr;

  cgltf_result result = cgltf_parse_file(&options, filePath, &data);
  if (result != cgltf_result_success) {
    std::cerr << "cgltf_parse_file " << filePath << " failed: " << (int)result
              << std::endl;
    std::abort();
  }

  // For .glb, this is usually enough, but still call it to be safe.
  // Base dir resolves external resources if any exist.
  result = cgltf_load_buffers(&options, data, /*base_dir*/ nullptr);
  if (result != cgltf_result_success) {
    std::cerr << "cgltf_load_buffers " << filePath << " failed: " << (int)result
              << std::endl;
    std::abort();
  }

  result = cgltf_validate(data);
  if (result != cgltf_result_success) {
    std::cerr << "cgltf_validate " << filePath << " failed: " << (int)result
              << std::endl;
    std::abort();
  }

  for (cgltf_size mi = 0; mi < data->meshes_count; mi++) {
    const cgltf_mesh &mesh = data->meshes[mi];

    for (cgltf_size pi = 0; pi < mesh.primitives_count; pi++) {
      const cgltf_primitive &prim = mesh.primitives[pi];

      std::vector<Vertex> vertices;
      std::vector<uint32_t> indices;

      if (!ExtractPrimitiveCPU(prim, vertices, indices)) {
        std::cout << "Skipping non-triangle or malformed." << std::endl;
        continue; // skip non-triangle or malformed
      }

      builder.addVertices(vertices);
      builder.addIndices(indices);

      builder.generateMesh(renderer);
    }
  }

  return builder.buildModel(renderer);
}

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

static const cgltf_image *
GetImageFromTexView(const cgltf_texture_view &rawTextureView) {
  if (!rawTextureView.texture || !rawTextureView.texture->image) {
    std::cout << "texture or texture image not available" << std::endl;

    return nullptr;
  } else {
    std::cout << "rawTextureView.texture->name: "
              << rawTextureView.texture->name << std::endl;

    // std::cout << "!rawTextureView.texture: " << !rawTextureView.texture
    //           << std::endl;
    // std::cout << "!rawTextureView.texture->image: "
    //           << !rawTextureView.texture->image << std::endl;
    // std::cout << "raw texture is not available, or raw texture image is not "
    //              "available..."
    //           << std::endl;
  }

  return rawTextureView.texture->image;
}

static Material *GenerateMaterial(Renderer &renderer,
                                  // TextureCache& cache,
                                  const cgltf_material *rawMaterial,
                                  const char *baseDirectory) {
  if (!rawMaterial) {
    return nullptr;
  }

  // auto out = std::make_shared<MaterialTextures>();

  // Albedo (sRGB)
  if (rawMaterial->has_pbr_metallic_roughness) {
    const cgltf_image *albedoImg = GetImageFromTexView(
        rawMaterial->pbr_metallic_roughness.base_color_texture);

    // out->albedo = LoadGpuTextureFromCgltfImage(
    //     renderer, cache, albedoImg, base_dir,
    //     VK_FORMAT_R8G8B8A8_SRGB, /*genMips=*/true);
  }

  // Normal (linear UNORM)
  {
    const cgltf_image *normalImg =
        GetImageFromTexView(rawMaterial->normal_texture);

    // out->normal = LoadGpuTextureFromCgltfImage(
    //     renderer, cache, normalImg, base_dir,
    //     VK_FORMAT_R8G8B8A8_UNORM, true);
  }

  // // Metallic + Roughness: glTF usually packs them into ONE texture
  // std::shared_ptr<GpuTexture> mrPacked;
  // if (rawMaterial->has_pbr_metallic_roughness) {
  //   const cgltf_image* mrImg =
  //       GetImageFromTexView(mat->pbr_metallic_roughness.metallic_roughness_texture);

  //   mrPacked = LoadGpuTextureFromCgltfImage(
  //       renderer, cache, mrImg, base_dir,
  //       VK_FORMAT_R8G8B8A8_UNORM, true);
  // }

  // // Your slots:
  // out->roughness = mrPacked; // alias
  // out->metallic  = mrPacked; // alias

  // // If you truly have separate metallic/roughness textures (nonstandard),
  // // you can override out->roughness / out->metallic here when you detect
  // them.

  // if (!out->albedo && !out->normal && !out->roughness && !out->metallic)
  // return nullptr; return out;
}

// Find an attribute accessor on a primitive (e.g. POSITION, NORMAL, TEXCOORD_0)
static const cgltf_accessor *FindAttr(const cgltf_primitive &prim,
                                      cgltf_attribute_type type,
                                      int index = 0) {
  for (cgltf_size i = 0; i < prim.attributes_count; i++) {
    const cgltf_attribute &a = prim.attributes[i];
    if (a.type == type && a.index == index) {
      return a.data;
    }
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
  for (int k = 0; k < n; k++) {
    dst[k] = 0.0f;
  }
  cgltf_accessor_read_float(acc, i, dst, n);
}

static bool ExtractPrimitiveCPU(const cgltf_primitive &prim,
                                std::vector<Vertex> &outVertices,
                                std::vector<uint32_t> &outIndices) {
  if (prim.type != cgltf_primitive_type_triangles) {
    return false;
  }

  const cgltf_accessor *position =
      FindAttr(prim, cgltf_attribute_type_position);
  if (!position) {
    return false;
  }

  const cgltf_accessor *normal = FindAttr(prim, cgltf_attribute_type_normal);
  const cgltf_accessor *tangent = FindAttr(prim, cgltf_attribute_type_tangent);
  const cgltf_accessor *uv = FindAttr(prim, cgltf_attribute_type_texcoord, 0);
  const cgltf_accessor *color = FindAttr(prim, cgltf_attribute_type_color, 0);

  const cgltf_size vtxCount = position->count;
  outVertices.resize(vtxCount);

  float temp[4] = {0, 0, 0, 1};

  for (cgltf_size i = 0; i < vtxCount; i++) {
    Vertex vertex{};

    // POSITION (required)
    ReadFloatN(position, i, temp, 3);
    vertex.px = temp[0];
    vertex.py = temp[1];
    vertex.pz = temp[2];

    // TANGENT (optional)
    if (tangent) {
      ReadFloatN(tangent, i, temp, 4);
      vertex.tx = temp[0];
      vertex.ty = temp[1];
      vertex.tz = temp[2];
      vertex.tw = temp[3];
    } else {
      vertex.tx = 0.0;
      vertex.ty = 0.0;
      vertex.tz = 0.0;
      vertex.tw = 0.0;
    }

    // NORMAL (optional)
    if (normal) {
      ReadFloatN(normal, i, temp, 3);
      vertex.nx = temp[0];
      vertex.ny = temp[1];
      vertex.nz = temp[2];
    } else {
      // Safe default if missing (you can also compute later)
      vertex.nx = 0.0f;
      vertex.ny = 1.0f;
      vertex.nz = 0.0f;
    }

    // TEXCOORD (optional)
    if (uv) {
      ReadFloatN(uv, i, temp, 2);
      vertex.ux = temp[0];
      vertex.uy = temp[1];
    } else {
      vertex.ux = 0.0f;
      vertex.uy = 0.0f;
    }

    // COLOR (optional). glTF may store RGB or RGBA.
    if (color) {
      cgltf_accessor_read_float(color, i, temp, 4);
      vertex.r = temp[0];
      vertex.g = temp[1];
      vertex.b = temp[2];
    } else {
      // Default white
      vertex.r = 1.0f;
      vertex.g = 1.0f;
      vertex.b = 1.0f;
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

Model loadModel(Renderer &renderer, VertexDescriptor vertexDescriptor,
                const char *filePath, const char *baseDirectory) {
  Builder builder = Builder(vertexDescriptor);

  cgltf_options options{};
  cgltf_data *data = nullptr;

  cgltf_result result = cgltf_parse_file(&options, filePath, &data);
  if (result != cgltf_result_success) {
    std::cerr << "cgltf_parse_file " << filePath << " failed: " << (int)result
              << std::endl;
    std::abort();
  }

  result = cgltf_load_buffers(&options, data, baseDirectory);
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

  // TextureCache texCache;

  for (cgltf_size mi = 0; mi < data->meshes_count; mi++) {
    const cgltf_mesh &mesh = data->meshes[mi];

    for (cgltf_size pi = 0; pi < mesh.primitives_count; pi++) {
      const cgltf_primitive &prim = mesh.primitives[pi];

      std::vector<Vertex> vertices;
      std::vector<uint32_t> indices;

      if (!ExtractPrimitiveCPU(prim, vertices, indices)) {
        std::cout << "Skipping non-triangle or malformed." << std::endl;
        continue;
      }

      // auto materialTex = GenerateMaterial(renderer, prim.material, filePath);

      builder.addVertices(vertices);
      builder.addIndices(indices);

      builder.generateMesh(renderer);
    }
  }

  // cgltf_free(data);

  return builder.buildModel(renderer);
}

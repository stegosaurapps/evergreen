#pragma once

#ifndef CGLTF_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION
#endif

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#include "Builder.hpp"
#include "Renderer.hpp"
#include "Vertex.hpp"

#include <cgltf/cgltf.h>

#include <nothings/stb_image.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

static bool ReadAllBytes(const char *path, std::vector<uint8_t> &out) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f)
    return false;
  size_t size = (size_t)f.tellg();
  f.seekg(0);
  out.resize(size);
  f.read((char *)out.data(), size);
  return true;
}

static bool GetImageBytesFromCgltf(const cgltf_image *image,
                                   const char *baseDirectory,
                                   std::vector<uint8_t> &outBytes) {
  outBytes.clear();
  if (!image) {
    return false;
  }

  // URI path
  if (image->uri && image->uri[0] != '\0') {
    std::string full = std::string(baseDirectory) + std::string(image->uri);
    return ReadAllBytes(full.c_str(), outBytes);
  }

  return false;
}

static bool DecodeToRGBA8(const std::vector<uint8_t> &bytes,
                          std::vector<uint8_t> &outPixels, int &outW,
                          int &outH) {
  int comp = 0;
  stbi_uc *px = stbi_load_from_memory(bytes.data(), (int)bytes.size(), &outW,
                                      &outH, &comp, 4);
  if (!px)
    return false;

  outPixels.assign(px, px + (size_t)outW * (size_t)outH * 4);
  stbi_image_free(px);
  return true;
}

static Texture LoadCgltfImageAsTexture(Renderer &renderer,
                                       const cgltf_image *image,
                                       const char *baseDirectory,
                                       VkFormat format) {
  std::vector<uint8_t> fileBytes;
  if (!GetImageBytesFromCgltf(image, baseDirectory, fileBytes)) {
    std::cerr << "!GetImageBytesFromCgltf(image, baseDirectory, fileBytes)"
              << std::endl;
    std::abort();
  }

  std::vector<uint8_t> rgba;
  int w = 0, h = 0;
  if (!DecodeToRGBA8(fileBytes, rgba, w, h)) {
    std::cerr << "!DecodeToRGBA8(fileBytes, rgba, w, h)" << std::endl;
    std::abort();
  }

  return CreateTextureFromRGBA8(renderer.physicalDevice(), renderer.device(),
                                renderer.commandPool(), renderer.grapicsQueue(),
                                rgba.data(), (uint32_t)w, (uint32_t)h, format);
}

static const cgltf_image *
GetImageFromTexView(const cgltf_texture_view &rawTextureView) {
  return rawTextureView.texture->image;
}

static VkDescriptorSet CreateMaterialDescriptorSet(
    Renderer &renderer, VkDescriptorSetLayout materialSetLayout,
    Texture &albedo, Texture &metalRough, Texture &normal) {
  VkDevice device = renderer.device();
  VkDescriptorPool pool = renderer.materialDescriptorPool();

  if (pool == VK_NULL_HANDLE) {
    std::cerr << "CreateMaterialDescriptorSet: "
                 "renderer.materialDescriptorPool() is VK_NULL_HANDLE"
              << std::endl;
    std::abort();
  }
  if (materialSetLayout == VK_NULL_HANDLE) {
    std::cerr
        << "CreateMaterialDescriptorSet: materialSetLayout is VK_NULL_HANDLE"
        << std::endl;
    std::abort();
  }

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  descriptorSetAllocateInfo.descriptorPool = pool;
  descriptorSetAllocateInfo.descriptorSetCount = 1;
  descriptorSetAllocateInfo.pSetLayouts = &materialSetLayout;

  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                               &descriptorSet) != VK_SUCCESS) {
    std::cerr << "CreateMaterialDescriptorSet: vkAllocateDescriptorSets failed"
              << std::endl;
    std::abort();
  }

  // Must be valid (no null view/sampler)
  VkDescriptorImageInfo albedoInfo{};
  albedoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  albedoInfo.imageView = albedo.view();
  albedoInfo.sampler = albedo.sampler();

  VkDescriptorImageInfo metallicRoughnessInfo{};
  metallicRoughnessInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  metallicRoughnessInfo.imageView = metalRough.view();
  metallicRoughnessInfo.sampler = metalRough.sampler();

  VkDescriptorImageInfo normalInfo{};
  normalInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  normalInfo.imageView = normal.view();
  normalInfo.sampler = normal.sampler();

  VkWriteDescriptorSet writeDescriptorSet[3]{};

  writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet[0].dstSet = descriptorSet;
  writeDescriptorSet[0].dstBinding = 0;
  writeDescriptorSet[0].descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorSet[0].descriptorCount = 1;
  writeDescriptorSet[0].pImageInfo = &albedoInfo;

  writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet[1].dstSet = descriptorSet;
  writeDescriptorSet[1].dstBinding = 1;
  writeDescriptorSet[1].descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorSet[1].descriptorCount = 1;
  writeDescriptorSet[1].pImageInfo = &metallicRoughnessInfo;

  writeDescriptorSet[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet[2].dstSet = descriptorSet;
  writeDescriptorSet[2].dstBinding = 2;
  writeDescriptorSet[2].descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorSet[2].descriptorCount = 1;
  writeDescriptorSet[2].pImageInfo = &normalInfo;

  vkUpdateDescriptorSets(device, 3, writeDescriptorSet, 0, nullptr);

  return descriptorSet;
}

static Material *GenerateMaterial(Renderer &renderer,
                                  VkDescriptorSetLayout materialSetLayout,
                                  const cgltf_material *rawMaterial,
                                  const char *baseDirectory) {
  if (!rawMaterial) {
    return nullptr;
  }

  if (!rawMaterial->has_pbr_metallic_roughness) {
    return nullptr;
  }

  const cgltf_image *albedoImage = GetImageFromTexView(
      rawMaterial->pbr_metallic_roughness.base_color_texture);

  const cgltf_image *metallicImage = GetImageFromTexView(
      rawMaterial->pbr_metallic_roughness.metallic_roughness_texture);

  const cgltf_image *normalImage =
      GetImageFromTexView(rawMaterial->normal_texture);

  Material *material = new Material();

  Texture albedoTexture = LoadCgltfImageAsTexture(
      renderer, albedoImage, baseDirectory, VK_FORMAT_R8G8B8A8_SRGB);

  Texture metallicTexture = LoadCgltfImageAsTexture(
      renderer, metallicImage, baseDirectory, VK_FORMAT_R8G8B8A8_UNORM);

  Texture normalTexture = LoadCgltfImageAsTexture(
      renderer, normalImage, baseDirectory, VK_FORMAT_R8G8B8A8_UNORM);

  VkDescriptorSet materialDescriptorSet =
      CreateMaterialDescriptorSet(renderer, materialSetLayout, albedoTexture,
                                  metallicTexture, normalTexture);

  material->init(materialDescriptorSet, albedoTexture, metallicTexture,
                 normalTexture);

  return material;
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
                VkDescriptorSetLayout descriptorSetLayout, const char *filePath,
                const char *baseDirectory) {
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

  for (cgltf_size mi = 0; mi < data->meshes_count; mi++) {
    const cgltf_mesh &mesh = data->meshes[mi];

    for (cgltf_size pi = 0; pi < mesh.primitives_count; pi++) {
      const cgltf_primitive &primitive = mesh.primitives[pi];

      std::vector<Vertex> vertices;
      std::vector<uint32_t> indices;

      if (!ExtractPrimitiveCPU(primitive, vertices, indices)) {
        continue;
      }

      builder.addVertices(vertices);
      builder.addIndices(indices);

      Material *material = GenerateMaterial(renderer, descriptorSetLayout,
                                            primitive.material, baseDirectory);

      builder.addMaterial(material);

      builder.generateMesh(renderer);
    }
  }

  return builder.buildModel(renderer);
}

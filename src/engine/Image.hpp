#pragma once

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

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

static bool DecodeToRGBA32F(const std::vector<uint8_t> &bytes,
                            std::vector<float> &outRGBA, int &outW, int &outH) {
  int comp = 0;
  float *px = stbi_loadf_from_memory(bytes.data(), (int)bytes.size(), &outW,
                                     &outH, &comp, 4);
  if (!px)
    return false;

  outRGBA.assign(px, px + (size_t)outW * (size_t)outH * 4);
  stbi_image_free(px);
  return true;
}

static VkCommandBuffer BeginOneTimeCmd(VkDevice device, VkCommandPool pool) {
  VkCommandBufferAllocateInfo ai{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  ai.commandPool = pool;
  ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  ai.commandBufferCount = 1;

  VkCommandBuffer cmd = VK_NULL_HANDLE;
  vkAllocateCommandBuffers(device, &ai, &cmd);

  VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &bi);
  return cmd;
}

static void EndOneTimeCmd(VkDevice device, VkQueue queue, VkCommandPool pool,
                          VkCommandBuffer cmd) {
  vkEndCommandBuffer(cmd);

  VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cmd;

  vkQueueSubmit(queue, 1, &si, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, pool, 1, &cmd);
}

static void TransitionImage(VkCommandBuffer cmd, VkImage image,
                            VkImageLayout oldLayout, VkImageLayout newLayout,
                            VkImageAspectFlags aspect, uint32_t mipLevels,
                            uint32_t layerCount) {
  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = aspect;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = layerCount;

  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  }

  vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);
}

static Texture CreateCubemapFromRGBA32F(Renderer &renderer,
                                        const float *faceRGBA[6],
                                        uint32_t width, uint32_t height,
                                        uint32_t mipLevels, VkFormat format) {
  VkPhysicalDevice phys = renderer.physicalDevice();
  VkDevice device = renderer.device();

  // ---- staging buffer (all faces, all mips packed) ----
  // Here we assume only mip 0 data is provided (common for diffuse cube).
  // For specular prefiltered, you’ll call this with mip data too (next
  // section).
  const uint32_t bytesPerPixel = 16; // 4 floats = 16 bytes
  VkDeviceSize faceSize =
      (VkDeviceSize)width * (VkDeviceSize)height * bytesPerPixel;
  VkDeviceSize totalSize = faceSize * 6;

  VkBuffer staging = VK_NULL_HANDLE;
  VkDeviceMemory stagingMem = VK_NULL_HANDLE;

  if (!CreateBuffer(phys, device, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    staging, stagingMem)) {
    std::cerr << "Failed to CreateBuffer for Cubemap Texture" << std::endl;
    std::abort();
  }

  void *mapped = nullptr;
  vkMapMemory(device, stagingMem, 0, totalSize, 0, &mapped);
  uint8_t *dst = (uint8_t *)mapped;

  for (int face = 0; face < 6; ++face) {
    memcpy(dst + face * faceSize, faceRGBA[face], (size_t)faceSize);
  }

  vkUnmapMemory(device, stagingMem);

  // ---- create cubemap image ----
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;

  VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  ici.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = format;
  ici.extent = {width, height, 1};
  ici.mipLevels = mipLevels;
  ici.arrayLayers = 6;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_OPTIMAL;
  ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &ici, nullptr, &image) != VK_SUCCESS) {
    std::cerr << "(memType == UINT32_MAX) for Cubemap Texture" << std::endl;
    std::abort();
  }

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device, image, &req);

  uint32_t memType = FindMemoryType(phys, req.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memType == UINT32_MAX) {
    std::cerr << "(memType == UINT32_MAX) for Cubemap Texture" << std::endl;
    std::abort();
  }

  VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = memType;

  if (vkAllocateMemory(device, &mai, nullptr, &memory) != VK_SUCCESS) {
    std::cerr << "Failed to vkAllocateMemory for Cubemap Texture" << std::endl;
    std::abort();
  }

  vkBindImageMemory(device, image, memory, 0);

  // ---- copy staging -> image ----
  VkCommandBuffer cmd = BeginOneTimeCmd(device, renderer.commandPool());

  TransitionImage(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 6);

  std::vector<VkBufferImageCopy> regions;
  regions.reserve(6);

  for (uint32_t face = 0; face < 6; ++face) {
    VkBufferImageCopy r{};
    r.bufferOffset = (VkDeviceSize)face * faceSize;
    r.bufferRowLength = 0;
    r.bufferImageHeight = 0;
    r.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    r.imageSubresource.mipLevel = 0;
    r.imageSubresource.baseArrayLayer = face;
    r.imageSubresource.layerCount = 1;
    r.imageExtent = {width, height, 1};
    regions.push_back(r);
  }

  vkCmdCopyBufferToImage(cmd, staging, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         (uint32_t)regions.size(), regions.data());

  TransitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 6);

  EndOneTimeCmd(device, renderer.grapicsQueue(), renderer.commandPool(), cmd);

  // ---- create view (CUBE) ----
  VkImageView view = VK_NULL_HANDLE;
  VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  vci.image = image;
  vci.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
  vci.format = format;
  vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vci.subresourceRange.baseMipLevel = 0;
  vci.subresourceRange.levelCount = mipLevels;
  vci.subresourceRange.baseArrayLayer = 0;
  vci.subresourceRange.layerCount = 6;

  if (vkCreateImageView(device, &vci, nullptr, &view) != VK_SUCCESS) {
    std::cerr << "Failed to vkCreateImageView for Cubemap Texture" << std::endl;
    std::abort();
  }

  // ---- sampler ----
  VkSampler sampler = VK_NULL_HANDLE;
  VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  sci.magFilter = VK_FILTER_LINEAR;
  sci.minFilter = VK_FILTER_LINEAR;
  sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.minLod = 0.0f;
  sci.maxLod = (float)mipLevels;

  if (vkCreateSampler(device, &sci, nullptr, &sampler) != VK_SUCCESS) {
    std::cerr << "Failed to vkCreateSampler for Cubemap Texture" << std::endl;
    std::abort();
  }

  // cleanup staging
  vkDestroyBuffer(device, staging, nullptr);
  vkFreeMemory(device, stagingMem, nullptr);

  Texture texture;

  texture.init(width, height, mipLevels, format, image, memory, view, sampler);

  return texture;
}

static Texture CreateCubemapArrayFromRGBA32F(Renderer &renderer,
                                             const float *pixels[/*level*/][6],
                                             uint32_t levels, uint32_t width,
                                             uint32_t height, VkFormat format) {
  VkPhysicalDevice phys = renderer.physicalDevice();
  VkDevice device = renderer.device();

  const uint32_t bytesPerPixel = 16; // RGBA32F
  VkDeviceSize faceSize =
      (VkDeviceSize)width * (VkDeviceSize)height * bytesPerPixel;
  VkDeviceSize levelSize = faceSize * 6;
  VkDeviceSize totalSize = levelSize * levels;

  VkBuffer staging = VK_NULL_HANDLE;
  VkDeviceMemory stagingMem = VK_NULL_HANDLE;

  if (!CreateBuffer(phys, device, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    staging, stagingMem)) {
    std::cerr << "Failed to CreateBuffer for Cubemap with levels Texture"
              << std::endl;
    std::abort();
  }

  void *mapped = nullptr;
  vkMapMemory(device, stagingMem, 0, totalSize, 0, &mapped);
  uint8_t *dst = (uint8_t *)mapped;

  for (uint32_t lvl = 0; lvl < levels; ++lvl) {
    VkDeviceSize base = (VkDeviceSize)lvl * levelSize;
    for (uint32_t face = 0; face < 6; ++face) {
      VkDeviceSize off = base + (VkDeviceSize)face * faceSize;
      memcpy(dst + off, pixels[lvl][face], (size_t)faceSize);
    }
  }

  vkUnmapMemory(device, stagingMem);

  // Create cube-array image
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;

  VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  ici.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = format;
  ici.extent = {width, height, 1};
  ici.mipLevels = 1;
  ici.arrayLayers = 6 * levels;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_OPTIMAL;
  ici.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &ici, nullptr, &image) != VK_SUCCESS) {
    std::cerr << "Failed to vkCreateImage for Cubemap with levels Texture"
              << std::endl;
    std::abort();
  }

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(device, image, &req);

  uint32_t memType = FindMemoryType(phys, req.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memType == UINT32_MAX) {
    std::cerr
        << "Failed (memType == UINT32_MAX) for Cubemap with levels Texture"
        << std::endl;
    std::abort();
  }

  VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  mai.allocationSize = req.size;
  mai.memoryTypeIndex = memType;

  if (vkAllocateMemory(device, &mai, nullptr, &memory) != VK_SUCCESS) {
    std::cerr << "Failed to vkAllocateMemory for Cubemap with levels Texture"
              << std::endl;
    std::abort();
  }
  vkBindImageMemory(device, image, memory, 0);

  VkCommandBuffer cmd = BeginOneTimeCmd(device, renderer.commandPool());

  TransitionImage(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_ASPECT_COLOR_BIT,
                  /*mipLevels=*/1,
                  /*layerCount=*/6 * levels);

  std::vector<VkBufferImageCopy> regions;
  regions.reserve(levels * 6);

  for (uint32_t lvl = 0; lvl < levels; ++lvl) {
    VkDeviceSize base = (VkDeviceSize)lvl * levelSize;
    for (uint32_t face = 0; face < 6; ++face) {
      VkBufferImageCopy r{};
      r.bufferOffset = base + (VkDeviceSize)face * faceSize;
      r.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      r.imageSubresource.mipLevel = 0;
      r.imageSubresource.baseArrayLayer = lvl * 6 + face;
      r.imageSubresource.layerCount = 1;
      r.imageExtent = {width, height, 1};
      regions.push_back(r);
    }
  }

  vkCmdCopyBufferToImage(cmd, staging, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         (uint32_t)regions.size(), regions.data());

  TransitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  VK_IMAGE_ASPECT_COLOR_BIT, 1, 6 * levels);

  EndOneTimeCmd(device, renderer.grapicsQueue(), renderer.commandPool(), cmd);

  // View: CUBE_ARRAY
  VkImageView view = VK_NULL_HANDLE;
  VkImageViewCreateInfo vci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  vci.image = image;
  vci.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
  vci.format = format;
  vci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vci.subresourceRange.baseMipLevel = 0;
  vci.subresourceRange.levelCount = 1;
  vci.subresourceRange.baseArrayLayer = 0;
  vci.subresourceRange.layerCount = 6 * levels;

  if (vkCreateImageView(device, &vci, nullptr, &view) != VK_SUCCESS) {
    std::cerr << "Failed to vkCreateImageView for Cubemap with levels Texture"
              << std::endl;
    std::abort();
  }

  VkSampler sampler = VK_NULL_HANDLE;
  VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  sci.magFilter = VK_FILTER_LINEAR;
  sci.minFilter = VK_FILTER_LINEAR;
  sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  sci.minLod = 0.0f;
  sci.maxLod = 0.0f;

  if (vkCreateSampler(device, &sci, nullptr, &sampler) != VK_SUCCESS) {
    std::cerr << "Failed to vkCreateSampler for Cubemap with levels Texture"
              << std::endl;
    std::abort();
  }

  vkDestroyBuffer(device, staging, nullptr);
  vkFreeMemory(device, stagingMem, nullptr);

  Texture texture;

  texture.init(width, height, /*mipLevels=*/1, format, image, memory, view,
               sampler);

  return texture;
}

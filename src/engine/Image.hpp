#pragma once

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#include <nothings/stb_image.h>

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

static void TransitionImage2D(VkCommandBuffer command, VkImage image,
                              VkImageLayout oldLayout,
                              VkImageLayout newLayout) {
  VkImageMemoryBarrier imageMemoryBarrier{
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  imageMemoryBarrier.oldLayout = oldLayout;
  imageMemoryBarrier.newLayout = newLayout;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
  imageMemoryBarrier.subresourceRange.levelCount = 1;
  imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
  imageMemoryBarrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    std::cerr << "Unsupported layout transition" << std::endl;
    std::abort();
  }

  vkCmdPipelineBarrier(command, srcStage, dstStage, 0, 0, nullptr, 0, nullptr,
                       1, &imageMemoryBarrier);
}

static void CopyBufferToImage2D(VkCommandBuffer cmd, VkBuffer buffer,
                                VkImage image, uint32_t width,
                                uint32_t height) {
  VkBufferImageCopy bufferImageCopy{};
  bufferImageCopy.bufferOffset = 0;
  bufferImageCopy.bufferRowLength = 0;
  bufferImageCopy.bufferImageHeight = 0;

  bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferImageCopy.imageSubresource.mipLevel = 0;
  bufferImageCopy.imageSubresource.baseArrayLayer = 0;
  bufferImageCopy.imageSubresource.layerCount = 1;

  bufferImageCopy.imageOffset = {0, 0, 0};
  bufferImageCopy.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(cmd, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &bufferImageCopy);
}

static VkSampler CreateSampler2D(VkDevice device) {
  VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerCreateInfo.maxLod = 0.0f; // no mips
  samplerCreateInfo.anisotropyEnable = VK_FALSE;

  VkSampler sampler = VK_NULL_HANDLE;
  if (vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler) !=
      VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return sampler;
}

static bool ReadAllBytes(const char *path, std::vector<uint8_t> &out) {
  std::ifstream fileStream(path, std::ios::binary | std::ios::ate);
  if (!fileStream) {
    return false;
  }

  size_t size = (size_t)fileStream.tellg();
  fileStream.seekg(0);
  out.resize(size);
  fileStream.read((char *)out.data(), size);

  return true;
}

static bool DecodeToRGBA8(const std::vector<uint8_t> &bytes,
                          std::vector<uint8_t> &outPixels, int &outW,
                          int &outH) {
  int comp = 0;
  stbi_uc *px = stbi_load_from_memory(bytes.data(), (int)bytes.size(), &outW,
                                      &outH, &comp, 4);
  if (!px) {
    return false;
  }

  outPixels.assign(px, px + (size_t)outW * (size_t)outH * 4);
  stbi_image_free(px);

  return true;
}

static bool DecodeToRGBA32F(const std::vector<uint8_t> &bytes,
                            std::vector<float> &outRGBA, int &outW, int &outH) {
  int comp = 0;
  float *px = stbi_loadf_from_memory(bytes.data(), (int)bytes.size(), &outW,
                                     &outH, &comp, 4);
  if (!px) {
    return false;
  }

  outRGBA.assign(px, px + (size_t)outW * (size_t)outH * 4);
  stbi_image_free(px);

  return true;
}

static VkCommandBuffer BeginOneTimeCmd(VkDevice device, VkCommandPool pool) {
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  commandBufferAllocateInfo.commandPool = pool;
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferAllocateInfo.commandBufferCount = 1;

  VkCommandBuffer cmd = VK_NULL_HANDLE;
  vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &cmd);

  VkCommandBufferBeginInfo commandBufferBeginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &commandBufferBeginInfo);

  return cmd;
}

static void EndOneTimeCmd(VkDevice device, VkQueue queue, VkCommandPool pool,
                          VkCommandBuffer cmd) {
  vkEndCommandBuffer(cmd);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, pool, 1, &cmd);
}

static void TransitionImage(VkCommandBuffer cmd, VkImage image,
                            VkImageLayout oldLayout, VkImageLayout newLayout,
                            VkImageAspectFlags aspect, uint32_t mipLevels,
                            uint32_t layerCount) {
  VkImageMemoryBarrier imageMemoryBarrier{
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  imageMemoryBarrier.oldLayout = oldLayout;
  imageMemoryBarrier.newLayout = newLayout;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange.aspectMask = aspect;
  imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
  imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
  imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
  imageMemoryBarrier.subresourceRange.layerCount = layerCount;

  VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  imageMemoryBarrier.srcAccessMask = 0;
  imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  }

  vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1,
                       &imageMemoryBarrier);
}

static Texture CreateTextureFromRGBA8(Renderer &renderer,
                                      const uint8_t *rgbaPixels, uint32_t width,
                                      uint32_t height, VkFormat format) {
  VkPhysicalDevice physicalDevice = renderer.physicalDevice();
  VkDevice device = renderer.device();
  VkCommandPool commandPool = renderer.commandPool();
  VkQueue graphicsQueue = renderer.grapicsQueue();

  if (!rgbaPixels || width == 0 || height == 0) {
    std::cerr << "!rgbaPixels || width == 0 || height == 0" << std::endl;
    std::abort();
  }

  const VkDeviceSize uploadSize =
      (VkDeviceSize)width * (VkDeviceSize)height * 4;

  // 1) staging buffer
  VkBuffer stagingBuffer = VK_NULL_HANDLE;
  VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
  if (!CreateBuffer(physicalDevice, device, uploadSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer, stagingMemory)) {
    std::cerr << "!CreateBuffer" << std::endl;
    std::abort();
  }

  void *mapped = nullptr;
  vkMapMemory(device, stagingMemory, 0, uploadSize, 0, &mapped);
  memcpy(mapped, rgbaPixels, (size_t)uploadSize);
  vkUnmapMemory(device, stagingMemory);

  // 2) gpu image
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory imageMemory = VK_NULL_HANDLE;
  if (!CreateImage2D(
          physicalDevice, device, VK_SAMPLE_COUNT_1_BIT, width, height, format,
          VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, image,
          imageMemory)) {
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingMemory, nullptr);

    std::cerr << "!CreateImage2D" << std::endl;
    std::abort();
  }

  // 3) copy + transitions
  VkCommandBuffer cmd = BeginOneTimeCmd(device, renderer.commandPool());

  TransitionImage2D(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  CopyBufferToImage2D(cmd, stagingBuffer, image, width, height);

  TransitionImage2D(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  EndOneTimeCmd(device, renderer.grapicsQueue(), renderer.commandPool(), cmd);

  // 4) view + sampler
  VkImageView view =
      CreateImageView(device, image, format, VK_IMAGE_ASPECT_COLOR_BIT);
  VkSampler sampler = CreateSampler2D(device);

  // cleanup staging
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingMemory, nullptr);

  if (view == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
    if (sampler) {
      vkDestroySampler(device, sampler, nullptr);
    }

    if (view) {
      vkDestroyImageView(device, view, nullptr);
      vkDestroyImage(device, image, nullptr);

      vkFreeMemory(device, imageMemory, nullptr);
    }

    std::cerr << "!GetImageBytesFromCgltf(image, baseDirectory, fileBytes)"
              << std::endl;
    std::abort();
  }

  Texture texture;
  texture.init(width, height, 1, format, image, imageMemory, view, sampler);

  return texture;
}

static Texture CreateCubemapFromRGBA32F(Renderer &renderer,
                                        const float *faceRGBA[6],
                                        uint32_t width, uint32_t height,
                                        uint32_t mipLevels, VkFormat format) {
  VkPhysicalDevice physicalDevice = renderer.physicalDevice();
  VkDevice device = renderer.device();

  const uint32_t bytesPerPixel = 16; // 4 floats = 16 bytes
  VkDeviceSize faceSize =
      (VkDeviceSize)width * (VkDeviceSize)height * bytesPerPixel;
  VkDeviceSize totalSize = faceSize * 6;

  VkBuffer staging = VK_NULL_HANDLE;
  VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

  if (!CreateBuffer(physicalDevice, device, totalSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    staging, stagingMemory)) {
    std::cerr << "Failed to CreateBuffer for Cubemap Texture" << std::endl;
    std::abort();
  }

  void *mapped = nullptr;
  vkMapMemory(device, stagingMemory, 0, totalSize, 0, &mapped);
  uint8_t *dst = (uint8_t *)mapped;

  for (int face = 0; face < 6; ++face) {
    memcpy(dst + face * faceSize, faceRGBA[face], (size_t)faceSize);
  }

  vkUnmapMemory(device, stagingMemory);

  // ---- create cubemap image ----
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;

  VkImageCreateInfo imageCreateInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = format;
  imageCreateInfo.extent = {width, height, 1};
  imageCreateInfo.mipLevels = mipLevels;
  imageCreateInfo.arrayLayers = 6;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
    std::cerr << "(memType == UINT32_MAX) for Cubemap Texture" << std::endl;
    std::abort();
  }

  VkMemoryRequirements memoryRequirements{};
  vkGetImageMemoryRequirements(device, image, &memoryRequirements);

  uint32_t memoryType =
      FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memoryType == UINT32_MAX) {
    std::cerr << "(memType == UINT32_MAX) for Cubemap Texture" << std::endl;
    std::abort();
  }

  VkMemoryAllocateInfo memoryAllocateInfo{
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = memoryType;

  if (vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory) !=
      VK_SUCCESS) {
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
    VkBufferImageCopy bufferImageCopy{};
    bufferImageCopy.bufferOffset = (VkDeviceSize)face * faceSize;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.imageSubresource.baseArrayLayer = face;
    bufferImageCopy.imageSubresource.layerCount = 1;
    bufferImageCopy.imageExtent = {width, height, 1};

    regions.push_back(bufferImageCopy);
  }

  vkCmdCopyBufferToImage(cmd, staging, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         (uint32_t)regions.size(), regions.data());

  TransitionImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                  VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 6);

  EndOneTimeCmd(device, renderer.grapicsQueue(), renderer.commandPool(), cmd);

  // ---- create view (CUBE) ----
  VkImageView imageView = VK_NULL_HANDLE;

  VkImageViewCreateInfo imageViewCreateInfo{
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  imageViewCreateInfo.image = image;
  imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
  imageViewCreateInfo.format = format;
  imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
  imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  imageViewCreateInfo.subresourceRange.layerCount = 6;

  if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    std::cerr << "Failed to vkCreateImageView for Cubemap Texture" << std::endl;
    std::abort();
  }

  // ---- sampler ----
  VkSampler sampler = VK_NULL_HANDLE;
  VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.minLod = 0.0f;
  samplerCreateInfo.maxLod = (float)mipLevels;

  if (vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler) !=
      VK_SUCCESS) {
    std::cerr << "Failed to vkCreateSampler for Cubemap Texture" << std::endl;
    std::abort();
  }

  // cleanup staging
  vkDestroyBuffer(device, staging, nullptr);
  vkFreeMemory(device, stagingMemory, nullptr);

  Texture texture;
  texture.init(width, height, mipLevels, format, image, memory, imageView,
               sampler);

  return texture;
}

static Texture CreateCubemapArrayFromRGBA32F(Renderer &renderer,
                                             const float *pixels[/*level*/][6],
                                             uint32_t levels, uint32_t width,
                                             uint32_t height, VkFormat format) {
  VkPhysicalDevice physicalDevice = renderer.physicalDevice();
  VkDevice device = renderer.device();

  const uint32_t bytesPerPixel = 16; // RGBA32F
  VkDeviceSize faceSize =
      (VkDeviceSize)width * (VkDeviceSize)height * bytesPerPixel;
  VkDeviceSize levelSize = faceSize * 6;
  VkDeviceSize totalSize = levelSize * levels;

  VkBuffer staging = VK_NULL_HANDLE;
  VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

  if (!CreateBuffer(physicalDevice, device, totalSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    staging, stagingMemory)) {
    std::cerr << "Failed to CreateBuffer for Cubemap with levels Texture"
              << std::endl;
    std::abort();
  }

  void *mapped = nullptr;
  vkMapMemory(device, stagingMemory, 0, totalSize, 0, &mapped);
  uint8_t *dst = (uint8_t *)mapped;

  for (uint32_t lvl = 0; lvl < levels; ++lvl) {
    VkDeviceSize base = (VkDeviceSize)lvl * levelSize;
    for (uint32_t face = 0; face < 6; ++face) {
      VkDeviceSize off = base + (VkDeviceSize)face * faceSize;
      memcpy(dst + off, pixels[lvl][face], (size_t)faceSize);
    }
  }

  vkUnmapMemory(device, stagingMemory);

  // Create cube-array image
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;

  VkImageCreateInfo imageCreateInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format = format;
  imageCreateInfo.extent = {width, height, 1};
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 6 * levels;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
    std::cerr << "Failed to vkCreateImage for Cubemap with levels Texture"
              << std::endl;
    std::abort();
  }

  VkMemoryRequirements memoryRequirements{};
  vkGetImageMemoryRequirements(device, image, &memoryRequirements);

  uint32_t memoryType =
      FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memoryType == UINT32_MAX) {
    std::cerr
        << "Failed (memType == UINT32_MAX) for Cubemap with levels Texture"
        << std::endl;
    std::abort();
  }

  VkMemoryAllocateInfo memoryAllocateInfo{
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = memoryType;

  if (vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &memory) !=
      VK_SUCCESS) {
    std::cerr << "Failed to vkAllocateMemory for Cubemap with levels Texture"
              << std::endl;
    std::abort();
  }
  vkBindImageMemory(device, image, memory, 0);

  VkCommandBuffer cmd = BeginOneTimeCmd(device, renderer.commandPool());

  TransitionImage(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED,
                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                  VK_IMAGE_ASPECT_COLOR_BIT, 1, 6 * levels);

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
  VkImageView imageView = VK_NULL_HANDLE;
  VkImageViewCreateInfo imageViewCreateInfo{
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  imageViewCreateInfo.image = image;
  imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
  imageViewCreateInfo.format = format;
  imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  imageViewCreateInfo.subresourceRange.levelCount = 1;
  imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  imageViewCreateInfo.subresourceRange.layerCount = 6 * levels;

  if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    std::cerr << "Failed to vkCreateImageView for Cubemap with levels Texture"
              << std::endl;
    std::abort();
  }

  VkSampler sampler = VK_NULL_HANDLE;
  VkSamplerCreateInfo samplerCreateInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
  samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerCreateInfo.minLod = 0.0f;
  samplerCreateInfo.maxLod = 0.0f;

  if (vkCreateSampler(device, &samplerCreateInfo, nullptr, &sampler) !=
      VK_SUCCESS) {
    std::cerr << "Failed to vkCreateSampler for Cubemap with levels Texture"
              << std::endl;
    std::abort();
  }

  vkDestroyBuffer(device, staging, nullptr);
  vkFreeMemory(device, stagingMemory, nullptr);

  Texture texture;
  texture.init(width, height, 1, format, image, memory, imageView, sampler);

  return texture;
}

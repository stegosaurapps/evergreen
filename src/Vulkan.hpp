#pragma once

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "engine/Texture.hpp"

#include <SDL3/SDL.h>

#include <vulkan/vulkan.h>

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

static const char *kValidationLayer = "VK_LAYER_KHRONOS_validation";

static uint32_t FindMemoryType(VkPhysicalDevice physicalDevice,
                               uint32_t typeFilter,
                               VkMemoryPropertyFlags memoryPropertyFlags) {
  VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice,
                                      &physicalDeviceMemoryProperties);

  for (uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount;
       ++i) {
    if ((typeFilter & (1u << i)) &&
        (physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags &
         memoryPropertyFlags) == memoryPropertyFlags) {
      return i;
    }
  }
  return UINT32_MAX;
}

static bool CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice device,
                         VkDeviceSize deviceSize,
                         VkBufferUsageFlags bufferUsageFlags,
                         VkMemoryPropertyFlags memoryPropertyFlags,
                         VkBuffer &outputBuffer, VkDeviceMemory &outputMemory) {
  VkBufferCreateInfo bufferCreateInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferCreateInfo.size = deviceSize;
  bufferCreateInfo.usage = bufferUsageFlags;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &outputBuffer) !=
      VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements memoryRequirements{};
  vkGetBufferMemoryRequirements(device, outputBuffer, &memoryRequirements);

  uint32_t memType = FindMemoryType(
      physicalDevice, memoryRequirements.memoryTypeBits, memoryPropertyFlags);
  if (memType == UINT32_MAX) {
    return false;
  }

  VkMemoryAllocateInfo memoryAllocateInfo{
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = memType;
  if (vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &outputMemory) !=
      VK_SUCCESS) {
    return false;
  }

  vkBindBufferMemory(device, outputBuffer, outputMemory, 0);

  return true;
}

static bool CreateImage2D(VkPhysicalDevice physicalDevice, VkDevice device,
                          VkSampleCountFlagBits sampleCountFlagBits,
                          uint32_t width, uint32_t height, VkFormat format,
                          VkImageUsageFlags imageUsageFlags,
                          VkImage &outputImage, VkDeviceMemory &outputMemory) {
  VkImageCreateInfo imageCreateInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.extent = {width, height, 1};
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.format = format;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.usage = imageUsageFlags;
  imageCreateInfo.samples = sampleCountFlagBits;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &imageCreateInfo, nullptr, &outputImage) !=
      VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements memoryRequirement{};
  vkGetImageMemoryRequirements(device, outputImage, &memoryRequirement);
  uint32_t memType =
      FindMemoryType(physicalDevice, memoryRequirement.memoryTypeBits,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memType == UINT32_MAX) {
    return false;
  }

  VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  ai.allocationSize = memoryRequirement.size;
  ai.memoryTypeIndex = memType;
  if (vkAllocateMemory(device, &ai, nullptr, &outputMemory) != VK_SUCCESS) {
    return false;
  }

  vkBindImageMemory(device, outputImage, outputMemory, 0);

  return true;
}

static VkImageView CreateImageView(VkDevice device, VkImage image,
                                   VkFormat format,
                                   VkImageAspectFlags imageAspectFlags) {
  VkImageViewCreateInfo imageViewCreateInfo{
      VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  imageViewCreateInfo.image = image;
  imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCreateInfo.format = format;
  imageViewCreateInfo.subresourceRange.aspectMask = imageAspectFlags;
  imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  imageViewCreateInfo.subresourceRange.levelCount = 1;
  imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  imageViewCreateInfo.subresourceRange.layerCount = 1;

  VkImageView imageView = VK_NULL_HANDLE;
  if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return imageView;
}

static bool ReadFileBytes(const char *path, std::vector<char> &output) {
  std::ifstream fileStream(path, std::ios::ate | std::ios::binary);
  if (!fileStream.is_open()) {
    return false;
  }

  size_t size = (size_t)fileStream.tellg();
  output.resize(size);

  fileStream.seekg(0);
  fileStream.read(output.data(), size);
  fileStream.close();

  return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT debugUtilsMessageSeverityFlagsBits,
    VkDebugUtilsMessageTypeFlagsEXT debugUtilsMessageTypeFlags,
    const VkDebugUtilsMessengerCallbackDataEXT *data, void *userData) {
  (void)debugUtilsMessageTypeFlags;
  (void)userData;

  // Only print warnings/errors by default.
  if (debugUtilsMessageSeverityFlagsBits >=
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "[Vulkan] "
              << (data && data->pMessage ? data->pMessage : "(no message)")
              << std::endl;
  }

  return VK_FALSE;
}

static bool CheckInstanceLayerAvailable(const char *name) {
  uint32_t count = 0;
  vkEnumerateInstanceLayerProperties(&count, nullptr);

  std::vector<VkLayerProperties> layers(count);
  vkEnumerateInstanceLayerProperties(&count, layers.data());
  for (auto &layer : layers) {
    if (std::strcmp(layer.layerName, name) == 0)
      return true;
  }

  return false;
}

static bool CheckInstanceExtensionAvailable(const char *name) {
  uint32_t count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

  std::vector<VkExtensionProperties> extensionProperties(count);
  vkEnumerateInstanceExtensionProperties(nullptr, &count,
                                         extensionProperties.data());
  for (auto &extensionProperty : extensionProperties) {
    if (std::strcmp(extensionProperty.extensionName, name) == 0)
      return true;
  }

  return false;
}

static bool
CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice,
                            const std::vector<const char *> &requirements) {
  uint32_t count = 0;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count,
                                       nullptr);

  std::vector<VkExtensionProperties> extensionProperties(count);
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count,
                                       extensionProperties.data());

  std::set<std::string> missing;
  for (auto *requirement : requirements) {
    missing.insert(requirement);
  }

  for (auto &extensionProperty : extensionProperties) {
    missing.erase(extensionProperty.extensionName);
  }

  return missing.empty();
}

struct SwapchainSupport {
  VkSurfaceCapabilitiesKHR caps{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

static SwapchainSupport QuerySwapchainSupport(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface) {
  SwapchainSupport swapchainSupport{};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &swapchainSupport.caps);

  uint32_t fCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &fCount,
                                       nullptr);
  swapchainSupport.formats.resize(fCount);
  if (fCount) {
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &fCount,
                                         swapchainSupport.formats.data());
  }

  uint32_t pCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &pCount,
                                            nullptr);
  swapchainSupport.presentModes.resize(pCount);
  if (pCount) {
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice, surface, &pCount, swapchainSupport.presentModes.data());
  }

  return swapchainSupport;
}

static VkSurfaceFormatKHR
ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &surfaceFormats) {
  // Prefer SRGB if available.
  for (auto &surfaceFormat : surfaceFormats) {
    if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return surfaceFormat;
    }
  }

  // Else: first available.
  return surfaceFormats.empty()
             ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM,
                                  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
             : surfaceFormats[0];
}

static VkPresentModeKHR
ChoosePresentMode(const std::vector<VkPresentModeKHR> &presentModes) {
  // Mailbox; else FIFO (guaranteed).
  for (auto presentMode : presentModes) {
    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return presentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D
ChooseExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities, int w,
             int h) {
  if (surfaceCapabilities.currentExtent.width != UINT32_MAX) {
    return surfaceCapabilities.currentExtent;
  }

  VkExtent2D extent2D{};
  extent2D.width =
      (uint32_t)std::clamp(w, (int)surfaceCapabilities.minImageExtent.width,
                           (int)surfaceCapabilities.maxImageExtent.width);
  extent2D.height =
      (uint32_t)std::clamp(h, (int)surfaceCapabilities.minImageExtent.height,
                           (int)surfaceCapabilities.maxImageExtent.height);

  return extent2D;
}

static VkShaderModule CreateShaderModule(VkDevice device,
                                         const std::vector<char> &bytes) {
  VkShaderModuleCreateInfo shaderModuleCreateInfo{
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  shaderModuleCreateInfo.codeSize = bytes.size();
  shaderModuleCreateInfo.pCode =
      reinterpret_cast<const uint32_t *>(bytes.data());

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  if (vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr,
                           &shaderModule) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return shaderModule;
}

static VkCommandBuffer BeginOneShot(VkDevice device,
                                    VkCommandPool commandPool) {
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  commandBufferAllocateInfo.commandPool = commandPool;
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferAllocateInfo.commandBufferCount = 1;

  VkCommandBuffer command = VK_NULL_HANDLE;
  vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &command);

  VkCommandBufferBeginInfo commandBufferBeginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(command, &commandBufferBeginInfo);

  return command;
}

static void EndOneShot(VkDevice device, VkCommandPool commandPool,
                       VkQueue queue, VkCommandBuffer command) {
  vkEndCommandBuffer(command);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &command;
  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, commandPool, 1, &command);
}

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

static bool
CreateTextureFromRGBA8(VkPhysicalDevice physicalDevice, VkDevice device,
                       VkCommandPool commandPool, VkQueue graphicsQueue,
                       const uint8_t *rgbaPixels, uint32_t width,
                       uint32_t height,
                       VkFormat format, // VK_FORMAT_R8G8B8A8_UNORM or _SRGB
                       Texture &outTex) {
  if (!rgbaPixels || width == 0 || height == 0)
    return false;

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
    return false;
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

    return false;
  }

  // 3) copy + transitions
  VkCommandBuffer cmd = BeginOneShot(device, commandPool);

  TransitionImage2D(cmd, image, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  CopyBufferToImage2D(cmd, stagingBuffer, image, width, height);

  TransitionImage2D(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  EndOneShot(device, commandPool, graphicsQueue, cmd);

  // 4) view + sampler
  VkImageView view =
      CreateImageView(device, image, format, VK_IMAGE_ASPECT_COLOR_BIT);
  VkSampler sampler = CreateSampler2D(device);

  // cleanup staging
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingMemory, nullptr);

  if (view == VK_NULL_HANDLE || sampler == VK_NULL_HANDLE) {
    if (sampler)
      vkDestroySampler(device, sampler, nullptr);
    if (view)
      vkDestroyImageView(device, view, nullptr);
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, imageMemory, nullptr);

    return false;
  }

  outTex.init(width, height, 1, format, image, imageMemory, view, sampler);

  return true;
}

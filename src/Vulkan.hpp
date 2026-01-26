#pragma once

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

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

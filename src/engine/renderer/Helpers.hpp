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

static uint32_t FindMemoryType(VkPhysicalDevice pd, uint32_t typeFilter,
                               VkMemoryPropertyFlags props) {
  VkPhysicalDeviceMemoryProperties mp{};
  vkGetPhysicalDeviceMemoryProperties(pd, &mp);

  for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
    if ((typeFilter & (1u << i)) &&
        (mp.memoryTypes[i].propertyFlags & props) == props) {
      return i;
    }
  }
  return UINT32_MAX;
}

static bool CreateBuffer(VkPhysicalDevice pd, VkDevice dev, VkDeviceSize size,
                         VkBufferUsageFlags usage,
                         VkMemoryPropertyFlags memProps, VkBuffer &outBuf,
                         VkDeviceMemory &outMem) {
  VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  ci.size = size;
  ci.usage = usage;
  ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(dev, &ci, nullptr, &outBuf) != VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements req{};
  vkGetBufferMemoryRequirements(dev, outBuf, &req);

  uint32_t memType = FindMemoryType(pd, req.memoryTypeBits, memProps);
  if (memType == UINT32_MAX) {
    return false;
  }

  VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  ai.allocationSize = req.size;
  ai.memoryTypeIndex = memType;
  if (vkAllocateMemory(dev, &ai, nullptr, &outMem) != VK_SUCCESS) {
    return false;
  }

  vkBindBufferMemory(dev, outBuf, outMem, 0);
  return true;
}

static bool CreateImage2D(VkPhysicalDevice pd, VkDevice dev,
                          VkSampleCountFlagBits sampleCount, uint32_t width,
                          uint32_t height, VkFormat fmt,
                          VkImageUsageFlags usage, VkImage &outImg,
                          VkDeviceMemory &outMem) {
  VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  ci.imageType = VK_IMAGE_TYPE_2D;
  ci.extent = {width, height, 1};
  ci.mipLevels = 1;
  ci.arrayLayers = 1;
  ci.format = fmt;
  ci.tiling = VK_IMAGE_TILING_OPTIMAL;
  ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ci.usage = usage;
  ci.samples = sampleCount;
  ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(dev, &ci, nullptr, &outImg) != VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(dev, outImg, &req);
  uint32_t memType = FindMemoryType(pd, req.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memType == UINT32_MAX) {
    return false;
  }

  VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  ai.allocationSize = req.size;
  ai.memoryTypeIndex = memType;
  if (vkAllocateMemory(dev, &ai, nullptr, &outMem) != VK_SUCCESS) {
    return false;
  }

  vkBindImageMemory(dev, outImg, outMem, 0);
  return true;
}

static VkImageView CreateImageView(VkDevice dev, VkImage img, VkFormat fmt,
                                   VkImageAspectFlags aspect) {
  VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  iv.image = img;
  iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
  iv.format = fmt;
  iv.subresourceRange.aspectMask = aspect;
  iv.subresourceRange.baseMipLevel = 0;
  iv.subresourceRange.levelCount = 1;
  iv.subresourceRange.baseArrayLayer = 0;
  iv.subresourceRange.layerCount = 1;

  VkImageView out = VK_NULL_HANDLE;
  if (vkCreateImageView(dev, &iv, nullptr, &out) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  return out;
}

static bool ReadFileBytes(const char *path, std::vector<char> &out) {
  std::ifstream f(path, std::ios::ate | std::ios::binary);
  if (!f.is_open()) {
    return false;
  }
  size_t size = (size_t)f.tellg();
  out.resize(size);
  f.seekg(0);
  f.read(out.data(), size);
  f.close();
  return true;
}

static const char *kValidationLayer = "VK_LAYER_KHRONOS_validation";

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *data, void *userData) {
  (void)type;
  (void)userData;

  // Keep it readable: only print warnings/errors by default.
  if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
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
  for (auto &l : layers) {
    if (std::strcmp(l.layerName, name) == 0)
      return true;
  }

  return false;
}

static bool CheckInstanceExtensionAvailable(const char *name) {
  uint32_t count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

  std::vector<VkExtensionProperties> exts(count);
  vkEnumerateInstanceExtensionProperties(nullptr, &count, exts.data());
  for (auto &e : exts) {
    if (std::strcmp(e.extensionName, name) == 0)
      return true;
  }

  return false;
}

static bool
CheckDeviceExtensionSupport(VkPhysicalDevice pd,
                            const std::vector<const char *> &required) {
  uint32_t count = 0;
  vkEnumerateDeviceExtensionProperties(pd, nullptr, &count, nullptr);

  std::vector<VkExtensionProperties> exts(count);
  vkEnumerateDeviceExtensionProperties(pd, nullptr, &count, exts.data());

  std::set<std::string> missing;
  for (auto *r : required) {
    missing.insert(r);
  }

  for (auto &e : exts) {
    missing.erase(e.extensionName);
  }

  return missing.empty();
}

struct SwapchainSupport {
  VkSurfaceCapabilitiesKHR caps{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

static SwapchainSupport QuerySwapchainSupport(VkPhysicalDevice pd,
                                              VkSurfaceKHR surface) {
  SwapchainSupport s{};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &s.caps);

  uint32_t fCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &fCount, nullptr);
  s.formats.resize(fCount);
  if (fCount) {
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &fCount,
                                         s.formats.data());
  }

  uint32_t pCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &pCount, nullptr);
  s.presentModes.resize(pCount);
  if (pCount) {
    vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &pCount,
                                              s.presentModes.data());
  }

  return s;
}

static VkSurfaceFormatKHR
ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
  // Prefer SRGB if available.
  for (auto &f : formats) {
    if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
        f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return f;
    }
  }

  // Else: first available.
  return formats.empty() ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM,
                                              VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                         : formats[0];
}

static VkPresentModeKHR
ChoosePresentMode(const std::vector<VkPresentModeKHR> &modes) {
  // Mailbox feels great if available; else FIFO (guaranteed).
  for (auto m : modes) {
    if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
      return m;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR &caps, int w,
                               int h) {
  if (caps.currentExtent.width != UINT32_MAX) {
    return caps.currentExtent;
  }

  VkExtent2D e{};
  e.width = (uint32_t)std::clamp(w, (int)caps.minImageExtent.width,
                                 (int)caps.maxImageExtent.width);
  e.height = (uint32_t)std::clamp(h, (int)caps.minImageExtent.height,
                                  (int)caps.maxImageExtent.height);

  return e;
}

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

static VkShaderModule CreateShaderModule(VkDevice dev,
                                         const std::vector<char> &bytes) {
  VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  ci.codeSize = bytes.size();
  ci.pCode = reinterpret_cast<const uint32_t *>(bytes.data());
  VkShaderModule mod = VK_NULL_HANDLE;
  if (vkCreateShaderModule(dev, &ci, nullptr, &mod) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  return mod;
}

#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class Mesh {
public:
  Mesh() = default;
  ~Mesh() = default;

  void init(uint32_t indexCount, VkBuffer vertexBuffer,
            VkDeviceMemory vertexMemory, VkBuffer indexBuffer,
            VkDeviceMemory indexMemory);

  uint32_t indexCount();
  VkBuffer vertexBuffer();
  VkDeviceMemory vertexMemory();
  VkBuffer indexBuffer();
  VkDeviceMemory indexMemory();

  void clear();

private:
  uint32_t m_indexCount = 0;
  VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
  VkBuffer m_indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;
};

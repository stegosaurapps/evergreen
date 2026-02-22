#pragma once

#include "Material.hpp"

#include <vulkan/vulkan.h>

#include <vector>

class Renderer; // forward declaration

class Mesh {
public:
  Mesh() = default;
  ~Mesh() = default;

  void init(uint32_t indexCount, VkBuffer vertexBuffer,
            VkDeviceMemory vertexMemory, VkBuffer indexBuffer,
            VkDeviceMemory indexMemory, Material *material);

  uint32_t indexCount();
  VkBuffer vertexBuffer();
  VkDeviceMemory vertexMemory();
  VkBuffer indexBuffer();
  VkDeviceMemory indexMemory();

  Material *material();

  void clear(Renderer &renderer, VkDescriptorPool descriptorPool);

private:
  uint32_t m_indexCount = 0;
  VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
  VkBuffer m_indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;

  Material *m_material = nullptr;
};

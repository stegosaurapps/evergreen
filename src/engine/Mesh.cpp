#include "Mesh.hpp"
#include "Renderer.hpp"

#include <iostream>

void Mesh::init(uint32_t indexCount, VkBuffer vertexBuffer,
                VkDeviceMemory vertexMemory, VkBuffer indexBuffer,
                VkDeviceMemory indexMemory, Material *material) {
  m_indexCount = indexCount;

  m_vertexBuffer = vertexBuffer;
  m_vertexMemory = vertexMemory;
  m_indexBuffer = indexBuffer;
  m_indexMemory = indexMemory;

  m_material = material;
}

uint32_t Mesh::indexCount() { return m_indexCount; }

VkBuffer Mesh::vertexBuffer() { return m_vertexBuffer; }

VkDeviceMemory Mesh::vertexMemory() { return m_vertexMemory; }

VkBuffer Mesh::indexBuffer() { return m_indexBuffer; }

VkDeviceMemory Mesh::indexMemory() { return m_indexMemory; }

Material *Mesh::material() { return m_material; }

void Mesh::clear(Renderer &renderer, VkDescriptorPool descriptorPool) {
  auto device = renderer.device();

  if (!device) {
    return;
  }

  if (m_material != nullptr) {
    m_material->clear(renderer, descriptorPool);

    free(m_material);
  }

  if (m_vertexBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, m_vertexBuffer, nullptr);
    m_vertexBuffer = VK_NULL_HANDLE;
  }

  if (m_vertexMemory != VK_NULL_HANDLE) {
    vkFreeMemory(device, m_vertexMemory, nullptr);
    m_vertexMemory = VK_NULL_HANDLE;
  }

  if (m_indexBuffer != VK_NULL_HANDLE) {
    vkDestroyBuffer(device, m_indexBuffer, nullptr);
    m_indexBuffer = VK_NULL_HANDLE;
  }
  if (m_indexMemory != VK_NULL_HANDLE) {
    vkFreeMemory(device, m_indexMemory, nullptr);
    m_indexMemory = VK_NULL_HANDLE;
  }
}

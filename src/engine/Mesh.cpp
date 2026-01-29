#include "Mesh.hpp"

#include <iostream>

void Mesh::init(uint32_t indexCount, VkBuffer vertexBuffer,
                VkDeviceMemory vertexMemory, VkBuffer indexBuffer,
                VkDeviceMemory indexMemory) {
  m_indexCount = indexCount;
  m_vertexBuffer = vertexBuffer;
  m_vertexMemory = vertexMemory;
  m_indexBuffer = indexBuffer;
  m_indexMemory = indexMemory;
}

uint32_t Mesh::indexCount() { return m_indexCount; }

VkBuffer Mesh::vertexBuffer() { return m_vertexBuffer; }

VkDeviceMemory Mesh::vertexMemory() { return m_vertexMemory; }

VkBuffer Mesh::indexBuffer() { return m_indexBuffer; }

VkDeviceMemory Mesh::indexMemory() { return m_indexMemory; }

void Mesh::clear() {}

// void Mesh::destroyResources(Renderer &renderer) {
//   auto device = renderer.device();

//   if (!device) {
//     return;
//   }

//   // Mesh buffers
//   if (m_vertexBuffer) {
//     vkDestroyBuffer(device, m_vertexBuffer, nullptr);
//     m_vertexBuffer = VK_NULL_HANDLE;
//   }
//   if (m_vertexMemory) {
//     vkFreeMemory(device, m_vertexMemory, nullptr);
//     m_vertexMemory = VK_NULL_HANDLE;
//   }

//   if (m_indexBuffer) {
//     vkDestroyBuffer(device, m_indexBuffer, nullptr);
//     m_indexBuffer = VK_NULL_HANDLE;
//   }
//   if (m_indexMemory) {
//     vkFreeMemory(device, m_indexMemory, nullptr);
//     m_indexMemory = VK_NULL_HANDLE;
//   }

//   // Uniform buffers (per-frame)
//   for (int i = 0; i < FRAME_COUNT; ++i) {
//     if (m_uboMappedList[i]) {
//       vkUnmapMemory(device, m_uboMemoryList[i]);
//       m_uboMappedList[i] = nullptr;
//     }

//     if (m_uboBufferList[i]) {
//       vkDestroyBuffer(device, m_uboBufferList[i], nullptr);
//       m_uboBufferList[i] = VK_NULL_HANDLE;
//     }

//     if (m_uboMemoryList[i]) {
//       vkFreeMemory(device, m_uboMemoryList[i], nullptr);
//       m_uboMemoryList[i] = VK_NULL_HANDLE;
//     }
//   }

//   // Descriptor pool
//   if (m_descriptorPool) {
//     vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
//     m_descriptorPool = VK_NULL_HANDLE;
//   }

//   // Descriptor set layout
//   if (m_descriptorSetLayout) {
//     vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
//     m_descriptorSetLayout = VK_NULL_HANDLE;
//   }
// }

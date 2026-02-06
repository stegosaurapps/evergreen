#include "../Vulkan.hpp"

#include "Builder.hpp"

#include <iostream>

Builder::Builder(VertexDescriptor vertexDescriptor)
    : m_vertexCollector(VertexCollector(vertexDescriptor)) {}

unsigned long long Builder::vertexStride() {
  return m_vertexCollector.vertexStride();
}

void Builder::insertVertex(Vertex vertex) {
  m_vertexCollector.insertVertex(vertex);
}

void Builder::addVertices(std::vector<Vertex> vertices) {
  m_vertexCollector.addVertices(vertices);
}

void Builder::addIndices(std::vector<uint32_t> indices) {
  m_vertexCollector.addIndices(indices);
}

void Builder::generateMesh(Renderer &renderer) {
  auto physicalDevice = renderer.physicalDevice();
  auto device = renderer.device();

  size_t vertexCount = m_vertexCollector.vertexCount();
  size_t indexCount = m_vertexCollector.indexCount();

  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexMemory = VK_NULL_HANDLE;

  const VkDeviceSize vertexBufferSize =
      m_vertexCollector.vertexStride() * vertexCount;
  const VkDeviceSize indexBufferSize = sizeof(uint32_t) * indexCount;

  if (!CreateBuffer(physicalDevice, device, vertexBufferSize,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    vertexBuffer, vertexMemory)) {
    std::cerr << "Failed to create vertex buffer" << std::endl;
    std::abort();
  }

  if (!CreateBuffer(physicalDevice, device, indexBufferSize,
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    indexBuffer, indexMemory)) {
    std::cerr << "Failed to create index buffer" << std::endl;
    std::abort();
  }

  void *data = nullptr;
  vkMapMemory(device, vertexMemory, 0, vertexBufferSize, 0, &data);
  std::memcpy(data, m_vertexCollector.rawVertexData().data(),
              (size_t)vertexBufferSize);
  vkUnmapMemory(device, vertexMemory);

  vkMapMemory(device, indexMemory, 0, indexBufferSize, 0, &data);
  std::memcpy(data, m_vertexCollector.rawIndexData().data(),
              (size_t)indexBufferSize);
  vkUnmapMemory(device, indexMemory);

  std::cout << "vertices=" << vertexCount << " indices=" << indexCount
            << std::endl;

  // clear vertex collector
  m_vertexCollector.clearVertices();
  m_vertexCollector.clearIndices();

  Mesh mesh;
  mesh.init(indexCount, vertexBuffer, vertexMemory, indexBuffer, indexMemory);

  m_meshes.push_back(mesh);
}

Model Builder::buildModel(Renderer &renderer) {
  Model model;

  model.init(m_meshes);

  m_meshes = {};

  return model;
}

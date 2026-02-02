#include "../Vulkan.hpp"

#include "Renderer.hpp"
#include "Vertex.hpp"

#include <iostream>

VertexCollector::VertexCollector(std::vector<VertexAttribute> vertexAttributes)
    : m_vertexAttributes(vertexAttributes) {}

VkPipelineVertexInputStateCreateInfo
VertexCollector::pipelineVertexInputStateCreateInfo() {
  // Vertex input
  VkVertexInputBindingDescription vertexInputBindingDescription{};
  vertexInputBindingDescription.binding = 0;
  vertexInputBindingDescription.stride = vertexStride();
  vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::vector<VkVertexInputAttributeDescription>
      vertexInputAttributeDescriptions;

  uint32_t offset = 0;
  for (int i = 0; i < m_vertexAttributes.size(); i++) {
    auto vertexAttribute = m_vertexAttributes[i];

    uint32_t attributeSize = sizeof(float) * AttributeCount(vertexAttribute);

    VkVertexInputAttributeDescription vertexInputAttributeDescription;
    vertexInputAttributeDescription.location = i;
    vertexInputAttributeDescription.binding = 0;
    vertexInputAttributeDescription.format = VulkanFormat(vertexAttribute);
    vertexInputAttributeDescription.offset = offset;

    vertexInputAttributeDescriptions.push_back(vertexInputAttributeDescription);

    offset += attributeSize;
  }

  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
  pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions =
      &vertexInputBindingDescription;
  pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount =
      vertexInputAttributeDescriptions.size();
  pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions =
      vertexInputAttributeDescriptions.data();

  return pipelineVertexInputStateCreateInfo;
}

unsigned long long VertexCollector::vertexStride() {
  auto floatSize = sizeof(float);

  unsigned long long sizeAccumulator = 0;

  for (VertexAttribute vertexAttribute : m_vertexAttributes) {
    unsigned long long attributeCount = AttributeCount(vertexAttribute);
    unsigned long long attributeSize = attributeCount * floatSize;

    sizeAccumulator += attributeSize;
  }

  return sizeAccumulator;
}

std::vector<VertexAttribute> VertexCollector::vertexAttributes() {
  return m_vertexAttributes;
}

void VertexCollector::insertVertex(Vertex vertex) {
  m_vertices.push_back(vertex);
}

void VertexCollector::addVertices(std::vector<Vertex> vertices) {
  m_vertices = vertices;
}

void VertexCollector::addIndices(std::vector<uint32_t> indices) {
  m_indices = indices;
}

std::vector<float> VertexCollector::rawVertexData() {
  std::vector<float> data;

  for (Vertex vertex : m_vertices) {
    for (VertexAttribute vertexAttribute : m_vertexAttributes) {
      std::vector<float> attributeData =
          VertexAttributeData(vertex, vertexAttribute);

      for (float element : attributeData) {
        data.push_back(element);
      }
    }
  }

  return data;
}

Model VertexCollector::buildModel(Renderer &renderer) {
  auto physicalDevice = renderer.physicalDevice();
  auto device = renderer.device();

  uint32_t indexCount = 0;
  VkBuffer vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
  VkBuffer indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory indexMemory = VK_NULL_HANDLE;

  indexCount = (uint32_t)m_indices.size();

  const VkDeviceSize vertexBufferSize = vertexStride() * m_vertices.size();
  const VkDeviceSize indexBufferSize = sizeof(uint32_t) * m_indices.size();

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
  std::memcpy(data, rawVertexData().data(), (size_t)vertexBufferSize);
  vkUnmapMemory(device, vertexMemory);

  vkMapMemory(device, indexMemory, 0, indexBufferSize, 0, &data);
  std::memcpy(data, m_indices.data(), (size_t)indexBufferSize);
  vkUnmapMemory(device, indexMemory);

  std::cout << "vertices=" << m_vertices.size()
            << " indices=" << m_indices.size() << std::endl;

  Mesh mesh;
  mesh.init(indexCount, vertexBuffer, vertexMemory, indexBuffer, indexMemory);

  std::vector<Mesh> meshes = {mesh};

  Model model;
  model.init(meshes);

  return model;
}

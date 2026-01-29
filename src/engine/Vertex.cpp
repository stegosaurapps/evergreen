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

  int attributeCount = m_vertexAttributes.size();

  VkVertexInputAttributeDescription vertexInputAttributeDescription[3]{};

  // position
  vertexInputAttributeDescription[0].location = 0;
  vertexInputAttributeDescription[0].binding = 0;
  vertexInputAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescription[0].offset = offsetof(VertexColor, px);

  // normal
  vertexInputAttributeDescription[1].location = 1;
  vertexInputAttributeDescription[1].binding = 0;
  vertexInputAttributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescription[1].offset = offsetof(VertexColor, nx);

  // color
  vertexInputAttributeDescription[2].location = 2;
  vertexInputAttributeDescription[2].binding = 0;
  vertexInputAttributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescription[2].offset = offsetof(VertexColor, r);

  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
  pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions =
      &vertexInputBindingDescription;
  pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 3;
  pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions =
      vertexInputAttributeDescription;

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

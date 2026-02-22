#include "../Vulkan.hpp"

#include "Renderer.hpp"
#include "Vertex.hpp"

#include <iostream>

void VertexDescriptor::init(std::vector<VertexAttribute> vertexAttributes) {
  m_vertexAttributes = vertexAttributes;
}

unsigned long long VertexDescriptor::vertexStride() {
  auto floatSize = sizeof(float);

  unsigned long long sizeAccumulator = 0;

  for (VertexAttribute vertexAttribute : m_vertexAttributes) {
    unsigned long long attributeCount = AttributeCount(vertexAttribute);
    unsigned long long attributeSize = attributeCount * floatSize;

    sizeAccumulator += attributeSize;
  }

  return sizeAccumulator;
}

std::vector<VertexAttribute> VertexDescriptor::vertexAttributes() {
  return m_vertexAttributes;
}

VertexCollector::VertexCollector(VertexDescriptor vertexDescriptor)
    : m_vertexDescriptor(vertexDescriptor) {}

unsigned long long VertexCollector::vertexStride() {
  return m_vertexDescriptor.vertexStride();
}

std::vector<VertexAttribute> VertexCollector::vertexAttributes() {
  return m_vertexDescriptor.vertexAttributes();
}

void VertexCollector::insertVertex(Vertex vertex) {
  m_vertices.push_back(vertex);
}

void VertexCollector::addVertices(std::vector<Vertex> vertices) {
  m_vertices = vertices;
}

size_t VertexCollector::vertexCount() { return m_vertices.size(); }

std::vector<float> VertexCollector::rawVertexData() {
  std::vector<float> data;

  for (Vertex vertex : m_vertices) {
    for (VertexAttribute vertexAttribute :
         m_vertexDescriptor.vertexAttributes()) {
      std::vector<float> attributeData =
          VertexAttributeData(vertex, vertexAttribute);

      for (float element : attributeData) {
        data.push_back(element);
      }
    }
  }

  return data;
}

void VertexCollector::clearVertices() { m_vertices = {}; }

void VertexCollector::addIndices(std::vector<uint32_t> indices) {
  m_indices = indices;
}

size_t VertexCollector::indexCount() { return m_indices.size(); }

std::vector<uint32_t> VertexCollector::rawIndexData() {
  std::vector<uint32_t> data;

  for (uint32_t index : m_indices) {
    data.push_back(index);
  }

  return data;
}

void VertexCollector::clearIndices() { m_indices = {}; }

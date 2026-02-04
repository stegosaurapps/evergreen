#pragma once

#include "../Vulkan.hpp"

#include "Model.hpp"

#include <stddef.h>
#include <string>
#include <vector>

class Renderer; // forward declaration

enum VertexAttribute {
  Position,
  Normal,
  Tangent,
  TextureCoordinate,
  Color,
};

inline int AttributeCount(VertexAttribute vertexAttribute) {
  switch (vertexAttribute) {
  case VertexAttribute::Position:
    return 3;
  case VertexAttribute::Normal:
    return 3;
  case VertexAttribute::Tangent:
    return 4;
  case VertexAttribute::TextureCoordinate:
    return 2;
  case VertexAttribute::Color:
    return 3;
  }
}

inline std::string AttributeName(VertexAttribute vertexAttribute) {
  switch (vertexAttribute) {
  case VertexAttribute::Position:
    return "Position";
  case VertexAttribute::Normal:
    return "Normal";
  case VertexAttribute::Tangent:
    return "Tangent";
  case VertexAttribute::TextureCoordinate:
    return "TextureCoordinate";
  case VertexAttribute::Color:
    return "Color";
  }
}

inline VkFormat VulkanFormat(VertexAttribute vertexAttribute) {
  switch (vertexAttribute) {
  case VertexAttribute::Position:
    return VK_FORMAT_R32G32B32_SFLOAT;
  case VertexAttribute::Normal:
    return VK_FORMAT_R32G32B32_SFLOAT;
  case VertexAttribute::Tangent:
    return VK_FORMAT_R32G32B32A32_SFLOAT;
  case VertexAttribute::TextureCoordinate:
    return VK_FORMAT_R32G32_SFLOAT;
  case VertexAttribute::Color:
    return VK_FORMAT_R32G32B32_SFLOAT;
  }
}

struct Vertex {
  float px, py, pz;     // Position
  float nx, ny, nz;     // Normal
  float tx, ty, tz, tw; // Tangent
  float ux, uy;         // Texture Coordinate
  float r, g, b;        // Color
};

inline void PrintVertex(Vertex *vertex) {
  std::cout << "------ VERTEX ------" << std::endl;
  std::cout << "  Position: (" << vertex->px << ", " << vertex->py << ", "
            << vertex->pz << ")" << std::endl;
  std::cout << "  Normal: (" << vertex->nx << ", " << vertex->ny << ", "
            << vertex->nz << ")" << std::endl;
  // std::cout << "  Tangent: (" << vertex->px << ", " << vertex->py << ", " <<
  // vertex->pz << ")" << std::endl; std::cout << "  Texture Coordinate: (" <<
  // vertex->px << ", " << vertex->py << ", " << vertex->pz << ")" << std::endl;
  std::cout << "  Color: (" << vertex->r << ", " << vertex->g << ", "
            << vertex->b << ")" << std::endl;
  std::cout << "--------------------" << std::endl;
}

inline std::vector<float> VertexAttributeData(Vertex vertex,
                                              VertexAttribute vertexAttribute) {
  switch (vertexAttribute) {
  case VertexAttribute::Position:
    return {vertex.px, vertex.py, vertex.pz};
  case VertexAttribute::Normal:
    return {vertex.nx, vertex.ny, vertex.nz};
  case VertexAttribute::Tangent:
    return {vertex.px, vertex.py, vertex.pz};
  case VertexAttribute::TextureCoordinate:
    return {vertex.tx, vertex.ty, vertex.tz, vertex.tw};
  case VertexAttribute::Color:
    return {vertex.r, vertex.g, vertex.b};
  }
}

class VertexDescriptor {
public:
  VertexDescriptor() = default;
  ~VertexDescriptor() = default;

  void init(std::vector<VertexAttribute> vertexAttributes);

  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo();

  unsigned long long vertexStride();

  std::vector<VertexAttribute> vertexAttributes();

  void print();

private:
  std::vector<VertexAttribute> m_vertexAttributes;
};

class VertexCollector {
public:
  VertexCollector(VertexDescriptor vertexDescriptor);
  ~VertexCollector() = default;

  unsigned long long vertexStride();

  std::vector<VertexAttribute> vertexAttributes();

  void insertVertex(Vertex vertex);
  void addVertices(std::vector<Vertex> vertices);
  size_t vertexCount();
  std::vector<float> rawVertexData();
  void clearVertices();

  void addIndices(std::vector<uint32_t> indices);
  size_t indexCount();
  std::vector<uint32_t> rawIndexData();
  void clearIndices();

private:
  VertexDescriptor m_vertexDescriptor;

  std::vector<Vertex> m_vertices;
  std::vector<uint32_t> m_indices;
};

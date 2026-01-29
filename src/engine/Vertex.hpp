#pragma once

#include "../Vulkan.hpp"

#include "Model.hpp"

#include <stddef.h>
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

struct Vertex {
  float px, py, pz;     // Position
  float nx, ny, nz;     // Normal
  float tx, ty, tz, tw; // Tangent
  float ux, uy;         // Texture Coordinate
  float r, g, b;        // Color
};

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

class VertexCollector {
public:
  VertexCollector(std::vector<VertexAttribute> vertexAttributes);
  ~VertexCollector() = default;

  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo();

  unsigned long long vertexStride();

  void insertVertex(Vertex vertex);
  void addVertices(std::vector<Vertex> vertices);
  void addIndices(std::vector<uint32_t> indices);

  std::vector<float> rawVertexData();

  Model buildModel(Renderer &renderer);

private:
  std::vector<VertexAttribute> m_vertexAttributes;

  std::vector<Vertex> m_vertices;
  std::vector<uint32_t> m_indices;
};

struct VertexColor {
  float px, py, pz;
  float nx, ny, nz;
  float r, g, b;
};

static_assert(sizeof(VertexColor) == 36, "VertexColor must be 36 bytes");
static_assert(offsetof(VertexColor, px) == 0, "pos offset");
static_assert(offsetof(VertexColor, nx) == 12, "nrm offset");
static_assert(offsetof(VertexColor, r) == 24, "col offset");

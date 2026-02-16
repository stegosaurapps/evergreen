#pragma once

#include "Mesh.hpp"
#include "Model.hpp"
#include "Renderer.hpp"
#include "Vertex.hpp"

class Material; // forward declaration

class Builder {
public:
  Builder(VertexDescriptor vertexDescriptor);
  ~Builder() = default;

  unsigned long long vertexStride();

  void insertVertex(Vertex vertex);
  void addVertices(std::vector<Vertex> vertices);
  void addIndices(std::vector<uint32_t> indices);

  void addMaterial(Material *material);

  void generateMesh(Renderer &renderer);

  Model buildModel(Renderer &renderer);

private:
  VertexCollector m_vertexCollector;

  std::vector<Mesh> m_meshes;

  Material *m_material;
};

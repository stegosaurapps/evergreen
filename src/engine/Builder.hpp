#pragma once

#include "Mesh.hpp"
#include "Model.hpp"
#include "Renderer.hpp"
#include "Vertex.hpp"

class Builder {
public:
  Builder(VertexDescriptor vertexDescriptor);
  ~Builder() = default;

  void insertVertex(Vertex vertex);
  void addVertices(std::vector<Vertex> vertices);
  void addIndices(std::vector<uint32_t> indices);

  void generateMesh(Renderer &renderer);

  Model buildModel(Renderer &renderer);

private:
  VertexCollector m_vertexCollector;

  std::vector<Mesh> m_meshes;

  // Texture stuff will go here...
};
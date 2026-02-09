#pragma once

#include "Mesh.hpp"

#include <vector>

class Renderer; // forward declaration

class Model {
public:
  Model() = default;
  ~Model() = default;

  void init(std::vector<Mesh> meshes);

  std::vector<Mesh> &meshes();

  void clear(Renderer &renderer);

private:
  std::vector<Mesh> m_meshes;
};

#pragma once

#include "Mesh.hpp"

#include <vector>

class Model {
public:
  Model() = default;
  ~Model() = default;

  void init(std::vector<Mesh> meshes);

private:
  std::vector<Mesh> m_meshes;
  // This is where the textures go
};

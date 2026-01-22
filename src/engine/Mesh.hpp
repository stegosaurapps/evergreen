#pragma once

#include "Vertex.hpp"

#include <vector>

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

#pragma once

#include "Mesh.hpp"
#include "Transform.hpp"

struct Renderable {
  Transform transform;
  Mesh mesh;
};

#pragma once

#include "Math.hpp"

#include <cstdint>
#include <vector>

struct Transform {
  Vec3 position{0.0f, 0.0f, 0.0f};
  Vec3 scaleV{1.0f, 1.0f, 1.0f};
  // rotation omitted for now (add quaternion later if you want)

  Mat4 matrix() const { return mul(translate(position), scale(scaleV)); }
};

// CPU-side vertex for the demo.
struct Vertex {
  float px, py, pz;
  float nx, ny, nz;
  float ux, uy;
};

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
};

struct Renderable {
  Transform transform;
  Mesh mesh;
};

class Scene {
public:
  Renderable &add(Renderable r) {
    m_items.push_back(std::move(r));
    return m_items.back();
  }

  const std::vector<Renderable> &items() const { return m_items; }

private:
  std::vector<Renderable> m_items;
};

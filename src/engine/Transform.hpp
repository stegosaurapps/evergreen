#pragma once

#include "Math.hpp"

struct Transform {
  Vec3 position{0.0f, 0.0f, 0.0f};
  Vec3 scaleV{1.0f, 1.0f, 1.0f};
  // rotation omitted for now (add quaternion later if you want)

  Mat4 matrix() const { return mul(translate(position), scale(scaleV)); }
};
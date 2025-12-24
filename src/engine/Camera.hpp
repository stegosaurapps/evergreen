#pragma once

#include "Math.hpp"

// Simple FPS/orbit-ish camera. Keep it tiny for now.

struct CameraUBO {
  Mat4 view;
  Mat4 proj;
  Mat4 viewProj;
  Vec4 viewPos;
};

class Camera {
public:
  void setPerspective(float fovyRadians, float aspect, float zNear, float zFar);
  void setPosition(Vec3 p);
  void lookAt(Vec3 target, Vec3 up = {0.0f, 1.0f, 0.0f});
  void onResize(int w, int h);

  // Call after changing params.
  void updateMatrices();

  const CameraUBO &ubo() const { return m_ubo; }

private:
  Vec3 m_pos{0.0f, 0.0f, 3.0f};
  Vec3 m_target{0.0f, 0.0f, 0.0f};
  Vec3 m_up{0.0f, 1.0f, 0.0f};

  float m_fovy = 1.04719755f; // 60deg
  float m_aspect = 16.0f / 9.0f;
  float m_zNear = 0.1f;
  float m_zFar = 200.0f;

  CameraUBO m_ubo{};
};

#include "Camera.hpp"

void Camera::setPerspective(float fovyRadians, float aspect, float zNear,
                            float zFar) {
  m_fovy = fovyRadians;
  m_aspect = aspect;
  m_zNear = zNear;
  m_zFar = zFar;
}

void Camera::setPosition(Vec3 p) { m_pos = p; }

void Camera::lookAt(Vec3 target, Vec3 up) {
  m_target = target;
  m_up = up;
}

void Camera::onResize(int w, int h) {
  if (w > 0 && h > 0) {
    m_aspect = (float)w / (float)h;
  }
}

void Camera::updateMatrices() {
  m_ubo.view = lookAtRH(m_pos, m_target, m_up);
  m_ubo.proj = perspectiveRH(m_fovy, m_aspect, m_zNear, m_zFar);
  m_ubo.viewProj = mul(m_ubo.proj, m_ubo.view);
  m_ubo.viewPos = {m_pos.x, m_pos.y, m_pos.z, 1.0f};
}

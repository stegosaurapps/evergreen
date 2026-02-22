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
  const float kPitchLimit = 1.4f;
  if (m_orbitPitch > kPitchLimit) {
    m_orbitPitch = kPitchLimit;
  }
  if (m_orbitPitch < -kPitchLimit) {
    m_orbitPitch = -kPitchLimit;
  }

  float cy = cosf(m_orbitYaw);
  float sy = sinf(m_orbitYaw);
  float cp = cosf(m_orbitPitch);
  float sp = sinf(m_orbitPitch);

  // Convention: yaw=0 looks down +Z, pitch>0 goes up
  Vec3 eye{
      m_orbitTarget.x + m_orbitRadius * (sy * cp),
      m_orbitTarget.y + m_orbitRadius * (sp),
      m_orbitTarget.z + m_orbitRadius * (cy * cp),
  };

  m_pos = eye;
  m_target = m_orbitTarget;

  m_ubo.view = lookAtRH(m_pos, m_target, m_up);
  m_ubo.proj = perspectiveRH(m_fovy, m_aspect, m_zNear, m_zFar);
  m_ubo.viewProj = mul(m_ubo.proj, m_ubo.view);
  m_ubo.viewPos = {m_pos.x, m_pos.y, m_pos.z, 1.0f};
}

void Camera::orbitStep(float dtSeconds, float yawSpeed) {
  m_orbitYaw += dtSeconds * yawSpeed;
  updateMatrices();
}

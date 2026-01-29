#pragma once

#include <cmath>

struct Vec3 {
  float x = 0.0f, y = 0.0f, z = 0.0f;
};

struct Vec4 {
  float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
};

inline Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 mul(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }

inline float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float length(Vec3 v) { return std::sqrt(dot(v, v)); }

inline Vec3 normalize(Vec3 v) {
  float len = length(v);
  if (len <= 0.0f) {
    return {0.0f, 0.0f, 0.0f};
  }
  return mul(v, 1.0f / len);
}

// Mat4 is column-major: m[col*4 + row].
struct Mat4 {
  float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  static Mat4 identity() { return Mat4{}; }
};

inline Mat4 mul(Mat4 a, Mat4 b) {
  Mat4 out{};
  // column-major multiplication: out = a * b
  for (int c = 0; c < 4; ++c) {
    for (int r = 0; r < 4; ++r) {
      out.m[c * 4 + r] =
          a.m[0 * 4 + r] * b.m[c * 4 + 0] + a.m[1 * 4 + r] * b.m[c * 4 + 1] +
          a.m[2 * 4 + r] * b.m[c * 4 + 2] + a.m[3 * 4 + r] * b.m[c * 4 + 3];
    }
  }
  return out;
}

inline Mat4 translate(Vec3 t) {
  Mat4 out = Mat4::identity();
  out.m[3 * 4 + 0] = t.x;
  out.m[3 * 4 + 1] = t.y;
  out.m[3 * 4 + 2] = t.z;
  return out;
}

inline Mat4 scale(Vec3 s) {
  Mat4 out = Mat4::identity();
  out.m[0] = s.x;
  out.m[5] = s.y;
  out.m[10] = s.z;
  return out;
}

// Right-handed perspective matrix.
inline Mat4 perspectiveRH(float fovyRadians, float aspect, float zNear,
                          float zFar) {
  Mat4 out{};
  for (int i = 0; i < 16; ++i) {
    out.m[i] = 0.0f;
  }

  float f = 1.0f / std::tan(fovyRadians * 0.5f);
  out.m[0] = f / aspect;
  out.m[5] = f;
  out.m[10] = zFar / (zNear - zFar);
  out.m[11] = -1.0f;
  out.m[14] = (zNear * zFar) / (zNear - zFar);
  return out;
}

inline Mat4 lookAtRH(Vec3 eye, Vec3 center, Vec3 up) {
  Vec3 f = normalize(sub(center, eye));
  Vec3 s = normalize(cross(f, up));
  Vec3 u = cross(s, f);

  Mat4 out = Mat4::identity();
  out.m[0] = s.x;
  out.m[4] = s.y;
  out.m[8] = s.z;

  out.m[1] = u.x;
  out.m[5] = u.y;
  out.m[9] = u.z;

  out.m[2] = -f.x;
  out.m[6] = -f.y;
  out.m[10] = -f.z;

  out.m[12] = -dot(s, eye);
  out.m[13] = -dot(u, eye);
  out.m[14] = dot(f, eye);

  return out;
}

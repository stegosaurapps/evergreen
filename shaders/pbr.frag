#version 450

layout(set = 0, binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  vec4 viewPos;
} ubo;

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec2 vUV;

layout(location = 0) out vec4 outColor;

// Placeholder "PBR".
// Replace this with your proper IBL BRDF + texture bindings.

void main() {
  vec3 N = normalize(vNormal);
  vec3 L = normalize(vec3(0.3, 0.7, 0.6));
  float ndotl = max(dot(N, L), 0.0);

  vec3 base = vec3(vUV, 1.0);
  vec3 color = base * (0.15 + 0.85 * ndotl);

  outColor = vec4(color, 1.0);
}

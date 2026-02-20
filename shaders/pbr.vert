#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent; // xyz tangent, w = handedness
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec2 vUV;
layout(location = 2) out vec3 vN;
layout(location = 3) out vec3 vT;
layout(location = 4) out vec3 vB;

layout(set = 0, binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  vec3 eye;
} ubo;

layout(push_constant) uniform Push {
  mat4 model;
} pc;

void main() {
  vec4 worldPos4 = pc.model * vec4(inPosition, 1.0);
  vWorldPos = worldPos4.xyz;

  // Proper normal transform
  mat3 normalMat = transpose(inverse(mat3(pc.model)));

  vec3 N = normalize(normalMat * inNormal);
  vec3 T = normalize(normalMat * inTangent.xyz);
  vec3 B = normalize(cross(N, T)) * inTangent.w; // handedness

  vN = N;
  vT = T;
  vB = B;

  vUV = inUV;

  gl_Position = ubo.viewProj * worldPos4;
}
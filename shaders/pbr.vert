#version 450

layout(set = 0, binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  vec4 viewPos;
} ubo;

layout(push_constant) uniform Push {
  mat4 model;
} push;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec2 vUV;

void main() {
  mat4 mvp = ubo.viewProj * push.model;
  gl_Position = mvp * vec4(inPos, 1.0);
  vNormal = mat3(push.model) * inNormal;
  vUV = inUV;
}

#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec4 vTangent;
layout(location = 2) in vec2 vUV;

layout(location = 0) out vec4 outColor;

void main() {
  vec3 testColor = vec3(0.0, 1.0, 0.0);

  vec3 n = normalize(vNormal);
  // vec3 l = normalize(vec3(-0.3, -1.0, -0.2));
  vec3 l = normalize(vec3(0.0, 0.0, 1.0));
  float ndotl = max(dot(n, -l), 0.0);

  vec3 lit = testColor * (0.15 + 0.85 * ndotl);
  outColor = vec4(lit, 1.0);

  // outColor = vec4(testColor, 1.0);
}

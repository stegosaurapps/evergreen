#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec4 vTangent;
layout(location = 2) in vec2 vUV;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D uAlbedo;
layout(set = 1, binding = 1) uniform sampler2D uMetalRough; // G=roughness, B=metallic (glTF)
layout(set = 1, binding = 2) uniform sampler2D uNormal;

layout(set = 2, binding = 0) uniform sampler2D   uBrdfLut;
layout(set = 2, binding = 1) uniform samplerCube uIrradiance;
layout(set = 2, binding = 2) uniform samplerCube uPrefiltered;

void main() {
  // vec3 testColor = vec3(0.0, 1.0, 0.0);

  // vec3 n = normalize(vNormal);
  // // vec3 l = normalize(vec3(-0.3, -1.0, -0.2));
  // vec3 l = normalize(vec3(0.0, 0.0, 1.0));
  // float ndotl = max(dot(n, -l), 0.0);

  // vec3 lit = testColor * (0.15 + 0.85 * ndotl);
  // outColor = vec4(lit, 1.0);

  // outColor = vec4(testColor, 1.0);

  vec2 uv = vec2(vUV.x, 1.0 - vUV.y);
  
  vec3 albedo = texture(uAlbedo, uv).rgb;
  outColor = vec4(albedo, 1.0);

  // vec3 normalTexture = texture(uNormal, uv).rgb;
  // vec3 normal = normalize(normalTexture * 2.0 - 1.0);
  // outColor = vec4(normal * 0.5 + 0.5, 1.0); 
}

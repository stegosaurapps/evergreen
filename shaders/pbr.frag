#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec2 vUV;
layout(location = 2) in vec3 vN;
layout(location = 3) in vec3 vT;
layout(location = 4) in vec3 vB;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform CameraUBO {
  mat4 view;
  mat4 proj;
  mat4 viewProj;
  vec3 eye;
} ubo;

// Material textures (set=1)
layout(set = 1, binding = 0) uniform sampler2D uAlbedo;
layout(set = 1, binding = 1) uniform sampler2D uMetalRough; // G=roughness, B=metallic (glTF)
layout(set = 1, binding = 2) uniform sampler2D uNormal;

// IBL (set=2)
layout(set = 2, binding = 0) uniform sampler2D   uBrdfLut;
layout(set = 2, binding = 1) uniform samplerCube uIrradiance;

// IMPORTANT:
// - If your specular prefilter is a true mipmapped cubemap -> use samplerCube + textureLod
// - If your specular prefilter is a cube array (roughness levels in layers) -> use samplerCubeArray + texture(..., vec4(dir, layer))
// From our earlier plan, you built it as a cube-array. Keep this:
layout(set = 2, binding = 2) uniform samplerCubeArray uPrefiltered;

const float PI = 3.14159265359;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

vec3 F_Schlick(vec3 F0, float cosTheta) {
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 TonemapACES(vec3 x) {
  // Narkowicz 2015 ACES approximation
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 NormalFromMap(vec2 uv) {
  // Tangent-space normal map (UNORM), expand to [-1,1]
  vec3 nTex = texture(uNormal, uv).xyz * 2.0 - 1.0;

  // If bumps look inside-out, try uncommenting this:
  // nTex.g = -nTex.g;

  mat3 TBN = mat3(normalize(vT), normalize(vB), normalize(vN));
  return normalize(TBN * nTex);
}

void main() {
  // Your UV flip (glTF convention vs your texture orientation)
  vec2 uv = vec2(vUV.x, 1.0 - vUV.y);

  // --- Material inputs ---
  // Albedo is sampled as linear if the image is VK_FORMAT_*_SRGB (which you are using).
  vec3 baseColor = texture(uAlbedo, uv).rgb;

  // glTF metallic-roughness packing: G = roughness, B = metallic
  vec2 mr = texture(uMetalRough, uv).gb;
  float roughness = clamp(mr.x, 0.04, 1.0);
  float metallic  = clamp(mr.y, 0.0, 1.0);

  vec3 N = NormalFromMap(uv);
  vec3 V = normalize(ubo.eye - vWorldPos);
  float NoV = saturate(dot(N, V));

  // Fresnel reflectance at normal incidence
  vec3 F0 = mix(vec3(0.04), baseColor, metallic);

  // --- Diffuse IBL ---
  vec3 irradiance = texture(uIrradiance, N).rgb;
  vec3 diffuse = irradiance * baseColor * (1.0 - metallic);

  // --- Specular IBL ---
  vec3 R = reflect(-V, N);

  // BRDF integration LUT is usually indexed by (NoV, roughness)
  vec2 brdf = texture(uBrdfLut, vec2(NoV, roughness)).rg;

  // Prefiltered environment:
  // You have 6 roughness levels authored as a cube-array (layers 0..5)
  const float SPEC_LEVELS = 6.0;
  float level = roughness * (SPEC_LEVELS - 1.0);

  vec3 prefiltered = texture(uPrefiltered, vec4(normalize(R), level)).rgb;

  vec3 F = F_Schlick(F0, NoV);
  vec3 specular = prefiltered * (F * brdf.x + brdf.y);

  // --- Combine ---
  vec3 color = diffuse + specular;

  // Optional global env intensity if your sky is "too hot"
  float envIntensity = 1.0; // try 0.6 if still blown out
  color *= envIntensity;

  // --- Color correction / tonemapping ---
  float exposure = 0.4;     0.4..1.2
  color *= exposure;

  color = TonemapACES(color);
  color = pow(color, vec3(1.0/2.2)); // gamma to display

  outColor = vec4(color, 1.0);
}

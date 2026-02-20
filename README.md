# Evergreen Renderer

Evergreen is a Vulkan-based physically based rendering (PBR) engine focused on modern real-time rendering techniques and clean GPU architecture.

It supports glTF asset loading, image-based lighting, and a descriptor-driven material system designed for extensibility.

---

## Features

### Core Rendering
- Vulkan backend
- Indexed mesh rendering
- Push-constant model transforms
- Per-frame camera uniform buffers

### glTF 2.0 Support
- Mesh + index extraction
- Tangents, normals, UVs
- Metallic-roughness materials
- sRGB albedo workflow
- Normal mapping (tangent space)

### Physically Based Rendering (PBR)
- Metallic-roughness shading model (glTF compliant)
- Split-sum image-based lighting
- Diffuse irradiance cubemap
- Specular prefiltered environment (cube array)
- BRDF integration LUT
- Fresnel-Schlick approximation
- Energy-conserving diffuse/specular blending

### HDR & Color Pipeline
- HDR environment support (.hdr)
- ACES filmic tonemapping
- Exposure control
- Gamma correction (linear → sRGB)

### Texture Support
- 2D textures (RGBA8 sRGB / UNORM)
- HDR textures (RGBA32F)
- Cubemaps
- Cubemap arrays (roughness levels)
- Combined image samplers via descriptor sets

---

## Descriptor Layout

| Set | Purpose |
|-----|---------|
| 0   | Frame (Camera UBO) |
| 1   | Material textures |
| 2   | Global IBL resources |

---

## Current Capabilities

- Fully functional IBL-only PBR shading
- glTF metallic-roughness materials
- Environment-driven reflections
- Clean separation of material and global lighting resources

---

## Planned Improvements

- Direct lighting support
- Environment importance sampling
- GPU-based mip generation
- Render graph / frame abstraction
- Path tracing experiments

---

## Dependencies

- Vulkan SDK
- cgltf
- stb_image

---

## Example Output

![Screenshot](output/evergreen-output.jpg)
Bright outdoor HDR IBL with ACES tonemapping applied.

---

Evergreen is designed as a foundation for further rendering research, experimentation, and integration into larger real-time engines.

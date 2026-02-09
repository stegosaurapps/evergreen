#pragma once

#include "Texture.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

class Material {
public:
  Material() = default;
  ~Material() = default;

  void init(Texture albedoTexture, Texture roughnessTexture,
            Texture metallicTexture, Texture normalTexture);

  void clear();

private:
  std::unique_ptr<Texture> m_albedoTexture = nullptr;
  std::unique_ptr<Texture> m_roughnessTexture = nullptr;
  std::unique_ptr<Texture> m_metallicTexture = nullptr;
  std::unique_ptr<Texture> m_normalTexture = nullptr;
};

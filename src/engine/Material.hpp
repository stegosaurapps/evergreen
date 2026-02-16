#pragma once

#include "Texture.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

class Material {
public:
  Material() = default;
  ~Material() = default;

  void init(Texture albedoTexture, Texture metallicRoughnessTexture,
            Texture normalTexture);

  void clear();

private:
  std::unique_ptr<Texture> m_albedoTexture = nullptr;
  std::unique_ptr<Texture> m_metallicRoughnessTexture = nullptr;
  std::unique_ptr<Texture> m_normalTexture = nullptr;
};

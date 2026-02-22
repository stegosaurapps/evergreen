#pragma once

#include "Texture.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

class Renderer; // forward declaration

class Material {
public:
  Material() = default;
  ~Material() = default;

  void init(VkDescriptorSet descriptorSet, Texture albedoTexture,
            Texture metallicRoughnessTexture, Texture normalTexture);

  VkDescriptorSet *descriptorSet();

  void clear(Renderer &renderer, VkDescriptorPool descriptorPool);

private:
  VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

  std::unique_ptr<Texture> m_albedoTexture = nullptr;
  std::unique_ptr<Texture> m_metallicRoughnessTexture = nullptr;
  std::unique_ptr<Texture> m_normalTexture = nullptr;
};

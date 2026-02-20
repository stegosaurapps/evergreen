#pragma once

#include "Constants.hpp"
#include "Texture.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <vector>

class Renderer; // forward declaration

class Sky {
public:
  Sky(VkDescriptorPool skyDescriptorPool,
      VkDescriptorSetLayout skyDescriptorSetLayout,
      VkDescriptorSet skyDescriptorSet, Texture brdfLut,
      Texture diffuseIrradiance, Texture specularIrradiance);
  ~Sky() = default;

  VkDescriptorPool *skyDescriptorPool();
  VkDescriptorSetLayout *skyDescriptorSetLayout();
  VkDescriptorSet *skyDescriptorSet();

  Texture &brdfLut();
  Texture &diffuseIrradiance();
  Texture &specularIrradiance();

private:
  // Frame Descriptor
  VkDescriptorPool m_skyDescriptorPool = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_skyDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorSet m_skyDescriptorSet;

  Texture m_brdfLut;            // 2D
  Texture m_diffuseIrradiance;  // cube (1 mip)
  Texture m_specularIrradiance; // cube (6 mips)
};

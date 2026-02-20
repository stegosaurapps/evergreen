#pragma once

#include "Texture.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

class Renderer; // forward declaration

class Sky {
public:
  Sky(VkDescriptorSet descriptorSet, Texture brdfLut, Texture diffuseIrradiance,
      Texture specularIrradiance);
  ~Sky() = default;

  Texture &brdfLut();

  Texture &diffuseIrradiance();

  Texture &specularIrradiance();

private:
  //   VkDescriptorSetLayout m_setLayout = VK_NULL_HANDLE;
  VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

  Texture m_brdfLut;            // 2D
  Texture m_diffuseIrradiance;  // cube (1 mip)
  Texture m_specularIrradiance; // cube (6 mips)
};

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
  Sky(Texture brdfLut, Texture diffuseIrradiance, Texture specularIrradiance);
  ~Sky() = default;

  void init(VkDescriptorPool skyDescriptorPool,
            VkDescriptorSetLayout skyDescriptorSetLayout,
            std::array<VkDescriptorSet, FRAME_COUNT> skyDescriptorSets);

  VkDescriptorPool *skyDescriptorPool();
  VkDescriptorSetLayout *skyDescriptorSetLayout();
  std::array<VkDescriptorSet, FRAME_COUNT> *skyDescriptorSets();

  Texture &brdfLut();
  Texture &diffuseIrradiance();
  Texture &specularIrradiance();

private:
  // Frame Descriptor
  VkDescriptorPool m_skyDescriptorPool = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_skyDescriptorSetLayout = VK_NULL_HANDLE;
  std::array<VkDescriptorSet, FRAME_COUNT> m_skyDescriptorSets{};

  Texture m_brdfLut;            // 2D
  Texture m_diffuseIrradiance;  // cube (1 mip)
  Texture m_specularIrradiance; // cube (6 mips)
};

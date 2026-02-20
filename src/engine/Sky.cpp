#include "Sky.hpp"

#include <iostream>

Sky::Sky(VkDescriptorPool skyDescriptorPool,
         VkDescriptorSetLayout skyDescriptorSetLayout,
         VkDescriptorSet skyDescriptorSet, Texture brdfLut,
         Texture diffuseIrradiance, Texture specularIrradiance)
    : m_skyDescriptorPool(skyDescriptorPool),
      m_skyDescriptorSetLayout(skyDescriptorSetLayout),
      m_skyDescriptorSet(skyDescriptorSet), m_brdfLut(brdfLut),
      m_diffuseIrradiance(diffuseIrradiance),
      m_specularIrradiance(specularIrradiance) {}

VkDescriptorPool *Sky::skyDescriptorPool() { return &m_skyDescriptorPool; };

VkDescriptorSetLayout *Sky::skyDescriptorSetLayout() {
  return &m_skyDescriptorSetLayout;
}

VkDescriptorSet *Sky::skyDescriptorSet() { return &m_skyDescriptorSet; }

Texture &Sky::brdfLut() { return m_brdfLut; }

Texture &Sky::diffuseIrradiance() { return m_diffuseIrradiance; }

Texture &Sky::specularIrradiance() { return m_specularIrradiance; }

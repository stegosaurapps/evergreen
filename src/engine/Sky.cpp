#include "Sky.hpp"

#include <iostream>

Sky::Sky(Texture brdfLut, Texture diffuseIrradiance, Texture specularIrradiance)
    : m_brdfLut(brdfLut), m_diffuseIrradiance(diffuseIrradiance),
      m_specularIrradiance(specularIrradiance) {}

void Sky::init(VkDescriptorPool skyDescriptorPool,
               VkDescriptorSetLayout skyDescriptorSetLayout,
               std::array<VkDescriptorSet, FRAME_COUNT> skyDescriptorSets) {
  m_skyDescriptorPool = skyDescriptorPool;
  m_skyDescriptorSetLayout = skyDescriptorSetLayout;
  m_skyDescriptorSets = skyDescriptorSets;
}

VkDescriptorPool *Sky::skyDescriptorPool() { return &m_skyDescriptorPool; };

VkDescriptorSetLayout *Sky::skyDescriptorSetLayout() {
  return &m_skyDescriptorSetLayout;
}

std::array<VkDescriptorSet, FRAME_COUNT> *Sky::skyDescriptorSets() {
  return &m_skyDescriptorSets;
}

Texture &Sky::brdfLut() { return m_brdfLut; }

Texture &Sky::diffuseIrradiance() { return m_diffuseIrradiance; }

Texture &Sky::specularIrradiance() { return m_specularIrradiance; }

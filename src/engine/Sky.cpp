#include "Sky.hpp"
#include "Renderer.hpp"

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

void Sky::clear(Renderer &renderer) {
  auto device = renderer.device();

  if (!device) {
    return;
  }

  m_brdfLut.clear(renderer);
  m_diffuseIrradiance.clear(renderer);
  m_specularIrradiance.clear(renderer);

  if (m_skyDescriptorSet != VK_NULL_HANDLE) {
    vkFreeDescriptorSets(device, m_skyDescriptorPool, 1, &m_skyDescriptorSet);
    m_skyDescriptorSet = VK_NULL_HANDLE;
  }

  if (m_skyDescriptorSetLayout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device, m_skyDescriptorSetLayout, nullptr);
    m_skyDescriptorSetLayout = VK_NULL_HANDLE;
  }

  if (m_skyDescriptorPool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, m_skyDescriptorPool, nullptr);
    m_skyDescriptorPool = VK_NULL_HANDLE;
  }
}

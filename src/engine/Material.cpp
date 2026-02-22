#include "Material.hpp"
#include "Renderer.hpp"

#include <iostream>

void Material::init(VkDescriptorSet descriptorSet, Texture albedoTexture,
                    Texture metallicRoughnessTexture, Texture normalTexture) {
  m_descriptorSet = descriptorSet;

  m_albedoTexture = std::make_unique<Texture>(albedoTexture);
  m_metallicRoughnessTexture =
      std::make_unique<Texture>(metallicRoughnessTexture);
  m_normalTexture = std::make_unique<Texture>(normalTexture);
}

VkDescriptorSet *Material::descriptorSet() { return &m_descriptorSet; }

void Material::clear(Renderer &renderer, VkDescriptorPool descriptorPool) {
  auto device = renderer.device();

  if (!device) {
    return;
  }

  m_albedoTexture->clear(renderer);
  m_metallicRoughnessTexture->clear(renderer);
  m_normalTexture->clear(renderer);

  if (m_descriptorSet != VK_NULL_HANDLE) {
    vkFreeDescriptorSets(device, descriptorPool, 1, &m_descriptorSet);
    m_descriptorSet = VK_NULL_HANDLE;
  }
}

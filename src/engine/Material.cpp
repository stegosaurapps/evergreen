#include "Material.hpp"

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

void Material::clear() {
  m_albedoTexture->clear();
  m_metallicRoughnessTexture->clear();
  m_normalTexture->clear();
}

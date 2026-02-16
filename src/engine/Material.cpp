#include "Material.hpp"

#include <iostream>

void Material::init(Texture albedoTexture, Texture metallicRoughnessTexture,
                    Texture normalTexture) {
  m_albedoTexture = std::make_unique<Texture>(albedoTexture);
  m_metallicRoughnessTexture =
      std::make_unique<Texture>(metallicRoughnessTexture);
  m_normalTexture = std::make_unique<Texture>(normalTexture);
}

void Material::clear() {
  m_albedoTexture->clear();
  m_metallicRoughnessTexture->clear();
  m_normalTexture->clear();
}

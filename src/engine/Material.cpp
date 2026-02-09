#include "Material.hpp"

#include <iostream>

void Material::init(Texture albedoTexture, Texture roughnessTexture,
                    Texture metallicTexture, Texture normalTexture) {
  m_albedoTexture = std::make_unique<Texture>(albedoTexture);
  m_roughnessTexture = std::make_unique<Texture>(roughnessTexture);
  m_metallicTexture = std::make_unique<Texture>(metallicTexture);
  m_normalTexture = std::make_unique<Texture>(normalTexture);
}

void Material::clear() {
  m_albedoTexture->clear();
  m_roughnessTexture->clear();
  m_metallicTexture->clear();
  m_normalTexture->clear();
}

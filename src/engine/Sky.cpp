#include "Sky.hpp"

#include <iostream>

Sky::Sky(VkDescriptorSet descriptorSet, Texture brdfLut,
         Texture diffuseIrradiance, Texture specularIrradiance)
    : m_descriptorSet(descriptorSet), m_brdfLut(brdfLut),
      m_diffuseIrradiance(diffuseIrradiance),
      m_specularIrradiance(specularIrradiance) {}

Texture &Sky::brdfLut() { return m_brdfLut; }

Texture &Sky::diffuseIrradiance() { return m_diffuseIrradiance; }

Texture &Sky::specularIrradiance() { return m_specularIrradiance; }

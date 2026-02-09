#include "Texture.hpp"

#include <iostream>

void Texture::init(uint32_t width, uint32_t height, uint32_t mipLevels,
                   VkFormat format, VkImage image, VkDeviceMemory memory,
                   VkImageView view, VkSampler sampler) {
  m_width = width;
  m_height = height;
  m_mipLevels = mipLevels;

  m_format = format;

  m_image = image;
  m_memory = memory;
  m_view = view;
  m_sampler = sampler;
}

void Texture::clear() {}

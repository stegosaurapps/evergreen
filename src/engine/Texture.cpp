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

VkImage Texture::image() { return m_image; }

VkImageView Texture::view() { return m_view; }

VkSampler Texture::sampler() { return m_sampler; }

void Texture::clear() {
  // if (m_sampler) { vkDestroySampler(device, m_sampler, nullptr); m_sampler =
  // VK_NULL_HANDLE; } if (m_view)    { vkDestroyImageView(device, m_view,
  // nullptr); m_view = VK_NULL_HANDLE; } if (m_image)   {
  // vkDestroyImage(device, m_image, nullptr); m_image = VK_NULL_HANDLE; } if
  // (m_memory)  { vkFreeMemory(device, m_memory, nullptr); m_memory =
  // VK_NULL_HANDLE; }

  // m_width = 0;
  // m_height = 0;
  // m_mipLevels = 1;
  // m_format = VK_FORMAT_UNDEFINED;
}

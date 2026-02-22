#pragma once

#include <vulkan/vulkan.h>

#include <vector>

class Renderer; // forward declaration

class Texture {
public:
  Texture() = default;
  ~Texture() = default;

  void init(uint32_t width, uint32_t height, uint32_t mipLevels,
            VkFormat format, VkImage image, VkDeviceMemory memory,
            VkImageView view, VkSampler sampler);

  VkImage image();
  VkImageView view();
  VkSampler sampler();

  void clear(Renderer &renderer);

private:
  uint32_t m_width = 0;
  uint32_t m_height = 0;
  uint32_t m_mipLevels = 1;

  VkFormat m_format = VK_FORMAT_UNDEFINED;

  VkImage m_image = VK_NULL_HANDLE;
  VkDeviceMemory m_memory = VK_NULL_HANDLE;
  VkImageView m_view = VK_NULL_HANDLE;
  VkSampler m_sampler = VK_NULL_HANDLE;
};

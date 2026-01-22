#pragma once

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <SDL3/SDL.h>

#include <vulkan/vulkan.h>

#include <windows.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

void CreateDescriptorSetLayout(VkDevice device,
                               VkDescriptorSetLayout *descriptorSetLayout) {
  VkDescriptorSetLayoutBinding cam{};
  cam.binding = 0;
  cam.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  cam.descriptorCount = 1;
  cam.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo createInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  createInfo.bindingCount = 1;
  createInfo.pBindings = &cam;

  if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr,
                                  descriptorSetLayout) != VK_SUCCESS) {
    std::cerr << "vkCreateDescriptorSetLayout failed" << std::endl;
    std::abort();
  }
}

void CreateDescriptorPool(
    VkDevice device, VkDescriptorPool *descriptorPool,
    VkDescriptorSetLayout *descriptorSetLayout,
    std::array<VkDescriptorSet, FRAME_COUNT> *descriptorSets) {
  VkDescriptorPoolSize size{};
  size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  size.descriptorCount = FRAME_COUNT;

  VkDescriptorPoolCreateInfo createInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  createInfo.maxSets = FRAME_COUNT;
  createInfo.poolSizeCount = 1;
  createInfo.pPoolSizes = &size;

  if (vkCreateDescriptorPool(device, &createInfo, nullptr, descriptorPool) !=
      VK_SUCCESS) {
    std::cerr << "vkCreateDescriptorPool failed" << std::endl;
    std::abort();
  }

  std::array<VkDescriptorSetLayout, FRAME_COUNT> layouts;
  for (int i = 0; i < FRAME_COUNT; ++i) {
    layouts[i] = *descriptorSetLayout;
  }

  VkDescriptorSetAllocateInfo allocateInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocateInfo.descriptorPool = *descriptorPool;
  allocateInfo.descriptorSetCount = FRAME_COUNT;
  allocateInfo.pSetLayouts = layouts.data();

  if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets->data()) !=
      VK_SUCCESS) {
    std::cerr << "vkAllocateDescriptorSets failed" << std::endl;
    std::abort();
  }
}

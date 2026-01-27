#pragma once

#include "Camera.hpp"
#include "Constants.hpp"
#include "Dimensions.hpp"
#include "Platform.hpp"
#include "Scene.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>

class Renderer {
public:
  Renderer() = default;
  ~Renderer() = default;

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  bool init(const Win32WindowHandles &windowHandler, int width, int height,
            bool enableValidation);

  int frameIndex();
  Dimensions dimensions();
  VkPhysicalDevice physicalDevice();
  VkDevice device();
  VkSampleCountFlagBits sampleCount();
  VkRenderPass renderPass();

  void resize(int width, int height);
  void update(float deltaTime);
  void drawFrame(Scene *scene);

  void shutdown();

private:
  bool m_enableValidation = false;
  bool m_swapchainDirty = false;
  int m_width = 0;
  int m_height = 0;

  std::array<float, 3> m_clearColor = {0.0, 0.0, 1.0}; // {0.10, 0.16, 0.18};

  VkInstance m_instance = VK_NULL_HANDLE;

  VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;

  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;

  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;

  VkSampleCountFlagBits m_samples = VK_SAMPLE_COUNT_1_BIT;

  VkImage m_colorImage = VK_NULL_HANDLE;
  VkDeviceMemory m_colorMemory = VK_NULL_HANDLE;
  VkImageView m_colorView = VK_NULL_HANDLE;

  uint32_t m_graphicsFamily = UINT32_MAX;
  uint32_t m_presentFamily = UINT32_MAX;
  VkQueue m_graphicsQueue = VK_NULL_HANDLE;
  VkQueue m_presentQueue = VK_NULL_HANDLE;

  VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
  VkFormat m_swapchainFormat{};
  VkExtent2D m_swapchainExtent{};
  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;

  VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
  VkImage m_depthImage = VK_NULL_HANDLE;
  VkDeviceMemory m_depthMemory = VK_NULL_HANDLE;
  VkImageView m_depthView = VK_NULL_HANDLE;

  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> m_framebuffers;

  VkCommandPool m_cmdPool = VK_NULL_HANDLE;

  int m_frameIndex = 0;
  std::array<VkCommandBuffer, FRAME_COUNT> m_cmd{};
  std::array<VkSemaphore, FRAME_COUNT> m_imageAvailable{};
  std::array<VkSemaphore, FRAME_COUNT> m_renderFinished{};
  std::array<VkFence, FRAME_COUNT> m_inFlight{};

  void createInstance();
  void setupDebug();
  void createSurface(const Win32WindowHandles &wh);
  void createPhysicalDevice();
  void createDevice();
  void createSwapchain(int width, int height);
  void createSwapchainViews();
  void createRenderPass();
  void createColorResources();
  void createDepthResources();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffers();
  void createSyncObjects();

  void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex,
                           Scene *scene);

  void waitDeviceIdle();

  void recreateSwapchainIfNeeded(Scene *scene);

  void destroySwapchain();
  void destroyColorResources();
  void destroyDepthResources();
};

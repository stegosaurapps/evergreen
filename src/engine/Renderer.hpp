#pragma once

#include "Platform.hpp"

#include "Camera.hpp"
#include "Scene.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <vector>

class Renderer {
public:
  Renderer() = default;
  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  bool init(const Win32WindowHandles &wh, int width, int height,
            bool enableValidation);
  void shutdown();

  void onResize(int width, int height); // mark swapchain dirty
  void drawFrame();                     // present one frame

private:
  // --- high-level phases ---
  bool createInstance();
  bool setupDebug();
  bool createSurface(const Win32WindowHandles &wh);
  bool pickPhysicalDevice();
  bool createDevice();
  bool createSwapchain(int width, int height);
  bool createSwapchainViews();
  bool createRenderPass();
  bool createDepthResources();
  bool createFramebuffers();
  bool createCommandPool();
  bool createCommandBuffers();
  bool createSyncObjects();

  // Demo render plumbing
  bool createDescriptorSetLayout();
  bool createDescriptorPool();
  bool createUniformBuffers();
  bool createPipeline();
  bool createMeshBuffers();

  void destroySwapchain();
  bool recreateSwapchainIfNeeded();

  void destroyDepthResources();
  void destroyPipelineResources();

  // --- helpers ---
  void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
  void waitDeviceIdle();

  void updatePerFrameUBO(int frameIndex);

private:
  static constexpr int kFramesInFlight = 2;

  bool m_enableValidation = false;
  bool m_swapchainDirty = false;
  int m_width = 0;
  int m_height = 0;

  VkInstance m_instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

  VkSurfaceKHR m_surface = VK_NULL_HANDLE;

  VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
  VkDevice m_device = VK_NULL_HANDLE;

  uint32_t m_graphicsFamily = UINT32_MAX;
  uint32_t m_presentFamily = UINT32_MAX;
  VkQueue m_graphicsQueue = VK_NULL_HANDLE;
  VkQueue m_presentQueue = VK_NULL_HANDLE;

  VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
  VkFormat m_swapchainFormat{};
  VkExtent2D m_swapchainExtent{};
  std::vector<VkImage> m_swapchainImages;
  std::vector<VkImageView> m_swapchainImageViews;

  // Depth
  VkFormat m_depthFormat = VK_FORMAT_D32_SFLOAT;
  VkImage m_depthImage = VK_NULL_HANDLE;
  VkDeviceMemory m_depthMemory = VK_NULL_HANDLE;
  VkImageView m_depthView = VK_NULL_HANDLE;

  VkRenderPass m_renderPass = VK_NULL_HANDLE;
  std::vector<VkFramebuffer> m_framebuffers;

  VkCommandPool m_cmdPool = VK_NULL_HANDLE;

  // --- demo scene state ---
  Camera m_camera;
  Scene m_scene;

  // Descriptors (set 0 = per-frame camera)
  VkDescriptorSetLayout m_setLayoutFrame = VK_NULL_HANDLE;
  VkDescriptorPool m_descPool = VK_NULL_HANDLE;
  std::array<VkDescriptorSet, kFramesInFlight> m_descSetFrame{};

  // Uniform buffers (one per frame-in-flight)
  std::array<VkBuffer, kFramesInFlight> m_uboBuffer{};
  std::array<VkDeviceMemory, kFramesInFlight> m_uboMemory{};
  std::array<void *, kFramesInFlight> m_uboMapped{};

  // Pipeline
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_pipeline = VK_NULL_HANDLE;

  // One demo mesh on GPU (first scene renderable)
  VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
  VkBuffer m_indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;
  uint32_t m_indexCount = 0;

  int m_frameIndex = 0;
  std::array<VkCommandBuffer, kFramesInFlight> m_cmd{};
  std::array<VkSemaphore, kFramesInFlight> m_imageAvailable{};
  std::array<VkSemaphore, kFramesInFlight> m_renderFinished{};
  std::array<VkFence, kFramesInFlight> m_inFlight{};
};

#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "../Vulkan.hpp"

#include "Cube.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "Vertex.hpp"

#include <SDL3/SDL.h>

#include <windows.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>

bool Renderer::init(const Win32WindowHandles &windowHandler, int width,
                    int height, bool enableValidation) {

  m_enableValidation = enableValidation;

  m_width = width;
  m_height = height;

  createInstance();
  setupDebug();

  createSurface(windowHandler);
  createPhysicalDevice();
  createDevice();
  createSwapchain(width, height);
  createSwapchainViews();
  createRenderPass();
  createColorResources();
  createDepthResources();
  createFramebuffers();
  createCommandPool();
  createCommandBuffers();
  createSyncObjects();

  std::cout << "Renderer init OK.\n";

  return true;
}

int Renderer::frameIndex() { return m_frameIndex; };

Dimensions Renderer::dimensions() {
  return Dimensions{(float)m_width, (float)m_height};
};

VkPhysicalDevice Renderer::physicalDevice() { return m_physicalDevice; }

VkDevice Renderer::device() { return m_device; }

VkSampleCountFlagBits Renderer::sampleCount() { return m_sampleCount; }

VkRenderPass Renderer::renderPass() { return m_renderPass; }

void Renderer::resize(int width, int height) {
  m_width = width;
  m_height = height;
  m_swapchainDirty = true;
}

void Renderer::update(float deltaTime) {}

void Renderer::drawFrame(Scene *scene) {
  if (!m_device) {
    return;
  }

  recreateSwapchainIfNeeded(scene);

  const int frameIndex = m_frameIndex;

  vkWaitForFences(m_device, 1, &m_inFlight[frameIndex], VK_TRUE, UINT64_MAX);
  vkResetFences(m_device, 1, &m_inFlight[frameIndex]);

  uint32_t imageIndex = 0;
  VkResult result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
                                          m_imageAvailable[frameIndex],
                                          VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    m_swapchainDirty = true;
    return;
  }
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    std::cerr << "vkAcquireNextImageKHR failed: " << (int)result << std::endl;
    std::abort();
  }

  vkResetCommandBuffer(m_cmd[frameIndex], 0);
  recordCommandBuffer(m_cmd[frameIndex], imageIndex, scene);

  VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &m_imageAvailable[frameIndex];
  submitInfo.pWaitDstStageMask = &waitStage;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_cmd[frameIndex];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &m_renderFinished[frameIndex];

  result =
      vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlight[frameIndex]);
  if (result != VK_SUCCESS) {
    std::cerr << "vkQueueSubmit failed: " << (int)result << std::endl;
    std::abort();
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &m_renderFinished[frameIndex];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &m_swapchain;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    m_swapchainDirty = true;
  } else if (result != VK_SUCCESS) {
    std::cerr << "vkQueuePresentKHR failed: " << (int)result << std::endl;
    std::abort();
  }

  m_frameIndex = (m_frameIndex + 1) % FRAME_COUNT;
}

void Renderer::shutdown() {
  if (!m_device && !m_instance) {
    return; // already down
  }

  waitDeviceIdle();

  // Per-frame sync
  for (int i = 0; i < FRAME_COUNT; ++i) {
    if (m_imageAvailable[i]) {
      vkDestroySemaphore(m_device, m_imageAvailable[i], nullptr);
    }
    if (m_renderFinished[i]) {
      vkDestroySemaphore(m_device, m_renderFinished[i], nullptr);
    }
    if (m_inFlight[i]) {
      vkDestroyFence(m_device, m_inFlight[i], nullptr);
    }

    m_imageAvailable[i] = VK_NULL_HANDLE;
    m_renderFinished[i] = VK_NULL_HANDLE;
    m_inFlight[i] = VK_NULL_HANDLE;
  }

  if (m_cmdPool) {
    vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
    m_cmdPool = VK_NULL_HANDLE;
  }

  destroySwapchain();

  if (m_device) {
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;
  }

  if (m_surface) {
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    m_surface = VK_NULL_HANDLE;
  }

  if (m_enableValidation && m_debugMessenger) {
    auto pfnDestroy =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            m_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (pfnDestroy) {
      pfnDestroy(m_instance, m_debugMessenger, nullptr);
    }

    m_debugMessenger = VK_NULL_HANDLE;
  }

  if (m_instance) {
    vkDestroyInstance(m_instance, nullptr);
    m_instance = VK_NULL_HANDLE;
  }

  m_physicalDevice = VK_NULL_HANDLE;
  m_graphicsQueue = VK_NULL_HANDLE;
  m_presentQueue = VK_NULL_HANDLE;
  m_graphicsFamily = UINT32_MAX;
  m_presentFamily = UINT32_MAX;

  std::cout << "Renderer shutdown OK.\n";
}

void Renderer::createInstance() {
  // Required instance extensions for Win32 surface.
  std::vector<const char *> exts = {VK_KHR_SURFACE_EXTENSION_NAME,
                                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

  if (m_enableValidation) {
    if (!CheckInstanceExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
      std::cerr << "Missing instance extension: "
                << VK_EXT_DEBUG_UTILS_EXTENSION_NAME << std::endl;
      std::abort();
    }

    exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    if (!CheckInstanceLayerAvailable(kValidationLayer)) {
      std::cerr << "Validation requested but layer not available: "
                << kValidationLayer << std::endl;
      std::abort();
    }
  }

  VkApplicationInfo app{};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pEngineName = "evergreen";
  app.pApplicationName = "evergreen";
  app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  app.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  ci.pApplicationInfo = &app;
  ci.enabledExtensionCount = (uint32_t)exts.size();
  ci.ppEnabledExtensionNames = exts.data();

  const char *layers[] = {kValidationLayer};
  if (m_enableValidation) {
    ci.enabledLayerCount = 1;
    ci.ppEnabledLayerNames = layers;
  }

  VkResult r = vkCreateInstance(&ci, nullptr, &m_instance);
  if (r != VK_SUCCESS || !m_instance) {
    std::cerr << "vkCreateInstance failed: " << (int)r << std::endl;
    std::abort();
  }
}

void Renderer::setupDebug() {
  if (!m_enableValidation) {
    return;
  }

  auto pfnCreate = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      m_instance, "vkCreateDebugUtilsMessengerEXT");

  if (!pfnCreate) {
    std::cerr << "vkCreateDebugUtilsMessengerEXT not found.\n";
    std::abort();
  }

  VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo{};
  debugUtilsMessengerCreateInfo.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debugUtilsMessengerCreateInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debugUtilsMessengerCreateInfo.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debugUtilsMessengerCreateInfo.pfnUserCallback = DebugCallback;

  VkResult r = pfnCreate(m_instance, &debugUtilsMessengerCreateInfo, nullptr,
                         &m_debugMessenger);
  if (r != VK_SUCCESS) {
    std::cerr << "CreateDebugUtilsMessenger failed: " << (int)r << std::endl;
    std::abort();
  }
}

void Renderer::createSurface(const Win32WindowHandles &wh) {
  if (!wh.hwnd || !wh.hinstance) {
    std::cerr << "createSurface: invalid window handles.\n";
    std::abort();
  }

  VkWin32SurfaceCreateInfoKHR sci{};
  sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  sci.hwnd = (HWND)wh.hwnd;
  sci.hinstance = (HINSTANCE)wh.hinstance;

  VkResult r = vkCreateWin32SurfaceKHR(m_instance, &sci, nullptr, &m_surface);
  if (r != VK_SUCCESS || !m_surface) {
    std::cerr << "vkCreateWin32SurfaceKHR failed: " << (int)r << std::endl;
    std::abort();
  }
}

void Renderer::createPhysicalDevice() {
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
  if (!count) {
    std::cerr << "No Vulkan physical devices found.\n";
    std::abort();
  }

  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

  const std::vector<const char *> requiredDevExts = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  auto scoreDevice = [&](VkPhysicalDevice pd, uint32_t &outGfx,
                         uint32_t &outPresent) -> int {
    // Must support swapchain extension.
    if (!CheckDeviceExtensionSupport(pd, requiredDevExts)) {
      std::cerr << "Swapchain extension is not supported.\n";
      std::abort();
    }

    // Find queue families.
    uint32_t qCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> qprops(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &qCount, qprops.data());

    std::optional<uint32_t> gfx;
    std::optional<uint32_t> present;

    for (uint32_t i = 0; i < qCount; ++i) {
      if (qprops[i].queueCount == 0) {
        continue;
      }

      if (qprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        if (!gfx) {
          gfx = i;
        }
      }

      VkBool32 supportsPresent = VK_FALSE;
      vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, m_surface, &supportsPresent);
      if (supportsPresent) {
        if (!present) {
          present = i;
        }
      }
    }

    if (!gfx || !present) {
      std::cerr << "Physical device could not be configured.\n";
      std::abort();
    }

    // Swapchain must have at least one format + present mode.
    auto sup = QuerySwapchainSupport(pd, m_surface);
    if (sup.formats.empty() || sup.presentModes.empty()) {
      std::cerr << "Swapchain must have at least one supported format and "
                   "present mode.\n";
      std::abort();
    }

    VkPhysicalDeviceProperties physicalDeviceProperties{};
    vkGetPhysicalDeviceProperties(pd, &physicalDeviceProperties);

    VkSampleCountFlags counts =
        physicalDeviceProperties.limits.framebufferColorSampleCounts &
        physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_8_BIT) {
      m_sampleCount = VK_SAMPLE_COUNT_8_BIT;
    } else if (counts & VK_SAMPLE_COUNT_4_BIT) {
      m_sampleCount = VK_SAMPLE_COUNT_4_BIT;
    } else if (counts & VK_SAMPLE_COUNT_2_BIT) {
      m_sampleCount = VK_SAMPLE_COUNT_2_BIT;
    }

    int score = 0;
    if (physicalDeviceProperties.deviceType ==
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      score += 1000;
    }
    score += (int)physicalDeviceProperties.limits.maxImageDimension2D;

    outGfx = *gfx;
    outPresent = *present;
  };

  int bestScore = -1;
  VkPhysicalDevice best = VK_NULL_HANDLE;
  uint32_t bestG = UINT32_MAX, bestP = UINT32_MAX;

  for (auto pd : devices) {
    uint32_t g = UINT32_MAX, p = UINT32_MAX;
    int s = scoreDevice(pd, g, p);
    if (s > bestScore) {
      bestScore = s;
      best = pd;
      bestG = g;
      bestP = p;
    }
  }

  if (!best) {
    std::cerr << "No suitable GPU found.\n";
    std::abort();
  }

  m_physicalDevice = best;
  m_graphicsFamily = bestG;
  m_presentFamily = bestP;

  VkPhysicalDeviceProperties physicalDeviceProperties{};
  vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);

  std::cout << "Using GPU: " << physicalDeviceProperties.deviceName
            << std::endl;
  std::cout << "Queue families: graphics=" << m_graphicsFamily
            << " present=" << m_presentFamily << std::endl;
}

void Renderer::createDevice() {
  std::set<uint32_t> uniqueFamilies = {m_graphicsFamily, m_presentFamily};
  float priority = 1.0f;

  std::vector<VkDeviceQueueCreateInfo> qcis;
  qcis.reserve(uniqueFamilies.size());

  for (uint32_t fam : uniqueFamilies) {
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = fam;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;
    qcis.push_back(qci);
  }

  const std::vector<const char *> devExts = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkPhysicalDeviceFeatures features{}; // keep minimal for now

  VkDeviceCreateInfo dci{};
  dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  dci.queueCreateInfoCount = (uint32_t)qcis.size();
  dci.pQueueCreateInfos = qcis.data();
  dci.pEnabledFeatures = &features;
  dci.enabledExtensionCount = (uint32_t)devExts.size();
  dci.ppEnabledExtensionNames = devExts.data();

  // Device layers are ignored on modern loaders, but harmless if present.
  const char *layers[] = {kValidationLayer};
  if (m_enableValidation) {
    dci.enabledLayerCount = 1;
    dci.ppEnabledLayerNames = layers;
  }

  VkResult r = vkCreateDevice(m_physicalDevice, &dci, nullptr, &m_device);
  if (r != VK_SUCCESS || !m_device) {
    std::cerr << "vkCreateDevice failed: " << (int)r << std::endl;
    std::abort();
  }

  vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, m_presentFamily, 0, &m_presentQueue);
}

void Renderer::createSwapchain(int width, int height) {
  auto sup = QuerySwapchainSupport(m_physicalDevice, m_surface);

  VkSurfaceFormatKHR fmt = ChooseSurfaceFormat(sup.formats);
  VkPresentModeKHR pm = ChoosePresentMode(sup.presentModes);
  VkExtent2D extent = ChooseExtent(sup.caps, width, height);

  uint32_t imageCount = sup.caps.minImageCount + 1;
  if (sup.caps.maxImageCount > 0 && imageCount > sup.caps.maxImageCount) {
    imageCount = sup.caps.maxImageCount;
  }

  VkSwapchainCreateInfoKHR sci{};
  sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  sci.surface = m_surface;
  sci.minImageCount = imageCount;
  sci.imageFormat = fmt.format;
  sci.imageColorSpace = fmt.colorSpace;
  sci.imageExtent = extent;
  sci.imageArrayLayers = 1;
  sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t qIdx[] = {m_graphicsFamily, m_presentFamily};
  if (m_graphicsFamily != m_presentFamily) {
    sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    sci.queueFamilyIndexCount = 2;
    sci.pQueueFamilyIndices = qIdx;
  } else {
    sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  sci.preTransform = sup.caps.currentTransform;
  sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sci.presentMode = pm;
  sci.clipped = VK_TRUE;
  sci.oldSwapchain = VK_NULL_HANDLE;

  VkResult r = vkCreateSwapchainKHR(m_device, &sci, nullptr, &m_swapchain);
  if (r != VK_SUCCESS || !m_swapchain) {
    std::cerr << "vkCreateSwapchainKHR failed: " << (int)r << std::endl;
    std::abort();
  }

  m_swapchainFormat = fmt.format;
  m_swapchainExtent = extent;

  uint32_t scCount = 0;
  vkGetSwapchainImagesKHR(m_device, m_swapchain, &scCount, nullptr);
  m_swapchainImages.resize(scCount);
  vkGetSwapchainImagesKHR(m_device, m_swapchain, &scCount,
                          m_swapchainImages.data());
}

void Renderer::createSwapchainViews() {
  m_swapchainImageViews.resize(m_swapchainImages.size());

  for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
    VkImageViewCreateInfo iv{};
    iv.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iv.image = m_swapchainImages[i];
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = m_swapchainFormat;
    iv.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    iv.subresourceRange.baseMipLevel = 0;
    iv.subresourceRange.levelCount = 1;
    iv.subresourceRange.baseArrayLayer = 0;
    iv.subresourceRange.layerCount = 1;

    VkResult r =
        vkCreateImageView(m_device, &iv, nullptr, &m_swapchainImageViews[i]);
    if (r != VK_SUCCESS) {
      std::cerr << "vkCreateImageView failed: " << (int)r << std::endl;
      std::abort();
    }
  }
}

void Renderer::createRenderPass() {
  VkSubpassDependency subpassDependency{};
  subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependency.dstSubpass = 0;
  subpassDependency.srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  subpassDependency.dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  subpassDependency.srcAccessMask = 0;
  subpassDependency.dstAccessMask =
      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassCreateInfo{};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.dependencyCount = 1;
  renderPassCreateInfo.pDependencies = &subpassDependency;

  if (m_sampleCount == VK_SAMPLE_COUNT_1_BIT) {
    // ----- No MSAA path: swapchain color + depth -----

    VkAttachmentDescription color{};
    color.format = m_swapchainFormat;
    color.samples = VK_SAMPLE_COUNT_1_BIT;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth{};
    depth.format = m_depthFormat;
    depth.samples = VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{
        1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkAttachmentDescription atts[2] = {color, depth};
    renderPassCreateInfo.attachmentCount = 2;
    renderPassCreateInfo.pAttachments = atts;
    renderPassCreateInfo.pSubpasses = &subpass;

    VkResult r = vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr,
                                    &m_renderPass);
    if (r != VK_SUCCESS) {
      std::cerr << "vkCreateRenderPass failed: " << (int)r << std::endl;
      std::abort();
    }
  } else {
    // ----- MSAA path: msaaColor + resolve(swapchain) + msaaDepth -----

    VkAttachmentDescription msaaColor{};
    msaaColor.format = m_swapchainFormat;
    msaaColor.samples = m_sampleCount;
    msaaColor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    msaaColor.storeOp =
        VK_ATTACHMENT_STORE_OP_DONT_CARE; // don't need to store MSAA buffer
    msaaColor.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    msaaColor.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    msaaColor.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    msaaColor.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Attachment 1: resolve color (this is the swapchain image view)
    VkAttachmentDescription resolve{};
    resolve.format = m_swapchainFormat;
    resolve.samples = VK_SAMPLE_COUNT_1_BIT;
    resolve.loadOp =
        VK_ATTACHMENT_LOAD_OP_DONT_CARE; // will be written by resolve op
    resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // we present it
    resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Attachment 2: multisampled depth
    VkAttachmentDescription depth{};
    depth.format = m_depthFormat;
    depth.samples = m_sampleCount;
    depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference resolveRef{1,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depthRef{
        2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;
    subpass.pResolveAttachments = &resolveRef;
    subpass.pDepthStencilAttachment = &depthRef;

    VkAttachmentDescription atts[3] = {msaaColor, resolve, depth};
    renderPassCreateInfo.attachmentCount = 3;
    renderPassCreateInfo.pAttachments = atts;
    renderPassCreateInfo.pSubpasses = &subpass;

    VkResult r = vkCreateRenderPass(m_device, &renderPassCreateInfo, nullptr,
                                    &m_renderPass);
    if (r != VK_SUCCESS) {
      std::cerr << "vkCreateRenderPass failed: " << (int)r << std::endl;
      std::abort();
    }
  }
}

void Renderer::createColorResources() {
  if (m_colorView) {
    vkDestroyImageView(m_device, m_colorView, nullptr);
    m_colorView = VK_NULL_HANDLE;
  }

  if (m_colorImage) {
    vkDestroyImage(m_device, m_colorImage, nullptr);
    m_colorImage = VK_NULL_HANDLE;
  }

  if (m_colorMemory) {
    vkFreeMemory(m_device, m_colorMemory, nullptr);
    m_colorMemory = VK_NULL_HANDLE;
  }

  if (m_sampleCount == VK_SAMPLE_COUNT_1_BIT) {
    return; // no MSAA needed
  }

  if (!CreateImage2D(m_physicalDevice, m_device, m_sampleCount,
                     m_swapchainExtent.width, m_swapchainExtent.height,
                     m_swapchainFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                     m_colorImage, m_colorMemory)) {
    std::cerr << "Failed to create MSAA color image" << std::endl;
    std::abort();
  }

  m_colorView = CreateImageView(m_device, m_colorImage, m_swapchainFormat,
                                VK_IMAGE_ASPECT_COLOR_BIT);

  if (!m_colorView) {
    std::cerr << "Failed to create MSAA color image view" << std::endl;
    std::abort();
  }
}

void Renderer::createDepthResources() {
  if (m_depthView) {
    destroyDepthResources();
  }

  if (!CreateImage2D(m_physicalDevice, m_device, m_sampleCount,
                     m_swapchainExtent.width, m_swapchainExtent.height,
                     m_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                     m_depthImage, m_depthMemory)) {
    std::cerr << "Failed to create depth image" << std::endl;
    std::abort();
  }

  m_depthView = CreateImageView(m_device, m_depthImage, m_depthFormat,
                                VK_IMAGE_ASPECT_DEPTH_BIT);
  if (!m_depthView) {
    std::cerr << "Failed to create depth image view" << std::endl;
    std::abort();
  }
}

void Renderer::createFramebuffers() {
  m_framebuffers.resize(m_swapchainImageViews.size());

  for (size_t i = 0; i < m_swapchainImageViews.size(); ++i) {
    VkFramebufferCreateInfo frameBufferInfo{};

    if (m_sampleCount == VK_SAMPLE_COUNT_1_BIT) {
      VkImageView attachments[] = {m_swapchainImageViews[i], m_depthView};

      frameBufferInfo.pAttachments = attachments;
      frameBufferInfo.attachmentCount = 2;
    } else {
      VkImageView attachments[] = {
          m_colorView,              // attachment 0: MSAA color
          m_swapchainImageViews[i], // attachment 1: resolve (presented)
          m_depthView               // attachment 2: depth
      };

      frameBufferInfo.pAttachments = attachments;
      frameBufferInfo.attachmentCount = 3;
    }

    frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferInfo.renderPass = m_renderPass;
    frameBufferInfo.width = m_swapchainExtent.width;
    frameBufferInfo.height = m_swapchainExtent.height;
    frameBufferInfo.layers = 1;

    VkResult r = vkCreateFramebuffer(m_device, &frameBufferInfo, nullptr,
                                     &m_framebuffers[i]);
    if (r != VK_SUCCESS) {
      std::cerr << "vkCreateFramebuffer failed: " << (int)r << std::endl;
      std::abort();
    }
  }
}

void Renderer::createCommandPool() {
  VkCommandPoolCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  ci.queueFamilyIndex = m_graphicsFamily;
  ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VkResult r = vkCreateCommandPool(m_device, &ci, nullptr, &m_cmdPool);
  if (r != VK_SUCCESS) {
    std::cerr << "vkCreateCommandPool failed: " << (int)r << std::endl;
    std::abort();
  }
}

void Renderer::createCommandBuffers() {
  VkCommandBufferAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  ai.commandPool = m_cmdPool;
  ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  ai.commandBufferCount = FRAME_COUNT;

  VkResult r = vkAllocateCommandBuffers(m_device, &ai, m_cmd.data());
  if (r != VK_SUCCESS) {
    std::cerr << "vkAllocateCommandBuffers failed: " << (int)r << std::endl;
    std::abort();
  }
}

void Renderer::createSyncObjects() {
  VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fci.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so first frame
                                            // doesn't wait forever

  for (int i = 0; i < FRAME_COUNT; ++i) {
    if (vkCreateSemaphore(m_device, &sci, nullptr, &m_imageAvailable[i]) !=
        VK_SUCCESS) {
      std::cerr << "Could not create image semaphore" << std::endl;
      std::abort();
    }
    if (vkCreateSemaphore(m_device, &sci, nullptr, &m_renderFinished[i]) !=
        VK_SUCCESS) {
      std::cerr << "Could not create render finished semaphore" << std::endl;
      std::abort();
    }
    if (vkCreateFence(m_device, &fci, nullptr, &m_inFlight[i]) != VK_SUCCESS) {
      std::cerr << "Could not create fence" << std::endl;
      std::abort();
    }
  }
}

void Renderer::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex,
                                   Scene *scene) {

  auto pipelineLayout = *scene->pipelineLayout();
  auto pipeline = *scene->pipeline();
  auto descriptorSets = *scene->descriptorSets();

  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkBeginCommandBuffer(cmd, &bi);

  VkRenderPassBeginInfo renderPassBeginInfo{};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = m_renderPass;
  renderPassBeginInfo.framebuffer = m_framebuffers[imageIndex];
  renderPassBeginInfo.renderArea.offset = {0, 0};
  renderPassBeginInfo.renderArea.extent = m_swapchainExtent;

  if (m_sampleCount == VK_SAMPLE_COUNT_1_BIT) {
    VkClearValue clears[2]{};

    // A pleasant “evergreen-ish” clear.
    clears[0].color.float32[0] = m_clearColor[0];
    clears[0].color.float32[1] = m_clearColor[1];
    clears[0].color.float32[2] = m_clearColor[2];
    clears[0].color.float32[3] = 1.0f;

    clears[1].depthStencil.depth = 1.0f;
    clears[1].depthStencil.stencil = 0;

    renderPassBeginInfo.pClearValues = clears;
    renderPassBeginInfo.clearValueCount = 2;
  } else {
    VkClearValue clears[3]{};

    // A pleasant “evergreen-ish” clear.
    clears[0].color.float32[0] = m_clearColor[0];
    clears[0].color.float32[1] = m_clearColor[1];
    clears[0].color.float32[2] = m_clearColor[2];
    clears[0].color.float32[3] = 1.0f;

    clears[2].depthStencil.depth = 1.0f;
    clears[2].depthStencil.stencil = 0;

    renderPassBeginInfo.pClearValues = clears;
    renderPassBeginInfo.clearValueCount = 3;
  }

  vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)m_swapchainExtent.width;
  viewport.height = (float)m_swapchainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(cmd, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = m_swapchainExtent;
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          0, 1, &descriptorSets[m_frameIndex], 0, nullptr);

  Mat4 model = Mat4::identity(); // MUST be initialized
  vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(Mat4), &model);

  VkDeviceSize off = 0;

  vkCmdBindVertexBuffers(cmd, 0, 1, scene->vertexBuffer(), &off);
  vkCmdBindIndexBuffer(cmd, *scene->indexBuffer(), 0, VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexed(cmd, *scene->indexCount(), 1, 0, 0, 0);

  vkCmdEndRenderPass(cmd);

  vkEndCommandBuffer(cmd);
}

void Renderer::waitDeviceIdle() {
  if (m_device)
    vkDeviceWaitIdle(m_device);
}

void Renderer::recreateSwapchainIfNeeded(Scene *scene) {
  if (!m_swapchainDirty) {
    return;
  }

  if (m_width <= 0 || m_height <= 0) {
    return;
  }

  waitDeviceIdle();

  // Pipeline depends on renderpass, which depends on swapchain format.
  scene->destroyPipeline(*this);
  destroySwapchain();

  createSwapchain(m_width, m_height);
  createSwapchainViews();
  createRenderPass();
  createColorResources();
  createDepthResources();
  createFramebuffers();
  scene->createPipeline(*this);

  m_swapchainDirty = false;
}

void Renderer::destroySwapchain() {
  destroyColorResources();
  destroyDepthResources();

  for (auto fb : m_framebuffers) {
    vkDestroyFramebuffer(m_device, fb, nullptr);
  }

  m_framebuffers.clear();

  if (m_renderPass) {
    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
    m_renderPass = VK_NULL_HANDLE;
  }

  for (auto iv : m_swapchainImageViews) {
    vkDestroyImageView(m_device, iv, nullptr);
  }

  m_swapchainImageViews.clear();
  m_swapchainImages.clear();

  if (m_swapchain) {
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
    m_swapchain = VK_NULL_HANDLE;
  }
}

void Renderer::destroyColorResources() {
  if (!m_device) {
    return;
  }

  if (m_colorView) {
    vkDestroyImageView(m_device, m_colorView, nullptr);
    m_colorView = VK_NULL_HANDLE;
  }
  if (m_colorImage) {
    vkDestroyImage(m_device, m_colorImage, nullptr);
    m_colorImage = VK_NULL_HANDLE;
  }
  if (m_colorMemory) {
    vkFreeMemory(m_device, m_colorMemory, nullptr);
    m_colorMemory = VK_NULL_HANDLE;
  }
}

void Renderer::destroyDepthResources() {
  if (!m_device) {
    return;
  }

  if (m_depthView) {
    vkDestroyImageView(m_device, m_depthView, nullptr);
    m_depthView = VK_NULL_HANDLE;
  }
  if (m_depthImage) {
    vkDestroyImage(m_device, m_depthImage, nullptr);
    m_depthImage = VK_NULL_HANDLE;
  }
  if (m_depthMemory) {
    vkFreeMemory(m_device, m_depthMemory, nullptr);
    m_depthMemory = VK_NULL_HANDLE;
  }
}

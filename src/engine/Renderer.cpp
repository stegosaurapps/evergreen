#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Renderer.hpp"

#include <windows.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <optional>
#include <set>

static const char *kValidationLayer = "VK_LAYER_KHRONOS_validation";

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *data, void *userData) {
  (void)type;
  (void)userData;

  // Keep it readable: only print warnings/errors by default.
  if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "[Vulkan] "
              << (data && data->pMessage ? data->pMessage : "(no message)")
              << "\n";
  }

  return VK_FALSE;
}

static bool CheckInstanceLayerAvailable(const char *name) {
  uint32_t count = 0;
  vkEnumerateInstanceLayerProperties(&count, nullptr);

  std::vector<VkLayerProperties> layers(count);
  vkEnumerateInstanceLayerProperties(&count, layers.data());
  for (auto &l : layers) {
    if (std::strcmp(l.layerName, name) == 0)
      return true;
  }

  return false;
}

static bool CheckInstanceExtensionAvailable(const char *name) {
  uint32_t count = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

  std::vector<VkExtensionProperties> exts(count);
  vkEnumerateInstanceExtensionProperties(nullptr, &count, exts.data());
  for (auto &e : exts) {
    if (std::strcmp(e.extensionName, name) == 0)
      return true;
  }

  return false;
}

static bool
CheckDeviceExtensionSupport(VkPhysicalDevice pd,
                            const std::vector<const char *> &required) {
  uint32_t count = 0;
  vkEnumerateDeviceExtensionProperties(pd, nullptr, &count, nullptr);

  std::vector<VkExtensionProperties> exts(count);
  vkEnumerateDeviceExtensionProperties(pd, nullptr, &count, exts.data());

  std::set<std::string> missing;
  for (auto *r : required) {
    missing.insert(r);
  }

  for (auto &e : exts) {
    missing.erase(e.extensionName);
  }

  return missing.empty();
}

struct SwapchainSupport {
  VkSurfaceCapabilitiesKHR caps{};
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

static SwapchainSupport QuerySwapchainSupport(VkPhysicalDevice pd,
                                              VkSurfaceKHR surface) {
  SwapchainSupport s{};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &s.caps);

  uint32_t fCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &fCount, nullptr);
  s.formats.resize(fCount);
  if (fCount) {
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surface, &fCount,
                                         s.formats.data());
  }

  uint32_t pCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &pCount, nullptr);
  s.presentModes.resize(pCount);
  if (pCount) {
    vkGetPhysicalDeviceSurfacePresentModesKHR(pd, surface, &pCount,
                                              s.presentModes.data());
  }

  return s;
}

static VkSurfaceFormatKHR
ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats) {
  // Prefer SRGB if available.
  for (auto &f : formats) {
    if (f.format == VK_FORMAT_B8G8R8A8_SRGB &&
        f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return f;
    }
  }

  // Else: first available.
  return formats.empty() ? VkSurfaceFormatKHR{VK_FORMAT_B8G8R8A8_UNORM,
                                              VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}
                         : formats[0];
}

static VkPresentModeKHR
ChoosePresentMode(const std::vector<VkPresentModeKHR> &modes) {
  // Mailbox feels great if available; else FIFO (guaranteed).
  for (auto m : modes) {
    if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
      return m;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR &caps, int w,
                               int h) {
  if (caps.currentExtent.width != UINT32_MAX) {
    return caps.currentExtent;
  }

  VkExtent2D e{};
  e.width = (uint32_t)std::clamp(w, (int)caps.minImageExtent.width,
                                 (int)caps.maxImageExtent.width);
  e.height = (uint32_t)std::clamp(h, (int)caps.minImageExtent.height,
                                  (int)caps.maxImageExtent.height);

  return e;
}

Renderer::~Renderer() { shutdown(); }

bool Renderer::init(const Win32WindowHandles &wh, int width, int height,
                    bool enableValidation) {
  m_enableValidation = enableValidation;
  m_width = width;
  m_height = height;

  // Instance, surface and device setup
  if (!createInstance()) {
    return false;
  }
  if (!setupDebug()) {
    return false;
  }
  if (!createSurface(wh)) {
    return false;
  }
  if (!pickPhysicalDevice()) {
    return false;
  }
  if (!createDevice()) {
    return false;
  }

  // Swapchain, render pass and frame buffer setup
  if (!createSwapchain(width, height)) {
    return false;
  }
  if (!createSwapchainViews()) {
    return false;
  }
  if (!createRenderPass()) {
    return false;
  }
  if (!createFramebuffers()) {
    return false;
  }

  // Command pool, command buffers and sync object setup
  if (!createCommandPool()) {
    return false;
  }
  if (!createCommandBuffers()) {
    return false;
  }
  if (!createSyncObjects()) {
    return false;
  }

  std::cout << "Renderer init OK.\n";

  return true;
}

void Renderer::shutdown() {
  if (!m_device && !m_instance) {
    return; // already down
  }

  waitDeviceIdle();

  // Per-frame sync
  for (int i = 0; i < kFramesInFlight; ++i) {
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

void Renderer::onResize(int width, int height) {
  m_width = width;
  m_height = height;
  m_swapchainDirty = true;
}

void Renderer::waitDeviceIdle() {
  if (m_device)
    vkDeviceWaitIdle(m_device);
}

bool Renderer::createInstance() {
  // Required instance extensions for Win32 surface.
  std::vector<const char *> exts = {VK_KHR_SURFACE_EXTENSION_NAME,
                                    VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

  if (m_enableValidation) {
    if (!CheckInstanceExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
      std::cerr << "Missing instance extension: "
                << VK_EXT_DEBUG_UTILS_EXTENSION_NAME << "\n";

      return false;
    }

    exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    if (!CheckInstanceLayerAvailable(kValidationLayer)) {
      std::cerr << "Validation requested but layer not available: "
                << kValidationLayer << "\n";

      return false;
    }
  }

  VkApplicationInfo app{};
  app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app.pApplicationName = "evergreen";
  app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app.pEngineName = "evergreen";
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
    std::cerr << "vkCreateInstance failed: " << (int)r << "\n";
    return false;
  }

  return true;
}

bool Renderer::setupDebug() {
  if (!m_enableValidation) {
    return true;
  }

  auto pfnCreate = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      m_instance, "vkCreateDebugUtilsMessengerEXT");

  if (!pfnCreate) {
    std::cerr << "vkCreateDebugUtilsMessengerEXT not found.\n";

    return false;
  }

  VkDebugUtilsMessengerCreateInfoEXT dci{};
  dci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  dci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  dci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  dci.pfnUserCallback = DebugCallback;

  VkResult r = pfnCreate(m_instance, &dci, nullptr, &m_debugMessenger);
  if (r != VK_SUCCESS) {
    std::cerr << "CreateDebugUtilsMessenger failed: " << (int)r << "\n";
    return false;
  }

  return true;
}

bool Renderer::createSurface(const Win32WindowHandles &wh) {
  if (!wh.hwnd || !wh.hinstance) {
    std::cerr << "createSurface: invalid window handles.\n";

    return false;
  }

  VkWin32SurfaceCreateInfoKHR sci{};
  sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
  sci.hwnd = (HWND)wh.hwnd;
  sci.hinstance = (HINSTANCE)wh.hinstance;

  VkResult r = vkCreateWin32SurfaceKHR(m_instance, &sci, nullptr, &m_surface);
  if (r != VK_SUCCESS || !m_surface) {
    std::cerr << "vkCreateWin32SurfaceKHR failed: " << (int)r << "\n";

    return false;
  }

  return true;
}

bool Renderer::pickPhysicalDevice() {
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
  if (!count) {
    std::cerr << "No Vulkan physical devices found.\n";

    return false;
  }

  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

  const std::vector<const char *> requiredDevExts = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  auto scoreDevice = [&](VkPhysicalDevice pd, uint32_t &outGfx,
                         uint32_t &outPresent) -> int {
    // Must support swapchain extension.
    if (!CheckDeviceExtensionSupport(pd, requiredDevExts))
      return -1;

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
      return -1;
    }

    // Swapchain must have at least one format + present mode.
    auto sup = QuerySwapchainSupport(pd, m_surface);
    if (sup.formats.empty() || sup.presentModes.empty()) {
      return -1;
    }

    VkPhysicalDeviceProperties props{};
    vkGetPhysicalDeviceProperties(pd, &props);

    int score = 0;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      score += 1000;
    }
    score += (int)props.limits.maxImageDimension2D;

    outGfx = *gfx;
    outPresent = *present;

    return score;
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

    return false;
  }

  m_physicalDevice = best;
  m_graphicsFamily = bestG;
  m_presentFamily = bestP;

  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(m_physicalDevice, &props);
  std::cout << "Using GPU: " << props.deviceName << "\n";
  std::cout << "Queue families: graphics=" << m_graphicsFamily
            << " present=" << m_presentFamily << "\n";

  return true;
}

bool Renderer::createDevice() {
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
    std::cerr << "vkCreateDevice failed: " << (int)r << "\n";
    return false;
  }

  vkGetDeviceQueue(m_device, m_graphicsFamily, 0, &m_graphicsQueue);
  vkGetDeviceQueue(m_device, m_presentFamily, 0, &m_presentQueue);

  return true;
}

bool Renderer::createSwapchain(int width, int height) {
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
    std::cerr << "vkCreateSwapchainKHR failed: " << (int)r << "\n";

    return false;
  }

  m_swapchainFormat = fmt.format;
  m_swapchainExtent = extent;

  uint32_t scCount = 0;
  vkGetSwapchainImagesKHR(m_device, m_swapchain, &scCount, nullptr);
  m_swapchainImages.resize(scCount);
  vkGetSwapchainImagesKHR(m_device, m_swapchain, &scCount,
                          m_swapchainImages.data());

  return true;
}

bool Renderer::createSwapchainViews() {
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
      std::cerr << "vkCreateImageView failed: " << (int)r << "\n";

      return false;
    }
  }

  return true;
}

bool Renderer::createRenderPass() {
  VkAttachmentDescription color{};
  color.format = m_swapchainFormat;
  color.samples = VK_SAMPLE_COUNT_1_BIT;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorRef{};
  colorRef.attachment = 0;
  colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;

  // Basic dependency for layout transitions.
  VkSubpassDependency dep{};
  dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  dep.dstSubpass = 0;
  dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dep.srcAccessMask = 0;
  dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo rp{};
  rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rp.attachmentCount = 1;
  rp.pAttachments = &color;
  rp.subpassCount = 1;
  rp.pSubpasses = &subpass;
  rp.dependencyCount = 1;
  rp.pDependencies = &dep;

  VkResult r = vkCreateRenderPass(m_device, &rp, nullptr, &m_renderPass);
  if (r != VK_SUCCESS) {
    std::cerr << "vkCreateRenderPass failed: " << (int)r << "\n";

    return false;
  }

  return true;
}

bool Renderer::createFramebuffers() {
  m_framebuffers.resize(m_swapchainImageViews.size());

  for (size_t i = 0; i < m_swapchainImageViews.size(); ++i) {
    VkImageView attachments[] = {m_swapchainImageViews[i]};

    VkFramebufferCreateInfo fb{};
    fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb.renderPass = m_renderPass;
    fb.attachmentCount = 1;
    fb.pAttachments = attachments;
    fb.width = m_swapchainExtent.width;
    fb.height = m_swapchainExtent.height;
    fb.layers = 1;

    VkResult r =
        vkCreateFramebuffer(m_device, &fb, nullptr, &m_framebuffers[i]);
    if (r != VK_SUCCESS) {
      std::cerr << "vkCreateFramebuffer failed: " << (int)r << "\n";

      return false;
    }
  }

  return true;
}

bool Renderer::createCommandPool() {
  VkCommandPoolCreateInfo ci{};
  ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  ci.queueFamilyIndex = m_graphicsFamily;
  ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VkResult r = vkCreateCommandPool(m_device, &ci, nullptr, &m_cmdPool);
  if (r != VK_SUCCESS) {
    std::cerr << "vkCreateCommandPool failed: " << (int)r << "\n";

    return false;
  }

  return true;
}

bool Renderer::createCommandBuffers() {
  VkCommandBufferAllocateInfo ai{};
  ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  ai.commandPool = m_cmdPool;
  ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  ai.commandBufferCount = kFramesInFlight;

  VkResult r = vkAllocateCommandBuffers(m_device, &ai, m_cmd.data());
  if (r != VK_SUCCESS) {
    std::cerr << "vkAllocateCommandBuffers failed: " << (int)r << "\n";

    return false;
  }

  return true;
}

bool Renderer::createSyncObjects() {
  VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fci.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled so first frame
                                            // doesn't wait forever

  for (int i = 0; i < kFramesInFlight; ++i) {
    if (vkCreateSemaphore(m_device, &sci, nullptr, &m_imageAvailable[i]) !=
        VK_SUCCESS) {
      return false;
    }
    if (vkCreateSemaphore(m_device, &sci, nullptr, &m_renderFinished[i]) !=
        VK_SUCCESS) {
      return false;
    }
    if (vkCreateFence(m_device, &fci, nullptr, &m_inFlight[i]) != VK_SUCCESS) {
      return false;
    }
  }

  return true;
}

void Renderer::destroySwapchain() {
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

bool Renderer::recreateSwapchainIfNeeded() {
  if (!m_swapchainDirty) {
    return true;
  }

  if (m_width <= 0 || m_height <= 0) {
    return true; // minimized; skip
  }

  waitDeviceIdle();
  destroySwapchain();

  if (!createSwapchain(m_width, m_height)) {
    return false;
  }
  if (!createSwapchainViews()) {
    return false;
  }
  if (!createRenderPass()) {
    return false;
  }
  if (!createFramebuffers()) {
    return false;
  }

  m_swapchainDirty = false;

  return true;
}

void Renderer::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {
  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkBeginCommandBuffer(cmd, &bi);

  VkClearValue clear{};
  // A pleasant “evergreen-ish” clear.
  clear.color.float32[0] = 0.10f;
  clear.color.float32[1] = 0.16f;
  clear.color.float32[2] = 0.18f;
  clear.color.float32[3] = 1.0f;

  VkRenderPassBeginInfo rp{};
  rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp.renderPass = m_renderPass;
  rp.framebuffer = m_framebuffers[imageIndex];
  rp.renderArea.offset = {0, 0};
  rp.renderArea.extent = m_swapchainExtent;
  rp.clearValueCount = 1;
  rp.pClearValues = &clear;

  vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdEndRenderPass(cmd);

  vkEndCommandBuffer(cmd);
}

void Renderer::drawFrame() {
  if (!m_device) {
    return;
  }

  if (!recreateSwapchainIfNeeded()) {
    std::cerr << "Swapchain recreation failed.\n";

    return;
  }

  const int fi = m_frameIndex;

  vkWaitForFences(m_device, 1, &m_inFlight[fi], VK_TRUE, UINT64_MAX);
  vkResetFences(m_device, 1, &m_inFlight[fi]);

  uint32_t imageIndex = 0;
  VkResult r =
      vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX,
                            m_imageAvailable[fi], VK_NULL_HANDLE, &imageIndex);

  if (r == VK_ERROR_OUT_OF_DATE_KHR) {
    m_swapchainDirty = true;
    return;
  }
  if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
    std::cerr << "vkAcquireNextImageKHR failed: " << (int)r << "\n";
    return;
  }

  vkResetCommandBuffer(m_cmd[fi], 0);
  recordCommandBuffer(m_cmd[fi], imageIndex);

  VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo si{};
  si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  si.waitSemaphoreCount = 1;
  si.pWaitSemaphores = &m_imageAvailable[fi];
  si.pWaitDstStageMask = &waitStage;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &m_cmd[fi];
  si.signalSemaphoreCount = 1;
  si.pSignalSemaphores = &m_renderFinished[fi];

  r = vkQueueSubmit(m_graphicsQueue, 1, &si, m_inFlight[fi]);
  if (r != VK_SUCCESS) {
    std::cerr << "vkQueueSubmit failed: " << (int)r << "\n";
    return;
  }

  VkPresentInfoKHR pi{};
  pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  pi.waitSemaphoreCount = 1;
  pi.pWaitSemaphores = &m_renderFinished[fi];
  pi.swapchainCount = 1;
  pi.pSwapchains = &m_swapchain;
  pi.pImageIndices = &imageIndex;

  r = vkQueuePresentKHR(m_presentQueue, &pi);
  if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) {
    m_swapchainDirty = true;
  } else if (r != VK_SUCCESS) {
    std::cerr << "vkQueuePresentKHR failed: " << (int)r << "\n";
  }

  m_frameIndex = (m_frameIndex + 1) % kFramesInFlight;
}

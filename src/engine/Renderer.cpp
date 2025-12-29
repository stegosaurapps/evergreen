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
#include <fstream>
#include <iostream>
#include <optional>
#include <set>

// ----- tiny helpers -----

static uint32_t FindMemoryType(VkPhysicalDevice pd, uint32_t typeFilter,
                               VkMemoryPropertyFlags props) {
  VkPhysicalDeviceMemoryProperties mp{};
  vkGetPhysicalDeviceMemoryProperties(pd, &mp);

  for (uint32_t i = 0; i < mp.memoryTypeCount; ++i) {
    if ((typeFilter & (1u << i)) &&
        (mp.memoryTypes[i].propertyFlags & props) == props) {
      return i;
    }
  }
  return UINT32_MAX;
}

static bool CreateBuffer(VkPhysicalDevice pd, VkDevice dev, VkDeviceSize size,
                         VkBufferUsageFlags usage,
                         VkMemoryPropertyFlags memProps, VkBuffer &outBuf,
                         VkDeviceMemory &outMem) {
  VkBufferCreateInfo ci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  ci.size = size;
  ci.usage = usage;
  ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(dev, &ci, nullptr, &outBuf) != VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements req{};
  vkGetBufferMemoryRequirements(dev, outBuf, &req);

  uint32_t memType = FindMemoryType(pd, req.memoryTypeBits, memProps);
  if (memType == UINT32_MAX) {
    return false;
  }

  VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  ai.allocationSize = req.size;
  ai.memoryTypeIndex = memType;
  if (vkAllocateMemory(dev, &ai, nullptr, &outMem) != VK_SUCCESS) {
    return false;
  }

  vkBindBufferMemory(dev, outBuf, outMem, 0);
  return true;
}

static bool CreateImage2D(VkPhysicalDevice pd, VkDevice dev, uint32_t width,
                          uint32_t height, VkFormat fmt,
                          VkImageUsageFlags usage, VkImage &outImg,
                          VkDeviceMemory &outMem) {
  VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  ci.imageType = VK_IMAGE_TYPE_2D;
  ci.extent = {width, height, 1};
  ci.mipLevels = 1;
  ci.arrayLayers = 1;
  ci.format = fmt;
  ci.tiling = VK_IMAGE_TILING_OPTIMAL;
  ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  ci.usage = usage;
  ci.samples = VK_SAMPLE_COUNT_1_BIT;
  ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(dev, &ci, nullptr, &outImg) != VK_SUCCESS) {
    return false;
  }

  VkMemoryRequirements req{};
  vkGetImageMemoryRequirements(dev, outImg, &req);
  uint32_t memType = FindMemoryType(pd, req.memoryTypeBits,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (memType == UINT32_MAX) {
    return false;
  }

  VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  ai.allocationSize = req.size;
  ai.memoryTypeIndex = memType;
  if (vkAllocateMemory(dev, &ai, nullptr, &outMem) != VK_SUCCESS) {
    return false;
  }

  vkBindImageMemory(dev, outImg, outMem, 0);
  return true;
}

static VkImageView CreateImageView(VkDevice dev, VkImage img, VkFormat fmt,
                                   VkImageAspectFlags aspect) {
  VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  iv.image = img;
  iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
  iv.format = fmt;
  iv.subresourceRange.aspectMask = aspect;
  iv.subresourceRange.baseMipLevel = 0;
  iv.subresourceRange.levelCount = 1;
  iv.subresourceRange.baseArrayLayer = 0;
  iv.subresourceRange.layerCount = 1;

  VkImageView out = VK_NULL_HANDLE;
  if (vkCreateImageView(dev, &iv, nullptr, &out) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  return out;
}

static bool ReadFileBytes(const char *path, std::vector<char> &out) {
  std::ifstream f(path, std::ios::ate | std::ios::binary);
  if (!f.is_open()) {
    return false;
  }
  size_t size = (size_t)f.tellg();
  out.resize(size);
  f.seekg(0);
  f.read(out.data(), size);
  f.close();
  return true;
}

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
  if (!createDepthResources()) {
    return false;
  }
  if (!createFramebuffers()) {
    return false;
  }

  // Demo resources (camera + one pipeline + one mesh)
  if (!createDescriptorSetLayout()) {
    return false;
  }
  if (!createDescriptorPool()) {
    return false;
  }
  if (!createUniformBuffers()) {
    return false;
  }
  if (!createPipeline()) {
    return false;
  }
  if (!createMeshBuffers()) {
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

  destroyDeviceResources();

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
  m_camera.onResize(width, height);
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

bool Renderer::createDepthResources() {
  if (m_depthView) {
    destroyDepthResources();
  }

  if (!CreateImage2D(m_physicalDevice, m_device, m_swapchainExtent.width,
                     m_swapchainExtent.height, m_depthFormat,
                     VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, m_depthImage,
                     m_depthMemory)) {
    std::cerr << "Failed to create depth image\n";
    return false;
  }

  m_depthView = CreateImageView(m_device, m_depthImage, m_depthFormat,
                                VK_IMAGE_ASPECT_DEPTH_BIT);
  if (!m_depthView) {
    std::cerr << "Failed to create depth image view\n";
    return false;
  }

  return true;
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

  VkAttachmentDescription depth{};
  depth.format = m_depthFormat;
  depth.samples = VK_SAMPLE_COUNT_1_BIT;
  depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthRef{};
  depthRef.attachment = 1;
  depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;

  // Basic dependency for layout transitions.
  VkSubpassDependency dep{};
  dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  dep.dstSubpass = 0;
  dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                     VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                     VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dep.srcAccessMask = 0;
  dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo rp{};
  rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  VkAttachmentDescription atts[2] = {color, depth};
  rp.attachmentCount = 2;
  rp.pAttachments = atts;
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
    VkImageView attachments[] = {m_swapchainImageViews[i], m_depthView};

    VkFramebufferCreateInfo fb{};
    fb.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb.renderPass = m_renderPass;
    fb.attachmentCount = 2;
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

bool Renderer::createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding cam{};
  cam.binding = 0;
  cam.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  cam.descriptorCount = 1;
  cam.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo ci{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  ci.bindingCount = 1;
  ci.pBindings = &cam;

  if (vkCreateDescriptorSetLayout(m_device, &ci, nullptr, &m_setLayoutFrame) !=
      VK_SUCCESS) {
    std::cerr << "vkCreateDescriptorSetLayout failed\n";
    return false;
  }
  return true;
}

bool Renderer::createDescriptorPool() {
  VkDescriptorPoolSize size{};
  size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  size.descriptorCount = kFramesInFlight;

  VkDescriptorPoolCreateInfo ci{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  ci.maxSets = kFramesInFlight;
  ci.poolSizeCount = 1;
  ci.pPoolSizes = &size;

  if (vkCreateDescriptorPool(m_device, &ci, nullptr, &m_descPool) !=
      VK_SUCCESS) {
    std::cerr << "vkCreateDescriptorPool failed\n";
    return false;
  }

  std::array<VkDescriptorSetLayout, kFramesInFlight> layouts;
  for (int i = 0; i < kFramesInFlight; ++i) {
    layouts[i] = m_setLayoutFrame;
  }

  VkDescriptorSetAllocateInfo ai{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  ai.descriptorPool = m_descPool;
  ai.descriptorSetCount = kFramesInFlight;
  ai.pSetLayouts = layouts.data();

  if (vkAllocateDescriptorSets(m_device, &ai, m_descSetFrame.data()) !=
      VK_SUCCESS) {
    std::cerr << "vkAllocateDescriptorSets failed\n";
    return false;
  }

  return true;
}

bool Renderer::createUniformBuffers() {
  for (int i = 0; i < kFramesInFlight; ++i) {
    if (!CreateBuffer(m_physicalDevice, m_device, sizeof(CameraUBO),
                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      m_uboBuffer[i], m_uboMemory[i])) {
      std::cerr << "Failed to create uniform buffer\n";
      return false;
    }

    if (vkMapMemory(m_device, m_uboMemory[i], 0, sizeof(CameraUBO), 0,
                    &m_uboMapped[i]) != VK_SUCCESS) {
      std::cerr << "vkMapMemory failed for UBO\n";
      return false;
    }

    VkDescriptorBufferInfo bi{};
    bi.buffer = m_uboBuffer[i];
    bi.offset = 0;
    bi.range = sizeof(CameraUBO);

    VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    w.dstSet = m_descSetFrame[i];
    w.dstBinding = 0;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w.pBufferInfo = &bi;

    vkUpdateDescriptorSets(m_device, 1, &w, 0, nullptr);
  }

  // Initialize camera.
  m_camera.setPerspective(1.04719755f, (float)m_width / (float)m_height, 0.1f,
                          200.0f);
  m_camera.lookAt({0.0f, 0.0f, 0.0f});
  m_camera.updateMatrices();
  return true;
}

static VkShaderModule CreateShaderModule(VkDevice dev,
                                         const std::vector<char> &bytes) {
  VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  ci.codeSize = bytes.size();
  ci.pCode = reinterpret_cast<const uint32_t *>(bytes.data());
  VkShaderModule mod = VK_NULL_HANDLE;
  if (vkCreateShaderModule(dev, &ci, nullptr, &mod) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }
  return mod;
}

bool Renderer::createPipeline() {
  // Destroy old pipeline if rebuilding (swapchain resize).
  if (m_pipeline) {
    destroyPipelineResources();
  }

  std::vector<char> vsBytes;
  std::vector<char> fsBytes;
  if (!ReadFileBytes("shaders/pbr.vert.spv", vsBytes) ||
      !ReadFileBytes("shaders/pbr.frag.spv", fsBytes)) {
    std::cerr << "Missing shaders. Build will generate shaders/pbr.*.spv (via "
                 "glslc).\n";
    return false;
  }

  VkShaderModule vs = CreateShaderModule(m_device, vsBytes);
  VkShaderModule fs = CreateShaderModule(m_device, fsBytes);
  if (!vs || !fs) {
    std::cerr << "Failed to create shader modules\n";
    return false;
  }

  VkPipelineShaderStageCreateInfo stages[2]{};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vs;
  stages[0].pName = "main";
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = fs;
  stages[1].pName = "main";

  // Vertex input
  VkVertexInputBindingDescription bind{};
  bind.binding = 0;
  bind.stride = sizeof(Vertex);
  bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attrs[3]{};
  // position
  attrs[0].location = 0;
  attrs[0].binding = 0;
  attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrs[0].offset = offsetof(Vertex, px);
  // normal
  attrs[1].location = 1;
  attrs[1].binding = 0;
  attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrs[1].offset = offsetof(Vertex, nx);
  // uv
  attrs[2].location = 2;
  attrs[2].binding = 0;
  attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
  attrs[2].offset = offsetof(Vertex, ux);

  VkPipelineVertexInputStateCreateInfo vi{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vi.vertexBindingDescriptionCount = 1;
  vi.pVertexBindingDescriptions = &bind;
  vi.vertexAttributeDescriptionCount = 3;
  vi.pVertexAttributeDescriptions = attrs;

  VkPipelineInputAssemblyStateCreateInfo ia{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  ia.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo vp{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  vp.viewportCount = 1;
  vp.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rs{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rs.polygonMode = VK_POLYGON_MODE_FILL;
  rs.cullMode = VK_CULL_MODE_BACK_BIT;
  rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rs.lineWidth = 1.0f;

  VkPipelineMultisampleStateCreateInfo ms{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo ds{
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  ds.depthTestEnable = VK_TRUE;
  ds.depthWriteEnable = VK_TRUE;
  ds.depthCompareOp = VK_COMPARE_OP_LESS;

  VkPipelineColorBlendAttachmentState cba{};
  cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  cba.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo cb{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  cb.attachmentCount = 1;
  cb.pAttachments = &cba;

  VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dyn{
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dyn.dynamicStateCount = 2;
  dyn.pDynamicStates = dynStates;

  // Push constant = model matrix.
  VkPushConstantRange pcr{};
  pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pcr.offset = 0;
  pcr.size = sizeof(Mat4);

  if (m_setLayoutFrame == VK_NULL_HANDLE) {
    std::cerr << "createPipeline: m_setLayoutFrame is VK_NULL_HANDLE "
                 "(descriptor set layout missing)\n";
    return false;
  }

  VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pl.setLayoutCount = 1;
  pl.pSetLayouts = &m_setLayoutFrame;
  pl.pushConstantRangeCount = 1;
  pl.pPushConstantRanges = &pcr;

  if (vkCreatePipelineLayout(m_device, &pl, nullptr, &m_pipelineLayout) !=
      VK_SUCCESS) {
    std::cerr << "vkCreatePipelineLayout failed\n";
    vkDestroyShaderModule(m_device, vs, nullptr);
    vkDestroyShaderModule(m_device, fs, nullptr);
    return false;
  }

  VkPipelineRenderingCreateInfoKHR rendering{};
  // Using classic render pass, so no dynamic rendering here.

  VkGraphicsPipelineCreateInfo pci{};
  pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pci.pNext = nullptr;

  pci.stageCount = 2;
  pci.pStages = stages;

  pci.pVertexInputState = &vi;
  pci.pInputAssemblyState = &ia;
  pci.pViewportState = &vp;
  pci.pRasterizationState = &rs;
  pci.pMultisampleState = &ms;
  pci.pDepthStencilState = &ds;
  pci.pColorBlendState = &cb;
  pci.pDynamicState = &dyn;

  pci.layout = m_pipelineLayout;
  pci.renderPass = m_renderPass;
  pci.subpass = 0;

  // Optional but commonly set:
  pci.basePipelineHandle = VK_NULL_HANDLE;
  pci.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pci, nullptr,
                                &m_pipeline) != VK_SUCCESS) {
    std::cerr << "vkCreateGraphicsPipelines failed\n";
    vkDestroyShaderModule(m_device, vs, nullptr);
    vkDestroyShaderModule(m_device, fs, nullptr);
    return false;
  }

  vkDestroyShaderModule(m_device, vs, nullptr);
  vkDestroyShaderModule(m_device, fs, nullptr);

  return true;
}

bool Renderer::createMeshBuffers() {
  // Build a single triangle in the scene. You'll replace this with a real model
  // loader.
  m_scene = Scene{};
  Renderable r{};
  r.mesh.vertices = {
      {0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f},
      {0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
      {-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
  };
  r.mesh.indices = {0, 1, 2};
  m_scene.add(std::move(r));

  const Mesh &m = m_scene.items()[0].mesh;
  m_indexCount = (uint32_t)m.indices.size();

  // Recreate buffers if they exist.
  if (m_vertexBuffer) {
    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexMemory, nullptr);
    m_vertexBuffer = VK_NULL_HANDLE;
    m_vertexMemory = VK_NULL_HANDLE;
  }
  if (m_indexBuffer) {
    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    vkFreeMemory(m_device, m_indexMemory, nullptr);
    m_indexBuffer = VK_NULL_HANDLE;
    m_indexMemory = VK_NULL_HANDLE;
  }

  VkDeviceSize vsize = sizeof(Vertex) * m.vertices.size();
  VkDeviceSize isize = sizeof(uint32_t) * m.indices.size();

  if (!CreateBuffer(m_physicalDevice, m_device, vsize,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    m_vertexBuffer, m_vertexMemory)) {
    return false;
  }
  if (!CreateBuffer(m_physicalDevice, m_device, isize,
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    m_indexBuffer, m_indexMemory)) {
    return false;
  }

  void *mapped = nullptr;
  vkMapMemory(m_device, m_vertexMemory, 0, vsize, 0, &mapped);
  std::memcpy(mapped, m.vertices.data(), (size_t)vsize);
  vkUnmapMemory(m_device, m_vertexMemory);

  vkMapMemory(m_device, m_indexMemory, 0, isize, 0, &mapped);
  std::memcpy(mapped, m.indices.data(), (size_t)isize);
  vkUnmapMemory(m_device, m_indexMemory);

  return true;
}

void Renderer::updatePerFrameUBO(int frameIndex) {
  // Keep camera current (aspect updates on resize handled in onResize).
  m_camera.updateMatrices();

  std::memcpy(m_uboMapped[frameIndex], &m_camera.ubo(), sizeof(CameraUBO));
}

void Renderer::destroyDeviceResources() {
  if (!m_device) {
    return;
  }

  // Mesh buffers
  if (m_vertexBuffer) {
    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    m_vertexBuffer = VK_NULL_HANDLE;
  }
  if (m_vertexMemory) {
    vkFreeMemory(m_device, m_vertexMemory, nullptr);
    m_vertexMemory = VK_NULL_HANDLE;
  }

  if (m_indexBuffer) {
    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    m_indexBuffer = VK_NULL_HANDLE;
  }
  if (m_indexMemory) {
    vkFreeMemory(m_device, m_indexMemory, nullptr);
    m_indexMemory = VK_NULL_HANDLE;
  }

  // Uniform buffers (per-frame)
  for (int i = 0; i < kFramesInFlight; ++i) {
    if (m_uboMapped[i]) {
      vkUnmapMemory(m_device, m_uboMemory[i]);
      m_uboMapped[i] = nullptr;
    }

    if (m_uboBuffer[i]) {
      vkDestroyBuffer(m_device, m_uboBuffer[i], nullptr);
      m_uboBuffer[i] = VK_NULL_HANDLE;
    }

    if (m_uboMemory[i]) {
      vkFreeMemory(m_device, m_uboMemory[i], nullptr);
      m_uboMemory[i] = VK_NULL_HANDLE;
    }
  }

  // Descriptor pool
  if (m_descPool) {
    vkDestroyDescriptorPool(m_device, m_descPool, nullptr);
    m_descPool = VK_NULL_HANDLE;
  }

  // Descriptor set layout
  if (m_setLayoutFrame) {
    vkDestroyDescriptorSetLayout(m_device, m_setLayoutFrame, nullptr);
    m_setLayoutFrame = VK_NULL_HANDLE;
  }
}

void Renderer::destroyPipelineResources() {
  if (!m_device) {
    return;
  }

  if (m_pipeline) {
    vkDestroyPipeline(m_device, m_pipeline, nullptr);
    m_pipeline = VK_NULL_HANDLE;
  }

  if (m_pipelineLayout) {
    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
    m_pipelineLayout = VK_NULL_HANDLE;
  }
}

void Renderer::destroySwapchain() {
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

bool Renderer::recreateSwapchainIfNeeded() {
  if (!m_swapchainDirty) {
    return true;
  }

  if (m_width <= 0 || m_height <= 0) {
    return true; // minimized; skip
  }

  waitDeviceIdle();

  // Pipeline depends on renderpass, which depends on swapchain format.
  destroyPipelineResources();
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

  if (!createDepthResources()) {
    return false;
  }

  if (!createFramebuffers()) {
    return false;
  }

  if (!createPipeline()) {
    return false;
  }

  m_swapchainDirty = false;

  return true;
}

void Renderer::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {
  VkCommandBufferBeginInfo bi{};
  bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkBeginCommandBuffer(cmd, &bi);

  VkClearValue clears[2]{};
  // A pleasant “evergreen-ish” clear.
  clears[0].color.float32[0] = 0.10f;
  clears[0].color.float32[1] = 0.16f;
  clears[0].color.float32[2] = 0.18f;
  clears[0].color.float32[3] = 1.0f;
  clears[1].depthStencil.depth = 1.0f;
  clears[1].depthStencil.stencil = 0;

  VkRenderPassBeginInfo rp{};
  rp.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp.renderPass = m_renderPass;
  rp.framebuffer = m_framebuffers[imageIndex];
  rp.renderArea.offset = {0, 0};
  rp.renderArea.extent = m_swapchainExtent;
  rp.clearValueCount = 2;
  rp.pClearValues = clears;

  vkCmdBeginRenderPass(cmd, &rp, VK_SUBPASS_CONTENTS_INLINE);

  // --- draw demo scene ---
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

  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          m_pipelineLayout, 0, 1, &m_descSetFrame[m_frameIndex],
                          0, nullptr);

  // Push model matrix for the first renderable.
  Mat4 model = Mat4::identity();
  if (!m_scene.items().empty()) {
    model = m_scene.items()[0].transform.matrix();
  }
  vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                     sizeof(Mat4), &model);

  VkDeviceSize off = 0;
  vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer, &off);
  vkCmdBindIndexBuffer(cmd, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdDrawIndexed(cmd, m_indexCount, 1, 0, 0, 0);

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

  // Update UBO for this frame-in-flight before recording.
  updatePerFrameUBO(fi);
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

#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <array>
#include <cstdint>

struct Win32WindowHandles {
    void* hwnd;       // HWND
    void* hinstance;  // HINSTANCE
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    bool init(const Win32WindowHandles& wh, int width, int height, bool enableValidation);
    void shutdown();

    void onResize(int width, int height);   // mark swapchain dirty
    void drawFrame();                       // present one frame

private:
    // --- high-level phases ---
    bool createInstance();
    bool setupDebug();
    bool createSurface(const Win32WindowHandles& wh);
    bool pickPhysicalDevice();
    bool createDevice();
    bool createSwapchain(int width, int height);
    bool createSwapchainViews();
    bool createRenderPass();
    bool createFramebuffers();
    bool createCommandPool();
    bool createCommandBuffers();
    bool createSyncObjects();

    void destroySwapchain();
    bool recreateSwapchainIfNeeded();

    // --- helpers ---
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    void waitDeviceIdle();

private:
    bool m_enableValidation = false;
    bool m_swapchainDirty = false;
    int  m_width = 0;
    int  m_height = 0;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;

    uint32_t m_graphicsFamily = UINT32_MAX;
    uint32_t m_presentFamily  = UINT32_MAX;
    VkQueue  m_graphicsQueue  = VK_NULL_HANDLE;
    VkQueue  m_presentQueue   = VK_NULL_HANDLE;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_swapchainFormat{};
    VkExtent2D m_swapchainExtent{};
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_framebuffers;

    VkCommandPool m_cmdPool = VK_NULL_HANDLE;

    static constexpr int kFramesInFlight = 2;
    int m_frameIndex = 0;
    std::array<VkCommandBuffer, kFramesInFlight> m_cmd{};
    std::array<VkSemaphore, kFramesInFlight> m_imageAvailable{};
    std::array<VkSemaphore, kFramesInFlight> m_renderFinished{};
    std::array<VkFence,     kFramesInFlight> m_inFlight{};
};

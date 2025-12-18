#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>

#include <vulkan/vulkan.h>

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

static const char* VkResultToString(VkResult r)
{
    switch (r) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    default: return "VK_RESULT_(unhandled)";
    }
}

static void DumpAvailableInstanceExtensions()
{
    uint32_t count = 0;
    VkResult r = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (r != VK_SUCCESS) {
        std::cout << "vkEnumerateInstanceExtensionProperties failed: "
                  << VkResultToString(r) << " (" << (int)r << ")\n";
        return;
    }

    std::vector<VkExtensionProperties> exts(count);
    r = vkEnumerateInstanceExtensionProperties(nullptr, &count, exts.data());
    if (r != VK_SUCCESS) {
        std::cout << "vkEnumerateInstanceExtensionProperties (2) failed: "
                  << VkResultToString(r) << " (" << (int)r << ")\n";
        return;
    }

    std::cout << "Available instance extensions (" << count << "):\n";
    for (const auto& e : exts) {
        std::cout << "  " << e.extensionName << " (spec " << e.specVersion << ")\n";
    }
}

static void DumpAvailableLayers()
{
    uint32_t count = 0;
    VkResult r = vkEnumerateInstanceLayerProperties(&count, nullptr);
    if (r != VK_SUCCESS) {
        std::cout << "vkEnumerateInstanceLayerProperties failed: "
                  << VkResultToString(r) << " (" << (int)r << ")\n";
        return;
    }

    std::vector<VkLayerProperties> layers(count);
    r = vkEnumerateInstanceLayerProperties(&count, layers.data());
    if (r != VK_SUCCESS) {
        std::cout << "vkEnumerateInstanceLayerProperties (2) failed: "
                  << VkResultToString(r) << " (" << (int)r << ")\n";
        return;
    }

    std::cout << "Available layers (" << count << "):\n";
    for (const auto& l : layers) {
        std::cout << "  " << l.layerName
                  << " (spec " << l.specVersion
                  << ", impl " << l.implementationVersion << ")\n";
    }
}

int main(int, char**)
{
    std::cout << "evergreen: SDL3 + Vulkan bootstrap\n";

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "evergreen",
        800, 600,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    DumpAvailableInstanceExtensions();
    DumpAvailableLayers();

    std::vector<const char*> instanceExts = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "evergreen";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "evergreen";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &appInfo;
    ci.enabledExtensionCount = (uint32_t)instanceExts.size();
    ci.ppEnabledExtensionNames = instanceExts.data();

    std::cout << "Creating VkInstance with extensions:\n";
    for (auto* e : instanceExts) std::cout << "  " << e << "\n";

    VkInstance instance = VK_NULL_HANDLE;
    VkResult r = vkCreateInstance(&ci, nullptr, &instance);
    std::cout << "vkCreateInstance -> " << VkResultToString(r) << " (" << (int)r << ")\n";

    if (r != VK_SUCCESS || instance == VK_NULL_HANDLE) {
        std::cerr << "vkCreateInstance failed.\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // --- SDL3 Win32 handles via properties ---
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    if (!props) {
        std::cerr << "SDL_GetWindowProperties failed: " << SDL_GetError() << "\n";
        vkDestroyInstance(instance, nullptr);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    HWND hwnd = (HWND)SDL_GetPointerProperty(props, "SDL.window.win32.hwnd", nullptr);
    HINSTANCE hinstance = (HINSTANCE)SDL_GetPointerProperty(props, "SDL.window.win32.hinstance", nullptr);

    // Fallbacks: SDL3 sometimes doesn't provide hinstance.
    if (hwnd && !hinstance) {
        hinstance = (HINSTANCE)(intptr_t)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    }
    if (!hinstance) {
        hinstance = GetModuleHandle(nullptr);
    }

    if (!hwnd || !hinstance) {
        std::cerr << "Failed to retrieve Win32 handles.\n";
        std::cerr << "hwnd=" << hwnd << " hinstance=" << hinstance << "\n";
        vkDestroyInstance(instance, nullptr);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "Win32 handles OK: hwnd=" << hwnd << " hinstance=" << hinstance << "\n";

    // --- Create Win32 Vulkan surface ---
    auto pfnCreateWin32Surface =
        (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

    if (!pfnCreateWin32Surface) {
        std::cerr << "vkCreateWin32SurfaceKHR not found.\n";
        vkDestroyInstance(instance, nullptr);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    VkWin32SurfaceCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    sci.hwnd = hwnd;
    sci.hinstance = hinstance;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    r = pfnCreateWin32Surface(instance, &sci, nullptr, &surface);
    std::cout << "vkCreateWin32SurfaceKHR -> " << VkResultToString(r) << " (" << (int)r << ")\n";

    if (r != VK_SUCCESS || surface == VK_NULL_HANDLE) {
        std::cerr << "Surface creation failed.\n";
        vkDestroyInstance(instance, nullptr);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    std::cout << "Success: instance + surface created. Entering loop...\n";

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) running = false;
        }
        SDL_Delay(16);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Clean shutdown.\n";
    return 0;
}

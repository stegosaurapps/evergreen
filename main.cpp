#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>

#include <windows.h>
#include <iostream>

#include "src/engine/rendering/renderer.hpp"

static Win32WindowHandles GetWin32Handles(SDL_Window* window)
{
    Win32WindowHandles wh{};
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    if (!props) {
        std::cerr << "SDL_GetWindowProperties failed: " << SDL_GetError() << "\n";
        return wh;
    }

    HWND hwnd = (HWND)SDL_GetPointerProperty(props, "SDL.window.win32.hwnd", nullptr);
    HINSTANCE hinstance = (HINSTANCE)SDL_GetPointerProperty(props, "SDL.window.win32.hinstance", nullptr);

    // SDL3 sometimes doesn't provide hinstance. Recover it.
    if (hwnd && !hinstance) {
        hinstance = (HINSTANCE)(intptr_t)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
    }
    if (!hinstance) {
        hinstance = GetModuleHandle(nullptr);
    }

    wh.hwnd = hwnd;
    wh.hinstance = hinstance;
    return wh;
}

int main(int, char**)
{
    std::cout << "evergreen: SDL3 + Renderer\n";

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    const int startW = 800;
    const int startH = 600;

    SDL_Window* window = SDL_CreateWindow(
        "evergreen",
        startW, startH,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    Win32WindowHandles wh = GetWin32Handles(window);
    if (!wh.hwnd || !wh.hinstance) {
        std::cerr << "Failed to retrieve Win32 handles.\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // If you want Debug-only validation:
    bool enableValidation =
    #if defined(_DEBUG)
        true;
    #else
        false;
    #endif

    Renderer renderer;
    if (!renderer.init(wh, startW, startH, enableValidation)) {
        std::cerr << "Renderer init failed.\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                running = false;
                break;

            // SDL3 resize events you’ll commonly see:
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                // data1/data2 are the new size for these events
                int w = e.window.data1;
                int h = e.window.data2;
                if (w > 0 && h > 0) {
                    renderer.onResize(w, h);
                }
            } break;

            default:
                break;
            }
        }

        renderer.drawFrame();
        // No SDL_Delay needed; vsync via FIFO present mode will pace you.
        // If you later use MAILBOX or IMMEDIATE, consider adding a tiny delay.
    }

    renderer.shutdown();

    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "Clean shutdown.\n";
    return 0;
}

#include "Window.hpp"

#include <iostream>

Window::~Window() { shutdown(); }

bool Window::init(const char *title, int width, int height) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
    return false;
  }

  m_width = width;
  m_height = height;

  m_window = SDL_CreateWindow(title, width, height,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

  if (!m_window) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return false;
  }

  return true;
}

void Window::shutdown() {
  if (m_window) {
    SDL_DestroyWindow(m_window);
    m_window = nullptr;
  }

  // Safe to call even if SDL wasn’t initialized, but we only do it if we own
  // init flow.
  SDL_Quit();

  m_running = false;
}

bool Window::pumpEvents() {
  m_resized = false;

  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      m_running = false;
      break;

    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      m_width = e.window.data1;
      m_height = e.window.data2;
      m_resized = true;
      break;

    default:
      break;
    }
  }

  return m_running;
}

Win32WindowHandles Window::win32Handles() const {
  Win32WindowHandles wh{};

  if (!m_window) {
    return wh;
  }

  SDL_PropertiesID props = SDL_GetWindowProperties(m_window);
  if (!props) {
    std::cerr << "SDL_GetWindowProperties failed: " << SDL_GetError()
              << std::endl;

    return wh;
  }

  HWND hwnd =
      (HWND)SDL_GetPointerProperty(props, "SDL.window.win32.hwnd", nullptr);
  HINSTANCE hinstance = (HINSTANCE)SDL_GetPointerProperty(
      props, "SDL.window.win32.hinstance", nullptr);

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

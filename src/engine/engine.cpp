#include "Engine.hpp"

#include <iostream>

bool Engine::init() {
  const int width = 1920;
  const int height = 1080;

  if (!m_window.init("evergreen", width, height)) {
    return false;
  }

  auto windowHandle = m_window.win32Handles();
  if (!windowHandle.hwnd || !windowHandle.hinstance) {
    std::cerr << "Engine: failed to get Win32 handles." << std::endl;
    return false;
  }

  bool enableValidation = true;

  if (!m_renderer.init(windowHandle, width, height, enableValidation)) {
    std::cerr << "Engine: renderer init failed." << std::endl;
    return false;
  }

  return true;
}

void Engine::run() {
  bool running = true;

  while (running) {
    running = m_window.pumpEvents();

    tick();

    if (m_window.wasResized()) {
      m_renderer.resize(m_window.width(), m_window.height());
      m_scene.get()->resize(m_window.width(), m_window.height());

      m_window.clearResizedFlag();
    }

    m_renderer.update(m_deltaTime);
    m_scene.get()->update(m_renderer, m_deltaTime);

    m_renderer.drawFrame(m_scene.get());
  }
}

void Engine::tick() {
  uint64_t now = SDL_GetTicks();
  if (m_previousTime == 0) {
    m_previousTime = now;
  }
  m_deltaTime = float(now - m_previousTime) / 1000.0f;
  m_previousTime = now;
}

void Engine::clear() {
  // Scene depends on renderer.
  m_scene.get()->clear(m_renderer);

  // Renderer depends on window.
  m_renderer.clear();

  // Window is the last to go.
  m_window.clear();
}

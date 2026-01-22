#include "Engine.hpp"

#include <iostream>

Engine::~Engine() { shutdown(); }

bool Engine::init() {
  const int startW = 800;
  const int startH = 600;

  if (!m_window.init("evergreen", startW, startH)) {
    return false;
  }

  auto wh = m_window.win32Handles();
  if (!wh.hwnd || !wh.hinstance) {
    std::cerr << "Engine: failed to get Win32 handles.\n";
    return false;
  }

  //   bool enableValidation =
  // #if defined(_DEBUG)
  //       true;
  // #else
  //       false;
  // #endif
  bool enableValidation = true;

  if (!m_renderer.init(wh, startW, startH, enableValidation)) {
    std::cerr << "Engine: renderer init failed.\n";
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
      m_renderer.onResize(m_window.width(), m_window.height());
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

void Engine::shutdown() {
  // Order matters, scene depends on renderer, and renderer depends on window.
  m_scene.get()->shutdown();
  m_renderer.shutdown();
  m_window.shutdown();
}

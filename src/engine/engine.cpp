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

int Engine::run() {
  bool running = true;

  while (running) {
    running = m_window.pumpEvents();

    if (m_window.wasResized()) {
      m_renderer.onResize(m_window.width(), m_window.height());
      m_window.clearResizedFlag();
    }

    m_renderer.drawFrame();
  }

  return 0;
}

void Engine::shutdown() {
  // Renderer depends on window surface, so renderer down first.
  m_renderer.shutdown();

  m_window.shutdown();
}

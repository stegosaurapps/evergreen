#pragma once

#include "Renderer.hpp"
#include "Window.hpp"

class Engine {
public:
  Engine() = default;
  ~Engine();

  Engine(const Engine &) = delete;
  Engine &operator=(const Engine &) = delete;

  bool init();
  int run();
  void shutdown();

private:
  Window m_window;
  Renderer m_renderer;
};

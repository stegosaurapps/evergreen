#pragma once

#include "Scene.hpp"
#include "Window.hpp"

#include "renderer/Renderer.hpp"

#include <memory>

class Engine {
public:
  Engine() = default;
  ~Engine();

  Engine(const Engine &) = delete;
  Engine &operator=(const Engine &) = delete;

  bool init();

  void loadScene(Scene scene) { m_scene = std::make_unique<Scene>(scene); };
  void run();

  void shutdown();

  float deltaTime() { return m_deltaTime; };

  Window &window() { return m_window; };
  Renderer &renderer() { return m_renderer; };
  Scene *scene() { return m_scene.get(); };

private:
  uint64_t m_previousTime = 0;
  float m_deltaTime = 0;

  Window m_window;
  Renderer m_renderer;
  std::unique_ptr<Scene> m_scene;

  void tick();
};

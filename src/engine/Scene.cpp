#include "../Vulkan.hpp"

#include "Renderer.hpp"
#include "Scene.hpp"

#include <iostream>

void Scene::init(Renderer &renderer, Camera camera, std::vector<Model> models,
                 Sky *sky,
                 std::function<void(Renderer &, Scene *)> createPipeline,
                 std::function<void(Renderer &, Scene *)> destroyPipeline) {
  m_camera = camera;

  m_models = models;

  m_sky = sky;

  m_createPipeline = createPipeline;
  m_destroyPipeline = destroyPipeline;

  m_destroyPipeline(renderer, this);
  m_createPipeline(renderer, this);
}

void Scene::update(Renderer &renderer, float deltaTime) {
  // orbit update (camera owns orbit state)
  m_camera.orbitStep(deltaTime, 0.2);

  // Keep camera current (aspect updates on resize handled in onResize).
  m_camera.updateMatrices();

  std::memcpy(m_uboMappedList[renderer.frameIndex()], &m_camera.ubo(),
              sizeof(CameraUBO));
}

void Scene::draw(Renderer &renderer) {}

void Scene::createPipeline(Renderer &renderer) {
  m_createPipeline(renderer, this);
}

void Scene::destroyPipeline(Renderer &renderer) {
  m_destroyPipeline(renderer, this);
}

void Scene::resize(int width, int height) { m_camera.onResize(width, height); }

void Scene::attachCamera(Camera camera) { m_camera = camera; }

Camera &Scene::camera() { return m_camera; }

void Scene::addModel(Model model) { m_models.push_back(std::move(model)); }

std::vector<Model> &Scene::models() { return m_models; }

VkPipelineLayout *Scene::pipelineLayout() { return &m_pipelineLayout; }

VkPipeline *Scene::pipeline() { return &m_pipeline; }

VkDescriptorPool *Scene::frameDescriptorPool() {
  return &m_frameDescriptorPool;
};

VkDescriptorSetLayout *Scene::frameDescriptorSetLayout() {
  return &m_frameDescriptorSetLayout;
}

std::array<VkDescriptorSet, FRAME_COUNT> *Scene::frameDescriptorSets() {
  return &m_frameDescriptorSets;
}

std::array<VkBuffer, FRAME_COUNT> *Scene::uboBufferList() {
  return &m_uboBufferList;
}

std::array<VkDeviceMemory, FRAME_COUNT> *Scene::uboMemoryList() {
  return &m_uboMemoryList;
}

Sky *Scene::sky() { return m_sky; }

std::array<void *, FRAME_COUNT> *Scene::uboMappedList() {
  return &m_uboMappedList;
}

void Scene::shutdown(Renderer &renderer) {
  for (auto model : m_models) {
    model.clear(renderer);
  }

  // Clear resources...

  m_destroyPipeline(renderer, this);
}

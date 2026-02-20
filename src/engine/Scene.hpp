#pragma once

#include "Camera.hpp"
#include "Constants.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#include "Sky.hpp"
#include "Transform.hpp"
#include "Vertex.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

class Renderer; // forward declaration

class Scene {
public:
  Scene() = default;
  ~Scene() = default;

  void init(Renderer &renderer, Camera camera, std::vector<Model> models,
            Sky *sky, std::function<void(Renderer &, Scene *)> createPipeline,
            std::function<void(Renderer &, Scene *)> destroyPipeline);
  void update(Renderer &renderer, float deltaTime);
  void draw(Renderer &renderer);
  void createPipeline(Renderer &renderer);
  void destroyPipeline(Renderer &renderer);
  void resize(int width, int height);

  void attachCamera(Camera camera);
  Camera &camera();

  void addModel(Model model);
  std::vector<Model> &models();

  VkPipelineLayout *pipelineLayout();
  VkPipeline *pipeline();

  VkDescriptorPool *frameDescriptorPool();
  VkDescriptorSetLayout *frameDescriptorSetLayout();
  std::array<VkDescriptorSet, FRAME_COUNT> *frameDescriptorSets();

  std::array<VkBuffer, FRAME_COUNT> *uboBufferList();
  std::array<VkDeviceMemory, FRAME_COUNT> *uboMemoryList();
  std::array<void *, FRAME_COUNT> *uboMappedList();

  void shutdown(Renderer &renderer);

private:
  Camera m_camera;

  // List of renderables
  std::vector<Model> m_models;

  // Optional global lighting
  Sky *m_sky;

  // Pipeline
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_pipeline = VK_NULL_HANDLE;

  // Frame Descriptor
  VkDescriptorPool m_frameDescriptorPool = VK_NULL_HANDLE;
  VkDescriptorSetLayout m_frameDescriptorSetLayout = VK_NULL_HANDLE;
  std::array<VkDescriptorSet, FRAME_COUNT> m_frameDescriptorSets{};

  // Uniform buffers (one per frame-in-flight)
  std::array<VkBuffer, FRAME_COUNT> m_uboBufferList{};
  std::array<VkDeviceMemory, FRAME_COUNT> m_uboMemoryList{};
  std::array<void *, FRAME_COUNT> m_uboMappedList{};

  std::function<void(Renderer &, Scene *)> m_createPipeline;
  std::function<void(Renderer &, Scene *)> m_destroyPipeline;
};

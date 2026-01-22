#pragma once

#include "Camera.hpp"
#include "Constants.hpp"
#include "Mesh.hpp"
#include "Renderable.hpp"
// #include "Renderer.hpp"
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
  ~Scene();

  void init(Renderer &renderer, Camera camera,
            std::vector<Renderable> renderables,
            std::function<void(Renderer &, Scene *)> createPipeline,
            std::function<void(Renderer &, Scene *)> destroyPipeline);
  void update(Renderer &renderer, float deltaTime);
  void draw(Renderer &renderer);
  void createPipeline(Renderer &renderer);
  void destroyPipeline(Renderer &renderer);
  void resize(int width, int height);

  void attachCamera(Camera camera);
  Camera &camera();

  void addRenderable(Renderable renderable);
  std::vector<Renderable> &renderables();

  VkPipelineLayout *pipelineLayout();
  VkPipeline *pipeline();

  VkDescriptorSetLayout *descriptorSetLayout();
  VkDescriptorPool *descriptorPool();
  std::array<VkDescriptorSet, FRAME_COUNT> *descriptorSets();

  std::array<VkBuffer, FRAME_COUNT> *uboBufferList();
  std::array<VkDeviceMemory, FRAME_COUNT> *uboMemoryList();
  std::array<void *, FRAME_COUNT> *uboMappedList();

  uint32_t *indexCount();
  VkBuffer *vertexBuffer();
  VkDeviceMemory *vertexMemory();
  VkBuffer *indexBuffer();
  VkDeviceMemory *indexMemory();

  void shutdown();

private:
  Camera m_camera;
  std::vector<Renderable> m_renderables;

  // Pipeline
  VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
  VkPipeline m_pipeline = VK_NULL_HANDLE;

  // Descriptors (set 0 = per-frame camera)
  VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
  std::array<VkDescriptorSet, FRAME_COUNT> m_DescriptorSets{};

  // Uniform buffers (one per frame-in-flight)
  std::array<VkBuffer, FRAME_COUNT> m_uboBufferList{};
  std::array<VkDeviceMemory, FRAME_COUNT> m_uboMemoryList{};
  std::array<void *, FRAME_COUNT> m_uboMappedList{};

  // One demo mesh on GPU (first scene renderable)
  uint32_t m_indexCount = 0;
  VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
  VkBuffer m_indexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;

  std::function<void(Renderer &, Scene *)> m_createPipeline;
  std::function<void(Renderer &, Scene *)> m_destroyPipeline;

  void destroyResources(Renderer &renderer);
};

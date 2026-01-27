#include "Scene.hpp"

#include "../Vulkan.hpp"

#include "Renderer.hpp"

#include <iostream>

void Scene::init(Renderer &renderer, Camera camera, std::vector<Model> models,
                 std::function<void(Renderer &, Scene *)> createPipeline,
                 std::function<void(Renderer &, Scene *)> destroyPipeline) {
  m_camera = camera;
  m_models = models;
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

VkDescriptorSetLayout *Scene::descriptorSetLayout() {
  return &m_descriptorSetLayout;
}

VkDescriptorPool *Scene::descriptorPool() { return &m_descriptorPool; };

std::array<VkDescriptorSet, FRAME_COUNT> *Scene::descriptorSets() {
  return &m_DescriptorSets;
}

std::array<VkBuffer, FRAME_COUNT> *Scene::uboBufferList() {
  return &m_uboBufferList;
}

std::array<VkDeviceMemory, FRAME_COUNT> *Scene::uboMemoryList() {
  return &m_uboMemoryList;
}

std::array<void *, FRAME_COUNT> *Scene::uboMappedList() {
  return &m_uboMappedList;
}

uint32_t *Scene::indexCount() { return &m_indexCount; }

VkBuffer *Scene::vertexBuffer() { return &m_vertexBuffer; }

VkDeviceMemory *Scene::vertexMemory() { return &m_vertexMemory; }

VkBuffer *Scene::indexBuffer() { return &m_indexBuffer; }

VkDeviceMemory *Scene::indexMemory() { return &m_indexMemory; }

void Scene::destroyResources(Renderer &renderer) {
  auto device = renderer.device();

  if (!device) {
    return;
  }

  // Mesh buffers
  if (m_vertexBuffer) {
    vkDestroyBuffer(device, m_vertexBuffer, nullptr);
    m_vertexBuffer = VK_NULL_HANDLE;
  }
  if (m_vertexMemory) {
    vkFreeMemory(device, m_vertexMemory, nullptr);
    m_vertexMemory = VK_NULL_HANDLE;
  }

  if (m_indexBuffer) {
    vkDestroyBuffer(device, m_indexBuffer, nullptr);
    m_indexBuffer = VK_NULL_HANDLE;
  }
  if (m_indexMemory) {
    vkFreeMemory(device, m_indexMemory, nullptr);
    m_indexMemory = VK_NULL_HANDLE;
  }

  // Uniform buffers (per-frame)
  for (int i = 0; i < FRAME_COUNT; ++i) {
    if (m_uboMappedList[i]) {
      vkUnmapMemory(device, m_uboMemoryList[i]);
      m_uboMappedList[i] = nullptr;
    }

    if (m_uboBufferList[i]) {
      vkDestroyBuffer(device, m_uboBufferList[i], nullptr);
      m_uboBufferList[i] = VK_NULL_HANDLE;
    }

    if (m_uboMemoryList[i]) {
      vkFreeMemory(device, m_uboMemoryList[i], nullptr);
      m_uboMemoryList[i] = VK_NULL_HANDLE;
    }
  }

  // Descriptor pool
  if (m_descriptorPool) {
    vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
    m_descriptorPool = VK_NULL_HANDLE;
  }

  // Descriptor set layout
  if (m_descriptorSetLayout) {
    vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
    m_descriptorSetLayout = VK_NULL_HANDLE;
  }
}

void Scene::shutdown() {
  // m_destroyPipeline();
  // destroyResources();
}

// void Renderer::destroyDeviceResources() {
//   if (!m_device) {
//     return;
//   }

//   // Mesh buffers
//   if (m_vertexBuffer) {
//     vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
//     m_vertexBuffer = VK_NULL_HANDLE;
//   }
//   if (m_vertexMemory) {
//     vkFreeMemory(m_device, m_vertexMemory, nullptr);
//     m_vertexMemory = VK_NULL_HANDLE;
//   }

//   if (m_indexBuffer) {
//     vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
//     m_indexBuffer = VK_NULL_HANDLE;
//   }
//   if (m_indexMemory) {
//     vkFreeMemory(m_device, m_indexMemory, nullptr);
//     m_indexMemory = VK_NULL_HANDLE;
//   }

//   // Uniform buffers (per-frame)
//   for (int i = 0; i < FRAME_COUNT; ++i) {
//     if (m_uboMapped[i]) {
//       vkUnmapMemory(m_device, m_uboMemory[i]);
//       m_uboMapped[i] = nullptr;
//     }

//     if (m_uboBuffer[i]) {
//       vkDestroyBuffer(m_device, m_uboBuffer[i], nullptr);
//       m_uboBuffer[i] = VK_NULL_HANDLE;
//     }

//     if (m_uboMemory[i]) {
//       vkFreeMemory(m_device, m_uboMemory[i], nullptr);
//       m_uboMemory[i] = VK_NULL_HANDLE;
//     }
//   }

//   // Cube resources
//   // TODO: Move to scene
//   if (m_cubeVB) {
//     vkDestroyBuffer(m_device, m_cubeVB, nullptr);
//     m_cubeVB = VK_NULL_HANDLE;
//   }
//   if (m_cubeVBMem) {
//     vkFreeMemory(m_device, m_cubeVBMem, nullptr);
//     m_cubeVBMem = VK_NULL_HANDLE;
//   }
//   if (m_cubeIB) {
//     vkDestroyBuffer(m_device, m_cubeIB, nullptr);
//     m_cubeIB = VK_NULL_HANDLE;
//   }
//   if (m_cubeIBMem) {
//     vkFreeMemory(m_device, m_cubeIBMem, nullptr);
//     m_cubeIBMem = VK_NULL_HANDLE;
//   }
//   m_cubeIndexCount = 0;

//   // Descriptor pool
//   if (m_descPool) {
//     vkDestroyDescriptorPool(m_device, m_descPool, nullptr);
//     m_descPool = VK_NULL_HANDLE;
//   }

//   // Descriptor set layout
//   if (m_setLayoutFrame) {
//     vkDestroyDescriptorSetLayout(m_device, m_setLayoutFrame, nullptr);
//     m_setLayoutFrame = VK_NULL_HANDLE;
//   }
// }

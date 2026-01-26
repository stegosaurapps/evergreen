#pragma once

#include "../Vulkan.hpp"

#include "../engine/Camera.hpp"
#include "../engine/Cube.hpp"
#include "../engine/Dimensions.hpp"
#include "../engine/Renderable.hpp"
#include "../engine/Renderer.hpp"
#include "../engine/Scene.hpp"

#include <memory>
#include <vector>

Camera createCamera(Dimensions dimensions) {
  Camera camera;
  camera.setOrbitTarget({0.0f, 0.0f, 0.0f});
  camera.setOrbitRadius(4.0f);
  camera.setOrbitAngles(0.0f, 0.35f);
  camera.setPerspective(1.04719755f, dimensions.ratio(), 0.1f, 200.0f);
  camera.lookAt({0.0f, 0.0f, 0.0f});
  camera.updateMatrices();

  return camera;
}

void createDescriptorSetLayout(Renderer &renderer, Scene *scene) {
  auto device = renderer.device();

  VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
  descriptorSetLayoutBinding.binding = 0;
  descriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorSetLayoutBinding.descriptorCount = 1;
  descriptorSetLayoutBinding.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  descriptorSetLayoutCreateInfo.bindingCount = 1;
  descriptorSetLayoutCreateInfo.pBindings = &descriptorSetLayoutBinding;

  auto result =
      vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo,
                                  nullptr, scene->descriptorSetLayout());
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateDescriptorSetLayout failed" << std::endl;
    std::abort();
  }
}

void createDescriptorPool(Renderer &renderer, Scene *scene) {
  auto device = renderer.device();

  auto descriptorSetLayout = scene->descriptorSetLayout();
  auto descriptorPool = scene->descriptorPool();
  auto descriptorSets = scene->descriptorSets();

  VkDescriptorPoolSize descriptorPoolSize{};
  descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorPoolSize.descriptorCount = FRAME_COUNT;

  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  descriptorPoolCreateInfo.maxSets = FRAME_COUNT;
  descriptorPoolCreateInfo.poolSizeCount = 1;
  descriptorPoolCreateInfo.pPoolSizes = &descriptorPoolSize;

  auto result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo,
                                       nullptr, descriptorPool);
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateDescriptorPool failed" << std::endl;
    std::abort();
  }

  std::array<VkDescriptorSetLayout, FRAME_COUNT> descriptorSetLayouts;
  for (int i = 0; i < FRAME_COUNT; ++i) {
    descriptorSetLayouts[i] = *descriptorSetLayout;
  }

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  descriptorSetAllocateInfo.descriptorPool = *descriptorPool;
  descriptorSetAllocateInfo.descriptorSetCount = FRAME_COUNT;
  descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts.data();

  if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo,
                               descriptorSets->data()) != VK_SUCCESS) {
    std::cerr << "vkAllocateDescriptorSets failed" << std::endl;
    std::abort();
  }
}

void createPipeline(Renderer &renderer, Scene *scene) {
  auto device = renderer.device();

  auto pipelineLayout = scene->pipelineLayout();
  auto pipeline = scene->pipeline();

  auto descriptorSetLayout = scene->descriptorSetLayout();

  if (*pipeline) {
    vkDestroyPipeline(device, *pipeline, nullptr);
    *pipeline = VK_NULL_HANDLE;
  }

  if (*pipelineLayout) {
    vkDestroyPipelineLayout(device, *pipelineLayout, nullptr);
    *pipelineLayout = VK_NULL_HANDLE;
  }

  std::vector<char> vsBytes;
  std::vector<char> fsBytes;

  if (!ReadFileBytes("shaders/test.vert.spv", vsBytes)) {
    std::cerr << "Missing vertex shader shaders/test.vert.spv" << std::endl;
    std::abort();
  }

  if (!ReadFileBytes("shaders/test.frag.spv", fsBytes)) {
    std::cerr << "Missing fragment shader shaders/test.frag.spv" << std::endl;
    std::abort();
  }

  VkShaderModule vs = CreateShaderModule(device, vsBytes);
  if (!vs) {
    std::cerr << "Failed to create vertex shader module" << std::endl;
    std::abort();
  }

  VkShaderModule fs = CreateShaderModule(device, fsBytes);
  if (!fs) {
    std::cerr << "Failed to create fragment shader module" << std::endl;
    std::abort();
  }

  VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo[2]{};
  pipelineShaderStageCreateInfo[0].sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipelineShaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  pipelineShaderStageCreateInfo[0].module = vs;
  pipelineShaderStageCreateInfo[0].pName = "main";
  pipelineShaderStageCreateInfo[1].sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipelineShaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  pipelineShaderStageCreateInfo[1].module = fs;
  pipelineShaderStageCreateInfo[1].pName = "main";

  // Vertex input
  VkVertexInputBindingDescription vertexInputBindingDescription{};
  vertexInputBindingDescription.binding = 0;
  vertexInputBindingDescription.stride = sizeof(VertexPCN);
  vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription vertexInputAttributeDescription[3]{};

  // position
  vertexInputAttributeDescription[0].location = 0;
  vertexInputAttributeDescription[0].binding = 0;
  vertexInputAttributeDescription[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescription[0].offset = offsetof(VertexPCN, px);

  // normal
  vertexInputAttributeDescription[1].location = 1;
  vertexInputAttributeDescription[1].binding = 0;
  vertexInputAttributeDescription[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescription[1].offset = offsetof(VertexPCN, nx);

  // color
  vertexInputAttributeDescription[2].location = 2;
  vertexInputAttributeDescription[2].binding = 0;
  vertexInputAttributeDescription[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  vertexInputAttributeDescription[2].offset = offsetof(VertexPCN, r);

  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
  pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions =
      &vertexInputBindingDescription;
  pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 3;
  pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions =
      vertexInputAttributeDescription;

  VkPipelineInputAssemblyStateCreateInfo pipelineInputAsseblyStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  pipelineInputAsseblyStateCreateInfo.topology =
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  pipelineInputAsseblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  pipelineViewportStateCreateInfo.viewportCount = 1;
  pipelineViewportStateCreateInfo.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
  pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  pipelineRasterizationStateCreateInfo.frontFace =
      VK_FRONT_FACE_COUNTER_CLOCKWISE;
  pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
  pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;

  VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  pipelineMultisampleStateCreateInfo.rasterizationSamples =
      renderer.sampleCount();

  VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
  pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
  pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;

  VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
  pipelineColorBlendAttachmentState.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  pipelineColorBlendStateCreateInfo.attachmentCount = 1;
  pipelineColorBlendStateCreateInfo.pAttachments =
      &pipelineColorBlendAttachmentState;

  VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                    VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  pipelineDynamicStateCreateInfo.dynamicStateCount = 2;
  pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStates;

  // Push constant = model matrix.
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(Mat4);

  if (*descriptorSetLayout == VK_NULL_HANDLE) {
    std::cerr << "createPipeline: m_setLayoutFrame is VK_NULL_HANDLE "
                 "(descriptor set layout missing)\n";
  }

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout;
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr,
                             pipelineLayout) != VK_SUCCESS) {
    std::cerr << "vkCreatePipelineLayout failed\n";
    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);
    std::abort();
  }

  VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
  graphicsPipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  graphicsPipelineCreateInfo.pNext = nullptr;

  graphicsPipelineCreateInfo.stageCount = 2;
  graphicsPipelineCreateInfo.pStages = pipelineShaderStageCreateInfo;

  graphicsPipelineCreateInfo.pVertexInputState =
      &pipelineVertexInputStateCreateInfo;
  graphicsPipelineCreateInfo.pInputAssemblyState =
      &pipelineInputAsseblyStateCreateInfo;
  graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
  graphicsPipelineCreateInfo.pRasterizationState =
      &pipelineRasterizationStateCreateInfo;
  graphicsPipelineCreateInfo.pMultisampleState =
      &pipelineMultisampleStateCreateInfo;
  graphicsPipelineCreateInfo.pDepthStencilState =
      &pipelineDepthStencilStateCreateInfo;
  graphicsPipelineCreateInfo.pColorBlendState =
      &pipelineColorBlendStateCreateInfo;
  graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;

  graphicsPipelineCreateInfo.layout = *pipelineLayout;
  graphicsPipelineCreateInfo.renderPass = renderer.renderPass();
  graphicsPipelineCreateInfo.subpass = 0;

  // Optional but commonly set:
  graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  graphicsPipelineCreateInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                &graphicsPipelineCreateInfo, nullptr,
                                pipeline) != VK_SUCCESS) {
    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);
    std::cerr << "vkCreateGraphicsPipelines failed\n";
    std::abort();
  }

  vkDestroyShaderModule(device, vs, nullptr);
  vkDestroyShaderModule(device, fs, nullptr);
}

void destroyPipeline(Renderer &renderer, Scene *scene) {
  auto device = renderer.device();

  auto pipelineLayout = scene->pipelineLayout();
  auto pipeline = scene->pipeline();

  auto descriptorSetLayout = scene->descriptorSetLayout();

  if (!device) {
    return;
  }

  // Destroy old pipeline and layout if rebuilding (swapchain resize).
  if (*pipeline) {
    vkDestroyPipeline(device, *pipeline, nullptr);
    *pipeline = VK_NULL_HANDLE;
  }

  if (*pipelineLayout) {
    vkDestroyPipelineLayout(device, *pipelineLayout, nullptr);
    *pipelineLayout = VK_NULL_HANDLE;
  }
}

void createUniformBuffers(Renderer &renderer, Scene *scene) {
  auto device = renderer.device();

  auto uboBufferList = scene->uboBufferList();
  auto uboMemoryList = scene->uboMemoryList();
  auto uboMappedList = scene->uboMappedList();

  auto descriptorSets = scene->descriptorSets();

  for (int i = 0; i < FRAME_COUNT; ++i) {
    if (!CreateBuffer(renderer.physicalDevice(), device, sizeof(CameraUBO),
                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      (*uboBufferList)[i], (*uboMemoryList)[i])) {
      std::cerr << "Failed to create uniform buffer" << std::endl;
      std::abort();
    }

    if (vkMapMemory(device, (*uboMemoryList)[i], 0, sizeof(CameraUBO), 0,
                    &(*uboMappedList)[i]) != VK_SUCCESS) {
      std::cerr << "vkMapMemory failed for UBO" << std::endl;
      std::abort();
    }

    VkDescriptorBufferInfo descriptorBufferInfo{};
    descriptorBufferInfo.buffer = (*uboBufferList)[i];
    descriptorBufferInfo.offset = 0;
    descriptorBufferInfo.range = sizeof(CameraUBO);

    VkWriteDescriptorSet writeDescriptorSet{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeDescriptorSet.dstSet = (*descriptorSets)[i];
    writeDescriptorSet.dstBinding = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeDescriptorSet.pBufferInfo = &descriptorBufferInfo;

    vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
  }
}

void createMesh(Renderer &renderer, Scene *scene) {
  auto physicalDevice = renderer.physicalDevice();
  auto device = renderer.device();

  std::vector<VertexPCN> verts;
  std::vector<uint32_t> idx;

  BuildCube(verts, idx);

  *scene->indexCount() = (uint32_t)idx.size();

  const VkDeviceSize vertexBufferSize = sizeof(VertexPCN) * verts.size();
  const VkDeviceSize indexBufferSize = sizeof(uint32_t) * idx.size();

  if (!CreateBuffer(physicalDevice, device, vertexBufferSize,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    *scene->vertexBuffer(), *scene->vertexMemory())) {
    std::cerr << "Failed to create cube vertex buffer" << std::endl;
    std::abort();
  }

  if (!CreateBuffer(physicalDevice, device, indexBufferSize,
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    *scene->indexBuffer(), *scene->indexMemory())) {
    std::cerr << "Failed to create cube index buffer" << std::endl;
    std::abort();
  }

  void *data = nullptr;
  vkMapMemory(device, *scene->vertexMemory(), 0, vertexBufferSize, 0, &data);
  std::memcpy(data, verts.data(), (size_t)vertexBufferSize);
  vkUnmapMemory(device, *scene->vertexMemory());

  vkMapMemory(device, *scene->indexMemory(), 0, indexBufferSize, 0, &data);
  std::memcpy(data, idx.data(), (size_t)indexBufferSize);
  vkUnmapMemory(device, *scene->indexMemory());

  std::cout << "Cube verts=" << verts.size() << " idx=" << idx.size()
            << std::endl;
}

Scene LoadScene(Renderer &renderer) {
  Scene scene;

  // First create descriptors
  createDescriptorSetLayout(renderer, &scene);
  createDescriptorPool(renderer, &scene);

  // Next create uniform buffers
  createUniformBuffers(renderer, &scene);

  // Finally create renderable
  createMesh(renderer, &scene);

  // TODO: Setup Renderables
  std::vector<Renderable> renderables{};

  auto camera = createCamera(renderer.dimensions());

  scene.init(renderer, camera, renderables, createPipeline, destroyPipeline);

  return scene;
}

// void Renderer::createMeshBuffers() {
//   // Build a single triangle in the scene. You'll replace this with a real
//   model
//   // loader.
//   // m_scene = Scene{};
//   Scene m_scene;
//   Renderable r{};
//   r.mesh.vertices = {
//       {0.0f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.5f, 1.0f},
//       {0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f},
//       {-0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
//   };
//   r.mesh.indices = {0, 1, 2};
//   m_scene.addRenderable(std::move(r));

//   const Mesh &m = m_scene.renderables()[0].mesh;
//   m_indexCount = (uint32_t)m.indices.size();

//   // Recreate buffers if they exist.
//   if (m_vertexBuffer) {
//     vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
//     vkFreeMemory(m_device, m_vertexMemory, nullptr);
//     m_vertexBuffer = VK_NULL_HANDLE;
//     m_vertexMemory = VK_NULL_HANDLE;
//   }
//   if (m_indexBuffer) {
//     vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
//     vkFreeMemory(m_device, m_indexMemory, nullptr);
//     m_indexBuffer = VK_NULL_HANDLE;
//     m_indexMemory = VK_NULL_HANDLE;
//   }

//   VkDeviceSize vsize = sizeof(Vertex) * m.vertices.size();
//   VkDeviceSize isize = sizeof(uint32_t) * m.indices.size();

//   if (!CreateBuffer(m_physicalDevice, m_device, vsize,
//                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
//                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                     m_vertexBuffer, m_vertexMemory)) {
//     std::abort();
//   }
//   if (!CreateBuffer(m_physicalDevice, m_device, isize,
//                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
//                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                     m_indexBuffer, m_indexMemory)) {
//     std::abort();
//   }

//   void *mapped = nullptr;
//   vkMapMemory(m_device, m_vertexMemory, 0, vsize, 0, &mapped);
//   std::memcpy(mapped, m.vertices.data(), (size_t)vsize);
//   vkUnmapMemory(m_device, m_vertexMemory);

//   vkMapMemory(m_device, m_indexMemory, 0, isize, 0, &mapped);
//   std::memcpy(mapped, m.indices.data(), (size_t)isize);
//   vkUnmapMemory(m_device, m_indexMemory);
// }

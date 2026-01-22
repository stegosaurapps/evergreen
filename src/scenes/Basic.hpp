#pragma once

#include "Helpers.hpp"

#include "../engine/Camera.hpp"
#include "../engine/Cube.hpp"
#include "../engine/Dimensions.hpp"
#include "../engine/Renderable.hpp"
#include "../engine/Scene.hpp"

#include "../engine/renderer/Renderer.hpp"

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
  VkDescriptorSetLayoutBinding cam{};
  cam.binding = 0;
  cam.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  cam.descriptorCount = 1;
  cam.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo createInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  createInfo.bindingCount = 1;
  createInfo.pBindings = &cam;

  auto result = vkCreateDescriptorSetLayout(
      renderer.getDevice(), &createInfo, nullptr, scene->descriptorSetLayout());
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateDescriptorSetLayout failed" << std::endl;
    std::abort();
  }
}

void createDescriptorPool(Renderer &renderer, Scene *scene) {
  auto device = renderer.getDevice();

  auto descriptorSetLayout = scene->descriptorSetLayout();
  auto descriptorPool = scene->descriptorPool();
  auto descriptorSets = scene->descriptorSets();

  VkDescriptorPoolSize descriptorPoolSize{};
  descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorPoolSize.descriptorCount = FRAME_COUNT;

  VkDescriptorPoolCreateInfo createInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  createInfo.maxSets = FRAME_COUNT;
  createInfo.poolSizeCount = 1;
  createInfo.pPoolSizes = &descriptorPoolSize;

  auto result =
      vkCreateDescriptorPool(device, &createInfo, nullptr, descriptorPool);
  if (result != VK_SUCCESS) {
    std::cerr << "vkCreateDescriptorPool failed" << std::endl;
    std::abort();
  }

  std::array<VkDescriptorSetLayout, FRAME_COUNT> layouts;
  for (int i = 0; i < FRAME_COUNT; ++i) {
    layouts[i] = *descriptorSetLayout;
  }

  VkDescriptorSetAllocateInfo allocateInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocateInfo.descriptorPool = *descriptorPool;
  allocateInfo.descriptorSetCount = FRAME_COUNT;
  allocateInfo.pSetLayouts = layouts.data();

  if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets->data()) !=
      VK_SUCCESS) {
    std::cerr << "vkAllocateDescriptorSets failed" << std::endl;
    std::abort();
  }
}

void createPipeline(Renderer &renderer, Scene *scene) {
  auto device = renderer.getDevice();

  auto pipelineLayout = scene->pipelineLayout();
  auto pipeline = scene->pipeline();

  auto descriptorSetLayout = scene->descriptorSetLayout();

  // Destroy old pipeline and layout if rebuilding (swapchain resize).
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

  if (!ReadFileBytes("shaders/test.vert.spv", vsBytes) ||
      !ReadFileBytes("shaders/test.frag.spv", fsBytes)) {
    std::cerr << "Missing shaders. Build will generate shaders/test.*.spv (via "
                 "glslc)."
              << std::endl;
    std::abort();
  }

  VkShaderModule vs = CreateShaderModule(device, vsBytes);
  VkShaderModule fs = CreateShaderModule(device, fsBytes);
  if (!vs || !fs) {
    std::cerr << "Failed to create shader modules" << std::endl;
    std::abort();
  }

  VkPipelineShaderStageCreateInfo stages[2]{};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vs;
  stages[0].pName = "main";
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = fs;
  stages[1].pName = "main";

  // Vertex input
  VkVertexInputBindingDescription bind{};
  bind.binding = 0;
  bind.stride = sizeof(VertexPCN);
  bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attrs[3]{};

  // position
  attrs[0].location = 0;
  attrs[0].binding = 0;
  attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrs[0].offset = offsetof(VertexPCN, px);

  // normal
  attrs[1].location = 1;
  attrs[1].binding = 0;
  attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrs[1].offset = offsetof(VertexPCN, nx);

  // color
  attrs[2].location = 2;
  attrs[2].binding = 0;
  attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attrs[2].offset = offsetof(VertexPCN, r);

  VkPipelineVertexInputStateCreateInfo vi{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vi.vertexBindingDescriptionCount = 1;
  vi.pVertexBindingDescriptions = &bind;
  vi.vertexAttributeDescriptionCount = 3;
  vi.pVertexAttributeDescriptions = attrs;

  VkPipelineInputAssemblyStateCreateInfo ia{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  ia.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo vp{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  vp.viewportCount = 1;
  vp.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rs{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rs.polygonMode = VK_POLYGON_MODE_FILL;
  rs.cullMode = VK_CULL_MODE_BACK_BIT;
  rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rs.lineWidth = 1.0f;
  rs.cullMode = VK_CULL_MODE_NONE;

  VkPipelineMultisampleStateCreateInfo ms{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  ms.rasterizationSamples = renderer.getSampleCount();

  VkPipelineDepthStencilStateCreateInfo ds{
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  ds.depthTestEnable = VK_TRUE;
  ds.depthWriteEnable = VK_TRUE;
  ds.depthCompareOp = VK_COMPARE_OP_LESS;

  VkPipelineColorBlendAttachmentState cba{};
  cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  cba.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo cb{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  cb.attachmentCount = 1;
  cb.pAttachments = &cba;

  VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dyn{
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dyn.dynamicStateCount = 2;
  dyn.pDynamicStates = dynStates;

  // Push constant = model matrix.
  VkPushConstantRange pcr{};
  pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pcr.offset = 0;
  pcr.size = sizeof(Mat4);

  if (*descriptorSetLayout == VK_NULL_HANDLE) {
    std::cerr << "createPipeline: m_setLayoutFrame is VK_NULL_HANDLE "
                 "(descriptor set layout missing)\n";
  }

  VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pl.setLayoutCount = 1;
  pl.pSetLayouts = descriptorSetLayout;
  pl.pushConstantRangeCount = 1;
  pl.pPushConstantRanges = &pcr;

  if (vkCreatePipelineLayout(device, &pl, nullptr, pipelineLayout) !=
      VK_SUCCESS) {
    std::cerr << "vkCreatePipelineLayout failed\n";
    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);
    std::abort();
  }

  VkPipelineRenderingCreateInfoKHR rendering{};

  VkGraphicsPipelineCreateInfo pci{};
  pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pci.pNext = nullptr;

  pci.stageCount = 2;
  pci.pStages = stages;

  pci.pVertexInputState = &vi;
  pci.pInputAssemblyState = &ia;
  pci.pViewportState = &vp;
  pci.pRasterizationState = &rs;
  pci.pMultisampleState = &ms;
  pci.pDepthStencilState = &ds;
  pci.pColorBlendState = &cb;
  pci.pDynamicState = &dyn;

  pci.layout = *pipelineLayout;
  pci.renderPass = renderer.getRenderPass();
  pci.subpass = 0;

  // Optional but commonly set:
  pci.basePipelineHandle = VK_NULL_HANDLE;
  pci.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pci, nullptr,
                                pipeline) != VK_SUCCESS) {
    std::cerr << "vkCreateGraphicsPipelines failed\n";
    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);
    std::abort();
  }

  vkDestroyShaderModule(device, vs, nullptr);
  vkDestroyShaderModule(device, fs, nullptr);
}

void destroyPipeline(Renderer &renderer, Scene *scene) {
  auto device = renderer.getDevice();

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
  auto device = renderer.getDevice();

  auto uboBufferList = scene->uboBufferList();
  auto uboMemoryList = scene->uboMemoryList();
  auto uboMappedList = scene->uboMappedList();

  auto descriptorSets = scene->descriptorSets();

  for (int i = 0; i < FRAME_COUNT; ++i) {
    if (!CreateBuffer(renderer.getPhysicalDevice(), device, sizeof(CameraUBO),
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
  auto physicalDevice = renderer.getPhysicalDevice();
  auto device = renderer.getDevice();

  std::vector<VertexPCN> verts;
  std::vector<uint32_t> idx;

  BuildCube(verts, idx);

  *scene->indexCount() = (uint32_t)idx.size();

  // uint32_t IndexCount = (uint32_t)idx.size();
  // VkBuffer vertexBuffer = VK_NULL_HANDLE;
  // VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
  // VkBuffer indexBuffer = VK_NULL_HANDLE;
  // VkDeviceMemory indexMemory = VK_NULL_HANDLE;

  const VkDeviceSize vbSize = sizeof(VertexPCN) * verts.size();
  const VkDeviceSize ibSize = sizeof(uint32_t) * idx.size();

  if (!CreateBuffer(physicalDevice, device, vbSize,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    *scene->vertexBuffer(), *scene->vertexMemory())) {
    std::cerr << "Failed to create cube vertex buffer" << std::endl;
    std::abort();
  }

  if (!CreateBuffer(physicalDevice, device, ibSize,
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    *scene->indexBuffer(), *scene->indexMemory())) {
    std::cerr << "Failed to create cube index buffer" << std::endl;
    std::abort();
  }

  // Upload
  void *p = nullptr;
  vkMapMemory(device, *scene->vertexMemory(), 0, vbSize, 0, &p);
  std::memcpy(p, verts.data(), (size_t)vbSize);
  vkUnmapMemory(device, *scene->vertexMemory());

  vkMapMemory(device, *scene->indexMemory(), 0, ibSize, 0, &p);
  std::memcpy(p, idx.data(), (size_t)ibSize);
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

  // // Next Pipeline can be created
  // createPipeline(renderer, &scene);

  // Finally create renderable
  createMesh(renderer, &scene);

  // TODO: Setup Renderables
  std::vector<Renderable> renderables{};

  auto camera = createCamera(renderer.getDimensions());

  scene.init(renderer, camera, renderables, createPipeline, destroyPipeline);

  return scene;
}

// void Renderer::createCubeMesh() {
//   std::vector<VertexPCN> verts;
//   std::vector<uint32_t> idx;

//   BuildCube(verts, idx);

//   m_cubeIndexCount = (uint32_t)idx.size();

//   const VkDeviceSize vbSize = sizeof(VertexPCN) * verts.size();
//   const VkDeviceSize ibSize = sizeof(uint32_t) * idx.size();

//   // For now: simplest path, host-visible buffers (fine for a test cube).
//   if (!CreateBuffer(m_physicalDevice, m_device, vbSize,
//                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
//                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                     m_cubeVB, m_cubeVBMem)) {
//     std::cerr << "Failed to create cube VB\n";
//     std::abort();
//   }

//   if (!CreateBuffer(m_physicalDevice, m_device, ibSize,
//                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
//                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                     m_cubeIB, m_cubeIBMem)) {
//     std::cerr << "Failed to create cube IB\n";
//     std::abort();
//   }

//   // Upload
//   void *p = nullptr;
//   vkMapMemory(m_device, m_cubeVBMem, 0, vbSize, 0, &p);
//   std::memcpy(p, verts.data(), (size_t)vbSize);
//   vkUnmapMemory(m_device, m_cubeVBMem);

//   vkMapMemory(m_device, m_cubeIBMem, 0, ibSize, 0, &p);
//   std::memcpy(p, idx.data(), (size_t)ibSize);
//   vkUnmapMemory(m_device, m_cubeIBMem);

//   std::cout << "Cube verts=" << verts.size() << " idx=" << idx.size()
//             << std::endl;
// }

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

#pragma once

#include "../Vulkan.hpp"

#include "../engine/Builder.hpp"
#include "../engine/Camera.hpp"
#include "../engine/Cube.hpp"
#include "../engine/Dimensions.hpp"
#include "../engine/Loader.hpp"
#include "../engine/Renderer.hpp"
#include "../engine/Scene.hpp"

#include <memory>
#include <vector>

VertexDescriptor basicVertexDescriptor() {
  VertexDescriptor vertexDescriptor;

  vertexDescriptor.init({VertexAttribute::Position, VertexAttribute::Normal,
                         VertexAttribute::Tangent,
                         VertexAttribute::TextureCoordinate});
  // vertexDescriptor.init({VertexAttribute::Position, VertexAttribute::Normal,
  // VertexAttribute::Color});

  return vertexDescriptor;
}

Camera createCamera(Dimensions dimensions) {
  Camera camera;
  camera.setOrbitTarget({0.0f, 2.0f, 0.0f});
  camera.setOrbitRadius(6.0f);
  camera.setOrbitAngles(0.0f, -0.45f);
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

VkDescriptorSetLayout createMaterialSetLayout(Renderer &renderer) {
  VkDevice device = renderer.device();

  VkDescriptorSetLayoutBinding bindings[3]{};

  // binding 0: albedo/baseColor (sRGB texture data, but descriptor type is the
  // same)
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // binding 1: metallicRoughness (linear UNORM, packed)
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // binding 2: normal (linear UNORM)
  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo ci{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  ci.bindingCount = 3;
  ci.pBindings = bindings;

  VkDescriptorSetLayout materialSetLayout = VK_NULL_HANDLE;
  if (vkCreateDescriptorSetLayout(device, &ci, nullptr, &materialSetLayout) !=
      VK_SUCCESS) {
    std::cerr << "Failed to create material set layout\n";
    std::abort();
  }

  return materialSetLayout;
}

void createPipeline(Renderer &renderer, Scene *scene) {
  VkDevice device = renderer.device();

  VkPipelineLayout *pipelineLayout = scene->pipelineLayout();
  VkPipeline *pipeline = scene->pipeline();

  VkDescriptorSetLayout *frameSetLayout = scene->descriptorSetLayout();
  VkDescriptorSetLayout materialSetLayout = createMaterialSetLayout(renderer);

  auto vertexDescriptor = basicVertexDescriptor();
  PrintVertexDescriptor(&vertexDescriptor);

  // Destroy old pipeline + layout
  if (*pipeline) {
    vkDestroyPipeline(device, *pipeline, nullptr);
    *pipeline = VK_NULL_HANDLE;
  }
  if (*pipelineLayout) {
    vkDestroyPipelineLayout(device, *pipelineLayout, nullptr);
    *pipelineLayout = VK_NULL_HANDLE;
  }

  // ---- Shader modules ----
  std::vector<char> vsBytes;
  std::vector<char> fsBytes;

  if (!ReadFileBytes("shaders/pbr.vert.spv", vsBytes)) {
    std::cerr << "Missing vertex shader shaders/pbr.vert.spv" << std::endl;
    std::abort();
  }
  if (!ReadFileBytes("shaders/pbr.frag.spv", fsBytes)) {
    std::cerr << "Missing fragment shader shaders/pbr.frag.spv" << std::endl;
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

  VkPipelineShaderStageCreateInfo stages[2]{};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vs;
  stages[0].pName = "main";

  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = fs;
  stages[1].pName = "main";

  // ---- Vertex input ----
  VkVertexInputBindingDescription binding{};
  binding.binding = 0;
  binding.stride = vertexDescriptor.vertexStride();
  binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  auto vertexAttributes = vertexDescriptor.vertexAttributes();
  std::vector<VkVertexInputAttributeDescription> attrs;

  uint32_t offset = 0;
  for (int i = 0; i < (int)vertexAttributes.size(); i++) {
    auto va = vertexAttributes[i];
    uint32_t attributeSize = sizeof(float) * AttributeCount(va);

    VkVertexInputAttributeDescription ad{};
    ad.location = i;
    ad.binding = 0;
    ad.format = VulkanFormat(va);
    ad.offset = offset;
    attrs.push_back(ad);

    offset += attributeSize;
  }

  VkPipelineVertexInputStateCreateInfo vi{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vi.vertexBindingDescriptionCount = 1;
  vi.pVertexBindingDescriptions = &binding;
  vi.vertexAttributeDescriptionCount = (uint32_t)attrs.size();
  vi.pVertexAttributeDescriptions = attrs.data();

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
  rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rs.lineWidth = 1.0f;
  rs.cullMode = VK_CULL_MODE_NONE;

  VkPipelineMultisampleStateCreateInfo ms{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  ms.rasterizationSamples = renderer.sampleCount();

  VkPipelineDepthStencilStateCreateInfo ds{
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  ds.depthTestEnable = VK_TRUE;
  ds.depthWriteEnable = VK_TRUE;
  ds.depthCompareOp = VK_COMPARE_OP_LESS;

  VkPipelineColorBlendAttachmentState cbAttach{};
  cbAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                            VK_COLOR_COMPONENT_G_BIT |
                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  cbAttach.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo cb{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  cb.attachmentCount = 1;
  cb.pAttachments = &cbAttach;

  VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dyn{
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dyn.dynamicStateCount = 2;
  dyn.pDynamicStates = dynStates;

  // Push constant = model matrix.
  VkPushConstantRange pc{};
  pc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pc.offset = 0;
  pc.size = sizeof(Mat4);

  if (!frameSetLayout || *frameSetLayout == VK_NULL_HANDLE) {
    std::cerr << "createPipeline: frame descriptor set layout (set=0) is "
                 "VK_NULL_HANDLE\n";
    std::abort();
  }

  // ---- Create material descriptor set layout (set=1) if missing ----
  if (!materialSetLayout) {
    std::cerr << "createPipeline: materialSetLayout pointer is null (Scene "
                 "missing storage"
              << std::endl;
    std::abort();
  }

  // ---- Pipeline layout: set 0 (frame) + set 1 (material) ----
  VkDescriptorSetLayout setLayouts[2] = {*frameSetLayout, materialSetLayout};

  VkPipelineLayoutCreateInfo pli{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pli.setLayoutCount = 2;
  pli.pSetLayouts = setLayouts;
  pli.pushConstantRangeCount = 1;
  pli.pPushConstantRanges = &pc;

  if (vkCreatePipelineLayout(device, &pli, nullptr, pipelineLayout) !=
      VK_SUCCESS) {
    std::cerr << "vkCreatePipelineLayout failed\n";
    vkDestroyShaderModule(device, vs, nullptr);
    vkDestroyShaderModule(device, fs, nullptr);
    std::abort();
  }

  // ---- Create graphics pipeline ----
  VkGraphicsPipelineCreateInfo gp{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  gp.stageCount = 2;
  gp.pStages = stages;

  gp.pVertexInputState = &vi;
  gp.pInputAssemblyState = &ia;
  gp.pViewportState = &vp;
  gp.pRasterizationState = &rs;
  gp.pMultisampleState = &ms;
  gp.pDepthStencilState = &ds;
  gp.pColorBlendState = &cb;
  gp.pDynamicState = &dyn;

  gp.layout = *pipelineLayout;
  gp.renderPass = renderer.renderPass();
  gp.subpass = 0;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gp, nullptr,
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

std::vector<Model> createModels(Renderer &renderer) {
  auto model = loadModel(
      renderer, basicVertexDescriptor(), createMaterialSetLayout(renderer),
      "./assets/model/chair/chair.gltf", "./assets/model/chair/");

  std::vector<Model> models = {model};

  return models;
}

Scene LoadScene(Renderer &renderer) {
  Scene scene;

  // First create descriptors.
  createDescriptorSetLayout(renderer, &scene);
  createDescriptorPool(renderer, &scene);

  // Next create uniform buffers.
  createUniformBuffers(renderer, &scene);

  // Next create Camera.
  auto camera = createCamera(renderer.dimensions());

  // Finally create model.
  auto models = createModels(renderer);

  scene.init(renderer, camera, models, createPipeline, destroyPipeline);

  return scene;
}

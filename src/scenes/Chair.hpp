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
  std::vector<char> vertexShaderBytes;
  std::vector<char> fragmentShaderBytes;

  if (!ReadFileBytes("shaders/pbr.vert.spv", vertexShaderBytes)) {
    std::cerr << "Missing vertex shader shaders/pbr.vert.spv" << std::endl;
    std::abort();
  }
  if (!ReadFileBytes("shaders/pbr.frag.spv", fragmentShaderBytes)) {
    std::cerr << "Missing fragment shader shaders/pbr.frag.spv" << std::endl;
    std::abort();
  }

  VkShaderModule vertexShader = CreateShaderModule(device, vertexShaderBytes);
  if (!vertexShader) {
    std::cerr << "Failed to create vertex shader module" << std::endl;
    std::abort();
  }

  VkShaderModule fragmentShader =
      CreateShaderModule(device, fragmentShaderBytes);
  if (!fragmentShader) {
    std::cerr << "Failed to create fragment shader module" << std::endl;
    std::abort();
  }

  VkPipelineShaderStageCreateInfo stages[2]{};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vertexShader;
  stages[0].pName = "main";

  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = fragmentShader;
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

  VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
  pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &binding;
  pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount =
      (uint32_t)attrs.size();
  pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions =
      attrs.data();

  VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  pipelineInputAssemblyStateCreateInfo.topology =
      VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  pipelineViewportStateCreateInfo.viewportCount = 1;
  pipelineViewportStateCreateInfo.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
  pipelineRasterizationStateCreateInfo.frontFace =
      VK_FRONT_FACE_COUNTER_CLOCKWISE;
  pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
  pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;

  VkPipelineMultisampleStateCreateInfo pipelineMultiSampleStateCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  pipelineMultiSampleStateCreateInfo.rasterizationSamples =
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
  VkPushConstantRange pushConstantsRange{};
  pushConstantsRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantsRange.offset = 0;
  pushConstantsRange.size = sizeof(Mat4);

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
  VkDescriptorSetLayout descriptorSetLayout[2] = {*frameSetLayout,
                                                  materialSetLayout};

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutCreateInfo.setLayoutCount = 2;
  pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayout;
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantsRange;

  if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr,
                             pipelineLayout) != VK_SUCCESS) {
    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragmentShader, nullptr);

    std::cerr << "vkCreatePipelineLayout failed" << std::endl;
    std::abort();
  }

  // ---- Create graphics pipeline ----
  VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  graphicsPipelineCreateInfo.stageCount = 2;
  graphicsPipelineCreateInfo.pStages = stages;

  graphicsPipelineCreateInfo.pVertexInputState =
      &pipelineVertexInputStateCreateInfo;
  graphicsPipelineCreateInfo.pInputAssemblyState =
      &pipelineInputAssemblyStateCreateInfo;
  graphicsPipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
  graphicsPipelineCreateInfo.pRasterizationState =
      &pipelineRasterizationStateCreateInfo;
  graphicsPipelineCreateInfo.pMultisampleState =
      &pipelineMultiSampleStateCreateInfo;
  graphicsPipelineCreateInfo.pDepthStencilState =
      &pipelineDepthStencilStateCreateInfo;
  graphicsPipelineCreateInfo.pColorBlendState =
      &pipelineColorBlendStateCreateInfo;
  graphicsPipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;

  graphicsPipelineCreateInfo.layout = *pipelineLayout;
  graphicsPipelineCreateInfo.renderPass = renderer.renderPass();
  graphicsPipelineCreateInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1,
                                &graphicsPipelineCreateInfo, nullptr,
                                pipeline) != VK_SUCCESS) {
    vkDestroyShaderModule(device, vertexShader, nullptr);
    vkDestroyShaderModule(device, fragmentShader, nullptr);

    std::cerr << "vkCreateGraphicsPipelines failed" << std::endl;
    std::abort();
  }

  vkDestroyShaderModule(device, vertexShader, nullptr);
  vkDestroyShaderModule(device, fragmentShader, nullptr);
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

VkDescriptorSetLayout CreateSkyDescriptorSetLayout(VkDevice device) {
  VkDescriptorSetLayoutBinding bindings[3]{};

  // binding 0: BRDF LUT (2D)
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // binding 1: diffuse irradiance cube
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  // binding 2: specular prefiltered cube (mipped)
  bindings[2].binding = 2;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[2].descriptorCount = 1;
  bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  descriptorSetLayoutCreateInfo.bindingCount = 3;
  descriptorSetLayoutCreateInfo.pBindings = bindings;

  VkDescriptorSetLayout layout = VK_NULL_HANDLE;
  if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo,
                                  nullptr, &layout) != VK_SUCCESS) {
    return VK_NULL_HANDLE;
  }

  return layout;
}

Sky *createSky(Renderer &renderer) { return nullptr; }

Scene LoadScene(Renderer &renderer) {
  Scene scene;

  // First create descriptors.
  createDescriptorSetLayout(renderer, &scene);
  createDescriptorPool(renderer, &scene);

  // Next create uniform buffers.
  createUniformBuffers(renderer, &scene);

  // Next create Camera.
  auto camera = createCamera(renderer.dimensions());

  // Next create model.
  auto models = createModels(renderer);

  // Finally create sky.
  Sky *sky = createSky(renderer);

  scene.init(renderer, camera, models, sky, createPipeline, destroyPipeline);

  return scene;
}

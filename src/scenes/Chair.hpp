#pragma once

#include "../Vulkan.hpp"

#include "../engine/Camera.hpp"
#include "../engine/Cube.hpp"
#include "../engine/Dimensions.hpp"
#include "../engine/Loader.hpp"
#include "../engine/Renderer.hpp"
#include "../engine/Scene.hpp"

#include <memory>
#include <vector>

Scene LoadScene(Renderer &renderer) {
  Scene scene;

  //   // First create descriptors
  //   createDescriptorSetLayout(renderer, &scene);
  //   createDescriptorPool(renderer, &scene);

  //   // Next create uniform buffers
  //   createUniformBuffers(renderer, &scene);

  //   // Finally create renderable
  //   createMesh(renderer, &scene);

  //   // TODO: Setup Renderables
  //   std::vector<Renderable> renderables{};

  //   auto camera = createCamera(renderer.dimensions());

  //   scene.init(renderer, camera, renderables, createPipeline,
  //   destroyPipeline);

  loadModel("./assets/model/chair/chair.glb");

  return scene;
}

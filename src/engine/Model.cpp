#include "Model.hpp"
#include "Renderer.hpp"

#include <iostream>

void Model::init(std::vector<Mesh> meshes) { m_meshes = meshes; }

std::vector<Mesh> &Model::meshes() { return m_meshes; }

void Model::clear(Renderer &renderer, VkDescriptorPool descriptorPool) {
  for (auto mesh : m_meshes) {
    mesh.clear(renderer, descriptorPool);
  }
}

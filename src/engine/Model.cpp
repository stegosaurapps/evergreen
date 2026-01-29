#include "Model.hpp"

#include <iostream>

void Model::init(std::vector<Mesh> meshes) { m_meshes = meshes; }

std::vector<Mesh> &Model::meshes() { return m_meshes; }

void Model::clear() {}

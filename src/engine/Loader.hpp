#pragma once

#ifndef CGLTF_IMPLEMENTATION
#define CGLTF_IMPLEMENTATION
#endif

#include <cgltf/cgltf.h>

#include <iostream>

void loadModel(const char *filePath) {
  cgltf_options options{};
  cgltf_data *data = nullptr;

  cgltf_result result = cgltf_parse_file(&options, filePath, &data);
  if (result != cgltf_result_success) {
    std::cerr << "cgltf_parse_file " << filePath << " failed: " << (int)result
              << std::endl;
    std::abort();
  }

  // For .glb, this is usually enough, but still call it to be safe.
  // Base dir resolves external resources if any exist.
  result = cgltf_load_buffers(&options, data, /*base_dir*/ nullptr);
  if (result != cgltf_result_success) {
    std::cerr << "cgltf_load_buffers " << filePath << " failed: " << (int)result
              << std::endl;
    std::abort();
  }

  result = cgltf_validate(data);
  if (result != cgltf_result_success) {
    std::cerr << "cgltf_validate " << filePath << " failed: " << (int)result
              << std::endl;
    std::abort();
  }

  std::cout << "Meshes: " << data->meshes_count << "\n";

  std::cout << "I loaded the chair!!!" << std::endl;
}

cmake_minimum_required(VERSION 3.10)

project(evergreen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_package(Threads REQUIRED)

# Vulkan SDK root
if(NOT DEFINED ENV{VULKAN_SDK})
  message(FATAL_ERROR "VULKAN_SDK environment variable is not set. Example: C:\\VulkanSDK\\1.4.335.0")
endif()
set(VULKAN_SDK "$ENV{VULKAN_SDK}")

# Pick correct lib folder for x64 vs Win32
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(SDL3_LIB_DIR "${VULKAN_SDK}/Lib")
else()
  set(SDL3_LIB_DIR "${VULKAN_SDK}/Lib32")
endif()

# Sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
  src/*.cpp
  main.cpp
)

add_executable(evergreen ${SOURCES})

target_compile_definitions(evergreen PRIVATE VK_USE_PLATFORM_WIN32_KHR)

target_include_directories(evergreen PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/third_party
  "${VULKAN_SDK}/Include"     # provides vulkan/ and SDL3/ headers (SDK)
)

# --- Shaders (GLSL -> SPIR-V using Vulkan SDK glslc) ---
set(GLSLC "${VULKAN_SDK}/Bin/glslc.exe")

set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_OUT_DIR "${CMAKE_SOURCE_DIR}/shaders")

set(SHADERS
  "${SHADER_SRC_DIR}/pbr.vert"
  "${SHADER_SRC_DIR}/pbr.frag"
)

set(SHADER_SPV
  "${SHADER_OUT_DIR}/pbr.vert.spv"
  "${SHADER_OUT_DIR}/pbr.frag.spv"
)

add_custom_command(
  OUTPUT ${SHADER_SPV}
  COMMAND "${GLSLC}" "${SHADER_SRC_DIR}/pbr.vert" -o "${SHADER_OUT_DIR}/pbr.vert.spv"
  COMMAND "${GLSLC}" "${SHADER_SRC_DIR}/pbr.frag" -o "${SHADER_OUT_DIR}/pbr.frag.spv"
  DEPENDS ${SHADERS}
  COMMENT "Compiling shaders with glslc"
  VERBATIM
)

add_custom_target(evergreen_shaders ALL DEPENDS ${SHADER_SPV})
add_dependencies(evergreen evergreen_shaders)

# Static SDL build expects this define
target_compile_definitions(evergreen PRIVATE SDL_STATIC_LIB)

# Let the linker find SDL3-static*.lib
target_link_directories(evergreen PRIVATE "${SDL3_LIB_DIR}")

# Link SDL3 statically (Debug/Release split)
target_link_libraries(evergreen PRIVATE
  Vulkan::Vulkan
  Threads::Threads
  $<$<CONFIG:Debug>:SDL3-staticd>
  $<$<NOT:$<CONFIG:Debug>>:SDL3-static>
)

# SDL on Windows commonly needs some system libs (safe to include)
target_link_libraries(evergreen PRIVATE
  user32
  gdi32
  shell32
  ole32
  oleaut32
  imm32
  winmm
  version
  setupapi
)

message(STATUS "VULKAN_SDK   = ${VULKAN_SDK}")
message(STATUS "SDL3_LIB_DIR = ${SDL3_LIB_DIR}")

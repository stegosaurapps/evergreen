@echo off
setlocal

set VULKAN_SDK=C:\VulkanSDK\1.4.335.0
set VCPKG_TOOLCHAIN=C:/Users/bonha/Documents/vcpkg-master/scripts/buildsystems/vcpkg.cmake

set BUILD_DIR=build/vs2019-x64

cmake -S . -B %BUILD_DIR% -G "Visual Studio 16 2019" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_TOOLCHAIN%

cmake --build %BUILD_DIR% --config Debug

endlocal

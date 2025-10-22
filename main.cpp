#include <iostream>
#include <cstring>
#include <cstdint>

#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "engine/engine.hpp"

// Example resolution
const int WIDTH = 800;
const int HEIGHT = 600;

#ifndef GL_UNSIGNED_INT_8_8_8_8_REV
#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367
#endif

// RGBA pixel buffer, 4 bytes per pixel
uint32_t framebuffer[WIDTH * HEIGHT];

int main() {
    std::cout << "Test..." << std::endl;

    glm::vec3 myVector(1.0f, 2.0f, 3.0f);

    Engine engine;

    engine.Init();
    engine.Run();
    engine.Shutdown();

    if (!glfwInit()) {
        std::cerr << "GLFW init failed!" << std::endl;
        return -1;
    }

    // Clear screen to black
    std::memset(framebuffer, 0, sizeof(framebuffer));

    // Draw red pixel at center
    int centerX = WIDTH / 2;
    int centerY = HEIGHT / 2;
    framebuffer[centerY * WIDTH + centerX] = 0xFF0000FF; // RGBA: Red

    GLFWwindow* window = glfwCreateWindow(800, 600, "evergreen", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {
        // Clear framebuffer
        std::memset(framebuffer, 0, sizeof(framebuffer));

        // Draw red pixel in center
        int centerX = WIDTH / 2;
        int centerY = HEIGHT / 2;
        framebuffer[centerY * WIDTH + centerX] = 0xFF0000FF; // RGBA

        // Draw it to screen
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawPixels(WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, framebuffer);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();


    std::cout << glm::to_string(myVector) << std::endl;

    return 0;
}

#include <iostream>
#include <cstring>

#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include "engine/engine.hpp"

// Example resolution
const int WIDTH = 800;
const int HEIGHT = 600;

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

    GLFWwindow* window = glfwCreateWindow(800, 600, "evergreen", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed!" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();


    std::cout << glm::to_string(myVector) << std::endl;

    return 0;
}

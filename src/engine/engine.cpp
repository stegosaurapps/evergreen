#include <iostream>

#include "engine.hpp"

Engine::Engine() : isRunning(false) {
    std::cout << "[Engine] Constructor called.\n";
}

Engine::~Engine() {
    std::cout << "[Engine] Destructor called.\n";
}

void Engine::Init() {
    std::cout << "[Engine] Initialization...\n";
    isRunning = true;
}

void Engine::Run() {
    std::cout << "[Engine] Running...\n";
    while (isRunning) {
        // Placeholder: Main loop content here
        isRunning = false; // Temporary: exit after one frame
    }
}

void Engine::Shutdown() {
    std::cout << "[Engine] Shutting down.\n";
    isRunning = false;
}

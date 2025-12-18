#include "src/engine/Engine.hpp"

#include <iostream>

int main(int, char**) {
    Engine engine;
    if (!engine.init()) {
        std::cout << "Failed to initialize engine" << std::endl;

        return 1;
    }
    
    return engine.run();
}

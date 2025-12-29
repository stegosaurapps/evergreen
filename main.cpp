#include "src/engine/Engine.hpp"

#include <iostream>

int main(int, char**) {
    Engine engine;
    if (!engine.init()) {
        return 1;
    }

    return engine.run();
}

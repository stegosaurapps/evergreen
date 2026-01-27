#include "src/engine/Engine.hpp"
#include "src/scenes/Basic.hpp"
// #include "src/scenes/Chair.hpp"

#include <iostream>

int main(int, char**) {
    Engine engine;
    if (!engine.init()) {
        return 1;
    }

    engine.loadScene(LoadScene(engine.renderer()));

    engine.run();

    // LoadScene(engine.renderer());

    return 0;
}

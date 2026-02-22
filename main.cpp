#include "src/engine/Engine.hpp"
// #include "src/scenes/Basic.hpp"
#include "src/scenes/Chair.hpp"

#include <iostream>

int main(int, char**) {
    Engine engine;
    if (!engine.init()) {
        return 1;
    }

    // Dynamically load a scene to render.
    engine.loadScene(LoadScene(engine.renderer()));

    // The main loop will run until program is ready to close.
    engine.run();

    // Trigger all cleanup.
    engine.clear();

    return 0;
}

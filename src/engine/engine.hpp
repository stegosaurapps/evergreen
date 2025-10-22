#pragma once

class Engine {
public:
    Engine();
    ~Engine();

    void Init();
    void Run();
    void Shutdown();

private:
    bool isRunning;
};

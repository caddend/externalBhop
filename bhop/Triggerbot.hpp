#pragma once
#include <windows.h>
#include <thread>
#include "GameMemory.hpp"

class Triggerbot {
private:
    GameMemory& memory;
    bool isTriggerRunning = false;
    std::thread triggerThread;
    int triggerKey = VK_MENU;

public:
    Triggerbot(GameMemory& mem) : memory(mem) {}
    ~Triggerbot() { Stop(); }

    void Start(int vKey);
    void Stop();
    void TriggerLoop();
};
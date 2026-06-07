#include "Triggerbot.hpp"
#include <iostream>
#include <chrono>

#pragma comment(lib, "winmm.lib")

void Triggerbot::Start(int vKey) {
    if (isTriggerRunning) return;
    triggerKey = vKey;
    isTriggerRunning = true;
    triggerThread = std::thread(&Triggerbot::TriggerLoop, this);
}

void Triggerbot::Stop() {
    isTriggerRunning = false;
    if (triggerThread.joinable()) {
        triggerThread.join();
    }
}

void Triggerbot::TriggerLoop() {
    std::cout << "[Triggerbot] Запущен, клавиша: " << triggerKey << std::endl;

    int lastHandle = 0;
    int shotCounter = 0;

    while (isTriggerRunning) {
        if (GetAsyncKeyState(triggerKey) & 0x8000) {

            uintptr_t localPawn = memory.Read<uintptr_t>(memory.clientAddress + offsets::dwLocalPlayerPawn);

            if (localPawn) {
                int crosshairHandle = memory.Read<int>(localPawn + offsets::m_iIDEntIndex);

                if (crosshairHandle != lastHandle && crosshairHandle != 0xFFFFFFFF && crosshairHandle != 0) {
                    std::cout << "[Triggerbot] Handle: 0x" << std::hex << crosshairHandle << std::dec << std::endl;
                    lastHandle = crosshairHandle;
                }

                if (crosshairHandle > 0 && crosshairHandle != 0xFFFFFFFF && crosshairHandle != 0) {
                    shotCounter++;
                    std::cout << "[Triggerbot] >>> ВЫСТРЕЛ #" << shotCounter << " <<<" << std::endl;

                    INPUT input = { 0 };
                    input.type = INPUT_MOUSE;
                    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                    SendInput(1, &input, sizeof(INPUT));

                    Sleep(8);

                    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                    SendInput(1, &input, sizeof(INPUT));

                    Sleep(120);
                }
            }

            Sleep(5);
        }
        else {
            Sleep(5);
        }
    }

    std::cout << "[Triggerbot] Остановлен" << std::endl;
}
#include "Triggerbot.hpp"
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
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    timeBeginPeriod(1);

    while (isTriggerRunning) {
        if (GetAsyncKeyState(triggerKey) & 0x8000) {

            uintptr_t localPawn = memory.Read<uintptr_t>(memory.clientAddress + offsets::dwLocalPlayerPawn);

            if (localPawn) {
                // Читаем индекс объекта под прицелом
                int crosshairHandle = memory.Read<int>(localPawn + offsets::m_iIDEntIndex);
                int crosshairId = crosshairHandle & 0x1FF;

                // Если прицел зацепил ЛЮБОГО игрока или бота (индексы от 1 до 128)
                if (crosshairHandle > 0 && crosshairId > 0 && crosshairId <= 128) {

                    // МГНОВЕННЫЙ ВЫСТРЕЛ ЧЕРЕЗ СИМУЛЯЦИЮ КЛИКА МЫШИ (mouse_event)
                    // Этот метод пробивает любую блокировку оконного ввода в Windows
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Держим ЛКМ 10 мс
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);

                    // Кулдаун между выстрелами, чтобы не было дикого зажима
                    std::this_thread::sleep_for(std::chrono::milliseconds(180));
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
        }
    }

    timeEndPeriod(1);
}
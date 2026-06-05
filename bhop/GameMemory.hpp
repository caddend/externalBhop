#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>

// Динамическая структура оффсетов в памяти C++
namespace offsets {
    inline uintptr_t dwEntityList = 0;
    inline uintptr_t dwLocalPlayerController = 0;
    inline uintptr_t dwLocalPlayerPawn = 0;
    inline uintptr_t m_fFlags = 0;

    // Статичные внутренние смещения контроллера
    namespace controller {
        constexpr uintptr_t m_hPawn = 0x6BC;
    }
}

struct Vector3 { float x, y, z; };

class GameMemory {
private:
    HANDLE hProcess = nullptr;
    DWORD processId = 0;
    bool isBhopRunning = false;
    std::thread bhopThread;

    uintptr_t GetModuleBaseAddress(const wchar_t* moduleName);

public:
    uintptr_t clientAddress = 0;

    GameMemory() = default;
    ~GameMemory() { Detach(); }

    bool RunAutoupdater();  // Метод запуска питона
    bool LoadOffsetsTxt();  // Метод парсинга txt
    bool Attach(const wchar_t* processName);
    void Detach();

    template <typename T>
    T Read(uintptr_t address) {
        T buffer{};
        if (address && hProcess) {
            ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(address), &buffer, sizeof(T), nullptr);
        }
        return buffer;
    }

    void StartBhop();
    void StopBhop();
    void BhopLoop();
};
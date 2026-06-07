#include "GameMemory.hpp"
#include <chrono>

#pragma comment(lib, "winmm.lib")

bool GameMemory::RunAutoupdater() {
    CreateDirectoryA(".\\dumper_data", NULL);
    std::cout << "[+] Запуск встроенного скрипта обновления оффсетов..." << std::endl;
    return true;
}

bool GameMemory::LoadOffsetsTxt() {
    std::ifstream file(".\\dumper_data\\offsets.txt");
    if (!file.is_open()) {
        std::cout << "[-] Ошибка: Не удалось открыть файл offsets.txt!" << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string key;
        std::string valueStr;

        if (std::getline(ss, key, '=') && std::getline(ss, valueStr)) {
            uintptr_t value = std::stoull(valueStr, nullptr, 0);

            if (key == "dwEntityList") offsets::dwEntityList = value;
            else if (key == "dwLocalPlayerController") offsets::dwLocalPlayerController = value;
            else if (key == "dwLocalPlayerPawn") offsets::dwLocalPlayerPawn = value;
            else if (key == "m_fFlags") offsets::m_fFlags = value;
            else if (key == "m_iIDEntIndex") offsets::m_iIDEntIndex = value;
            else if (key == "m_iHealth") offsets::m_iHealth = value;
            else if (key == "m_iTeamNum") offsets::m_iTeamNum = value;
            else if (key == "m_vecViewOffset") offsets::m_vecViewOffset = value;
            else if (key == "m_hPlayerPawn") offsets::m_hPlayerPawn = value;
            else if (key == "m_hPawn") offsets::m_hPawn = value;
        }
    }
    file.close();

    std::cout << "[*] Проверка загруженных оффсетов:" << std::endl;
    std::cout << "  -> dwEntityList: 0x" << std::hex << offsets::dwEntityList << std::endl;
    std::cout << "  -> dwLocalPlayerPawn: 0x" << offsets::dwLocalPlayerPawn << std::endl;
    std::cout << "  -> m_fFlags: 0x" << offsets::m_fFlags << std::endl;
    std::cout << "  -> m_iIDEntIndex: 0x" << offsets::m_iIDEntIndex << std::endl;
    std::cout << "  -> m_iHealth: 0x" << offsets::m_iHealth << std::endl;
    std::cout << "  -> m_iTeamNum: 0x" << offsets::m_iTeamNum << std::dec << std::endl;

    if (offsets::dwLocalPlayerPawn == 0 || offsets::m_fFlags == 0) {
        std::cout << "[-] Ошибка: Базовые оффсеты равны нулю!" << std::endl;
        return false;
    }

    return true;
}

bool GameMemory::Attach(const wchar_t* processName) {
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;

    bool found = false;
    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (wcscmp(pe32.szExeFile, processName) == 0) {
                processId = pe32.th32ProcessID;
                found = true;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }
    CloseHandle(hSnapshot);

    if (!found) return false;

    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (!hProcess) return false;

    clientAddress = GetModuleBaseAddress(L"client.dll");
    return (clientAddress != 0);
}

void GameMemory::Detach() {
    StopBhop();
    if (hProcess) {
        CloseHandle(hProcess);
        hProcess = nullptr;
    }
}

uintptr_t GameMemory::GetModuleBaseAddress(const wchar_t* moduleName) {
    uintptr_t addr = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W me32;
        me32.dwSize = sizeof(MODULEENTRY32W);
        if (Module32FirstW(hSnapshot, &me32)) {
            do {
                if (wcscmp(me32.szModule, moduleName) == 0) {
                    addr = reinterpret_cast<uintptr_t>(me32.modBaseAddr);
                    break;
                }
            } while (Module32NextW(hSnapshot, &me32));
        }
        CloseHandle(hSnapshot);
    }
    return addr;
}

void GameMemory::StartBhop() {
    if (isBhopRunning) return;
    isBhopRunning = true;
    bhopThread = std::thread(&GameMemory::BhopLoop, this);
}

void GameMemory::StopBhop() {
    isBhopRunning = false;
    if (bhopThread.joinable()) {
        bhopThread.join();
    }
}

void GameMemory::BhopLoop() {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    timeBeginPeriod(1);

    HWND gameWindow = FindWindowW(L"SDL_app", L"Counter-Strike 2");
    if (!gameWindow) gameWindow = FindWindowW(nullptr, L"Counter-Strike 2");

    LPARAM LParamDown = 1 | (0x39 << 16);
    LPARAM LParamUp = 1 | (0x39 << 16) | (1 << 30) | (1 << 31);

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    while (isBhopRunning) {
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {

            uintptr_t localPawn = Read<uintptr_t>(clientAddress + offsets::dwLocalPlayerPawn);

            if (localPawn && gameWindow) {
                int flags = Read<int>(localPawn + offsets::m_fFlags);

                if (flags & (1 << 0)) {
                    PostMessageA(gameWindow, WM_KEYDOWN, VK_SPACE, LParamDown);

                    // Óëüòðà-òî÷íûé àïïàðàòíûé ñïèí-ëîê óäåðæàíèÿ íà 4ìñ
                    LARGE_INTEGER start, current;
                    QueryPerformanceCounter(&start);
                    long long ticksToWait = (frequency.QuadPart * 4000) / 1000000;

                    do {
                        QueryPerformanceCounter(&current);
                    } while ((current.QuadPart - start.QuadPart) < ticksToWait);

                    PostMessageA(gameWindow, WM_KEYUP, VK_SPACE, LParamUp);

                    // Êóëäàóí ïðîïóñêà ïðîñàäîê ñóáòèêà
                    std::this_thread::sleep_for(std::chrono::milliseconds(12));
                }
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    timeEndPeriod(1);
}
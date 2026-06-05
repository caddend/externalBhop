#include "GameMemory.hpp"
#pragma comment(lib, "winmm.lib")
// Запуск скрипта автоматического обновления оффсетов
bool GameMemory::RunAutoupdater() {
    CreateDirectoryA(".\\dumper_data", NULL);

    std::cout << "[+] Запуск встроенного скрипта обновления оффсетов..." << std::endl;

    std::string command = "python .\\dumper_data\\parser.py";
    int result = system(command.c_str());
    if (result != 0) {
        command = "python3 .\\dumper_data\\parser.py";
        result = system(command.c_str());
    }

    return (result == 0);
}

// Чтение текстового файла конфигурации, созданного Python
bool GameMemory::LoadOffsetsTxt() {
    std::ifstream file(".\\dumper_data\\offsets.txt");
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line); // ИСПРАВЛЕНО: убрана лишняя 'i'
        std::string key;
        std::string valueStr;

        if (std::getline(ss, key, '=') && std::getline(ss, valueStr)) {
            uintptr_t value = std::stoull(valueStr, nullptr, 0);

            if (key == "dwEntityList") offsets::dwEntityList = value;
            else if (key == "dwLocalPlayerController") offsets::dwLocalPlayerController = value;
            else if (key == "dwLocalPlayerPawn") offsets::dwLocalPlayerPawn = value;
            else if (key == "m_fFlags") offsets::m_fFlags = value;
        }
    }
    file.close();

    return (offsets::dwLocalPlayerPawn != 0 && offsets::m_fFlags != 0);
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

#pragma region BunnyHop

// Подключаем необходимые заголовки для высокоточных таймеров процессора
#include <chrono>

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
    // 1. Форсируем максимальный приоритет потока реального времени в ОС
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    // 2. Переводим системный таймер Windows в режим максимальной точности (1 мс)
    // Без этого sleep_for() округляет задержки до 15.6 миллисекунд
    timeBeginPeriod(1);

    HWND gameWindow = FindWindowW(L"SDL_app", L"Counter-Strike 2");
    if (!gameWindow) gameWindow = FindWindowW(nullptr, L"Counter-Strike 2");

    LPARAM LParamDown = 1 | (0x39 << 16);
    LPARAM LParamUp = 1 | (0x39 << 16) | (1 << 30) | (1 << 31);

    // Получаем частоту аппаратного таймера процессора для Spin-lock
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    while (isBhopRunning) {
        if (GetAsyncKeyState(VK_SPACE) & 0x8000) {

            uintptr_t localPawn = Read<uintptr_t>(clientAddress + offsets::dwLocalPlayerPawn);

            if (localPawn && gameWindow) {
                int flags = Read<int>(localPawn + offsets::m_fFlags);

                // Проверяем флаг приземления FL_ONGROUND
                if (flags & (1 << 0)) {

                    // Шлем идеальный клик в момент коллизии с землей
                    PostMessageA(gameWindow, WM_KEYDOWN, VK_SPACE, LParamDown);

                    // Высокоточный аппаратный спин-лок на 4 миллисекунды (время удержания клавиши)
                    // Это гарантирует, что кнопка отожмется ровно тогда, когда нужно, без задержек ОС
                    LARGE_INTEGER start, current;
                    QueryPerformanceCounter(&start);
                    long long ticksToWait = (frequency.QuadPart * 4000) / 1000000; // 4000 микросекунд = 4 мс

                    do {
                        QueryPerformanceCounter(&current);
                    } while ((current.QuadPart - start.QuadPart) < ticksToWait);

                    // Отжимаем клавишу
                    PostMessageA(gameWindow, WM_KEYUP, VK_SPACE, LParamUp);

                    // Кулдаун: пока моделька летит вверх, спим 12 мс на уровне ядра ОС,
                    // чтобы разгрузить процессор и не поймать штраф скорости (Speed Penalty)
                    std::this_thread::sleep_for(std::chrono::milliseconds(12));
                }
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    // Возвращаем настройки таймера Windows в исходное состояние при выходе
    timeEndPeriod(1);
}

#pragma endregion
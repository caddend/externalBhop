#include <iostream>
#include "GameMemory.hpp"

int main() {
    setlocale(LC_ALL, "Russian");
    GameMemory memory;

    if (memory.RunAutoupdater()) {
        std::cout << "[+] Питон успешно отработал скрипт." << std::endl;
    }
    else {
        std::cout << "[-] Предупреждение: Не удалось запустить автоапдейтер. Пробуем загрузить старый кэш..." << std::endl;
    }

    if (!memory.LoadOffsetsTxt()) {
        std::cout << "[-] Критическая ошибка: Не удалось считать offsets.txt! Закрытие." << std::endl;
        std::cin.get();
        return -1;
    }

    std::cout << "[+] Оффсеты успешно загружены в память приложения!" << std::endl;
    std::cout << "-> dwLocalPlayerPawn: 0x" << std::hex << offsets::dwLocalPlayerPawn << std::endl;
    std::cout << "-> m_fFlags: 0x" << std::hex << offsets::m_fFlags << std::endl;

    std::cout << "\n[+] Ожидание запуска Counter-Strike 2..." << std::endl;
    while (!memory.Attach(L"cs2.exe")) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[+] Успешно подключено к игре!" << std::endl;
    std::cout << "Client Base Address: 0x" << std::hex << memory.clientAddress << std::endl; // ИСПРАВЛЕНО: убран не существующий engineAddress

    memory.StartBhop();
    std::cout << "[+] Бхоп ебашит на полную. Удерживай ПРОБЕЛ. Для выхода нажми [END]." << std::endl;

    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {
        // ИСПРАВЛЕНО: убран не существующий в текущей версии хедера UpdateEntityList()
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    memory.StopBhop();
    memory.Detach();
    return 0;
}
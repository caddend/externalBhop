#include <iostream>
#include <string>
#include "GameMemory.hpp"
#include "Triggerbot.hpp"

int ParseUserKey(const std::string& input) {
    if (input == "1") return VK_MENU;       // ALT
    if (input == "2") return VK_SHIFT;      // SHIFT
    if (input == "3") return VK_XBUTTON1;   // Mouse 4
    if (input == "4") return VK_XBUTTON2;   // Mouse 5
    if (input == "5") return VK_CONTROL;    // CTRL

    if (input.length() == 1) {
        char ch = input[0];
        if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
        if (ch >= 'A' && ch <= 'Z') return ch;
    }

    std::cout << "[-] Ошибка ввода, назначен стандартный ALT" << std::endl;
    return VK_MENU;
}

int main() {
    setlocale(LC_ALL, "Russian");
    GameMemory memory;
    Triggerbot trigger(memory);

    if (memory.RunAutoupdater()) {
        std::cout << "[+] Питон успешно обновил структуры." << std::endl;
    }
    else {
        std::cout << "[-] Предупреждение: Скрипт не отработал. Загрузка старого кэша..." << std::endl;
    }

    if (!memory.LoadOffsetsTxt()) {
        std::cout << "[-] Критическая ошибка: Не удалось считать данные из offsets.txt!" << std::endl;
        std::cin.get();
        return -1;
    }

    std::cout << "\n=== НАСТРОЙКА ТРИГГЕРБОТА ===" << std::endl;
    std::cout << "Выберите кнопку для триггербота:" << std::endl;
    std::cout << "1 - ALT\n2 - SHIFT\n3 - Mouse 4\n4 - Mouse 5\nИли введите любую букву (например: F): ";

    std::string userInput;
    std::cin >> userInput;
    int selectedTriggerKey = ParseUserKey(userInput);

    std::cout << "\n[+] Ожидание запуска Counter-Strike 2..." << std::endl;
    while (!memory.Attach(L"cs2.exe")) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[+] Успешно подключено к процессу!" << std::endl;
    std::cout << "Base Client Address: 0x" << std::hex << memory.clientAddress << std::endl;

    // Запускаем бхоп и триггербот в параллельных потоках
    memory.StartBhop();
    trigger.Start(selectedTriggerKey);

    std::cout << "\n[+] ВСЁ ЗАПУЩЕНО И ГОТОВО К БОЮ!" << std::endl;
    std::cout << "-> Бхоп работает на ПРОБЕЛ (Абсолютный хай-пресижн)" << std::endl;
    std::cout << "-> Триггербот работает на выбранную кнопку" << std::endl;
    std::cout << "Для закрытия софта зажми кнопку [END] прямо в игре." << std::endl;

    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    trigger.Stop();
    memory.StopBhop();
    memory.Detach();

    std::cout << "[+] Работа софта завершена." << std::endl;
    return 0;
}
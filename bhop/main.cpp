#include <iostream>
#include <string>
#include "GameMemory.hpp"
#include "Triggerbot.hpp"
#include "Updater.hpp"

int ParseUserKey(const std::string& input) {
    if (input == "1") return VK_MENU;
    if (input == "2") return VK_SHIFT;
    if (input == "3") return VK_XBUTTON1;
    if (input == "4") return VK_XBUTTON2;
    if (input == "5") return VK_CONTROL;

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

    // Автообновление оффсетов
    Updater updater;
    if (!updater.UpdateOffsets()) {
        std::cout << "[-] КРИТИЧЕСКАЯ ОШИБКА: Не удалось обновить оффсеты!" << std::endl;
        std::cout << "[-] Проверьте подключение к интернету и перезапустите программу." << std::endl;
        std::cin.get();
        return -1;
    }

    GameMemory memory;

    if (!memory.LoadOffsetsTxt()) {
        std::cout << "[-] Критическая ошибка: Не удалось загрузить offsets.txt!" << std::endl;
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
    std::cout << "Base Client Address: 0x" << std::hex << memory.clientAddress << std::dec << std::endl;

    memory.StartBhop();

    Triggerbot trigger(memory);
    trigger.Start(selectedTriggerKey);

    std::cout << "\n[+] ВСЁ ЗАПУЩЕНО!" << std::endl;
    std::cout << "-> Бхоп работает на ПРОБЕЛ" << std::endl;
    std::cout << "-> Триггербот работает на выбранную кнопку" << std::endl;
    std::cout << "Для закрытия зажми [END]" << std::endl;

    while (!(GetAsyncKeyState(VK_END) & 0x8000)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    trigger.Stop();
    memory.StopBhop();
    memory.Detach();

    std::cout << "[+] Работа завершена." << std::endl;
    return 0;
}
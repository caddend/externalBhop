#pragma once
#include <string>
#include <windows.h>
#include <wininet.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <direct.h>
#include <fileapi.h>
#include <tlhelp32.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "version.lib")

class Updater {
private:
    std::string versionFile = "dumper_data\\version.txt";
    std::string dumperPath = "dumper_data\\cs2-dumper.exe";
    std::string outputDir = "dumper_data\\output";
    std::string offsetsFile = "dumper_data\\offsets.txt";

    const std::string GITHUB_API = "https://api.github.com/repos/a2x/cs2-dumper/releases/latest";
    const std::string USER_AGENT = "CS2-Offset-Updater/1.0";

    // Правильные оффсеты для CS2 (актуальные)
    const uintptr_t CORRECT_MFFLAGS = 0x3F8;
    const uintptr_t CORRECT_MIIDENTINDEX = 0x33C;
    const uintptr_t CORRECT_MIHEALTH = 0x34C;
    const uintptr_t CORRECT_MITEAMNUM = 0x3EB;
    const uintptr_t CORRECT_MVECVIEWOFFSET = 0xE70;
    const uintptr_t CORRECT_MHPLAYERPAWN = 0x6BC;

    std::string HttpGet(const std::string& url) {
        HINTERNET hSession = InternetOpenA(USER_AGENT.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hSession) return "";

        HINTERNET hConnect = InternetOpenUrlA(hSession, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect) {
            InternetCloseHandle(hSession);
            return "";
        }

        std::string response;
        char buffer[4096];
        DWORD bytesRead;
        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            response.append(buffer, bytesRead);
        }

        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return response;
    }

    std::string ExtractVersion(const std::string& json) {
        size_t pos = json.find("\"tag_name\"");
        if (pos == std::string::npos) return "";

        pos = json.find("\"", pos + 10);
        if (pos == std::string::npos) return "";

        size_t end = json.find("\"", pos + 1);
        if (end == std::string::npos) return "";

        return json.substr(pos + 1, end - pos - 1);
    }

    std::string FindExeDownloadUrl(const std::string& json) {
        std::string search = "\"browser_download_url\"";
        size_t pos = 0;

        while ((pos = json.find(search, pos)) != std::string::npos) {
            size_t colonPos = json.find(":", pos);
            if (colonPos == std::string::npos) break;

            size_t urlStart = json.find("\"", colonPos + 1);
            if (urlStart == std::string::npos) break;

            size_t urlEnd = json.find("\"", urlStart + 1);
            if (urlEnd == std::string::npos) break;

            std::string url = json.substr(urlStart + 1, urlEnd - urlStart - 1);

            if (url.find("cs2-dumper.exe") != std::string::npos) {
                return url;
            }

            pos = urlEnd;
        }

        return "";
    }

    bool DownloadFile(const std::string& url, const std::string& path) {
        HINTERNET hSession = InternetOpenA(USER_AGENT.c_str(), INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (!hSession) return false;

        HINTERNET hConnect = InternetOpenUrlA(hSession, url.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hConnect) {
            InternetCloseHandle(hSession);
            return false;
        }

        HANDLE hFile = CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hSession);
            return false;
        }

        DWORD bytesRead;
        char buffer[8192];
        while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
            DWORD bytesWritten;
            WriteFile(hFile, buffer, bytesRead, &bytesWritten, NULL);
        }

        CloseHandle(hFile);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hSession);
        return true;
    }

    std::string GetLocalVersion() {
        DWORD dummy;
        DWORD size = GetFileVersionInfoSizeA(dumperPath.c_str(), &dummy);
        if (size == 0) return "";

        std::vector<char> buffer(size);
        if (!GetFileVersionInfoA(dumperPath.c_str(), 0, size, buffer.data())) return "";

        VS_FIXEDFILEINFO* pFileInfo;
        UINT len;
        if (!VerQueryValueA(buffer.data(), "\\", (LPVOID*)&pFileInfo, &len)) return "";

        char version[64];
        sprintf_s(version, "v%d.%d.%d",
            HIWORD(pFileInfo->dwFileVersionMS),
            LOWORD(pFileInfo->dwFileVersionMS),
            HIWORD(pFileInfo->dwFileVersionLS));

        return std::string(version);
    }

    void SaveVersion(const std::string& version) {
        std::ofstream file(versionFile);
        if (file.is_open()) {
            file << version;
            file.close();
        }
    }

    std::string LoadSavedVersion() {
        std::ifstream file(versionFile);
        if (!file.is_open()) return "";
        std::string version;
        std::getline(file, version);
        file.close();
        return version;
    }

    void KillDumperProcess() {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(hSnapshot, &pe32)) {
            do {
                if (wcscmp(pe32.szExeFile, L"cs2-dumper.exe") == 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProcess) {
                        TerminateProcess(hProcess, 0);
                        CloseHandle(hProcess);
                    }
                }
            } while (Process32NextW(hSnapshot, &pe32));
        }
        CloseHandle(hSnapshot);
    }

public:
    bool CheckAndUpdate() {
        std::cout << "[Updater] Проверка обновлений cs2-dumper..." << std::endl;

        CreateDirectoryA("dumper_data", NULL);

        std::string jsonResponse = HttpGet(GITHUB_API);
        if (jsonResponse.empty()) {
            std::cout << "[Updater] Не удалось подключиться к GitHub. Использую локальную версию." << std::endl;
            return true;
        }

        std::string latestVersion = ExtractVersion(jsonResponse);
        if (latestVersion.empty()) {
            std::cout << "[Updater] Не удалось определить версию." << std::endl;
            return true;
        }

        std::cout << "[Updater] Последняя версия: " << latestVersion << std::endl;

        std::string localVersion = GetLocalVersion();
        std::string savedVersion = LoadSavedVersion();

        if (localVersion.empty() || localVersion != latestVersion || savedVersion != latestVersion) {
            std::cout << "[Updater] Обновление с " << (localVersion.empty() ? "none" : localVersion) << " до " << latestVersion << std::endl;

            KillDumperProcess();
            Sleep(1000);

            std::string downloadUrl = FindExeDownloadUrl(jsonResponse);
            if (downloadUrl.empty()) {
                std::cout << "[Updater] Не удалось найти ссылку для скачивания cs2-dumper.exe!" << std::endl;
                return false;
            }

            std::cout << "[Updater] Скачивание: " << downloadUrl << std::endl;

            std::string tempFile = dumperPath + ".tmp";
            if (DownloadFile(downloadUrl, tempFile)) {
                DeleteFileA(dumperPath.c_str());
                if (MoveFileA(tempFile.c_str(), dumperPath.c_str())) {
                    std::cout << "[Updater] Дампер успешно обновлён!" << std::endl;
                    SaveVersion(latestVersion);
                    return true;
                }
                else {
                    std::cout << "[Updater] Ошибка замены файла!" << std::endl;
                }
            }
            else {
                std::cout << "[Updater] Ошибка скачивания!" << std::endl;
            }
            return false;
        }

        std::cout << "[Updater] Дампер актуален: " << localVersion << std::endl;
        return true;
    }

    bool RunDumper() {
        std::cout << "[Updater] Запуск cs2-dumper..." << std::endl;

        if (GetFileAttributesA(dumperPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            std::cout << "[Updater] Дампер не найден!" << std::endl;
            return false;
        }

        system(("rmdir /s /q \"" + outputDir + "\"").c_str());
        Sleep(500);

        STARTUPINFOA si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;

        std::string cmd = "\"" + dumperPath + "\"";

        if (!CreateProcessA(NULL, (LPSTR)cmd.c_str(), NULL, NULL, FALSE,
            CREATE_NO_WINDOW, NULL, "dumper_data", &si, &pi)) {
            std::cout << "[Updater] Ошибка запуска дампера! Код: " << GetLastError() << std::endl;
            return false;
        }

        WaitForSingleObject(pi.hProcess, 30000);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        std::cout << "[Updater] Дампер завершил работу." << std::endl;
        return true;
    }

    bool ParseOffsets() {
        std::cout << "[Updater] Парсинг оффсетов..." << std::endl;

        std::string offsetsJson = outputDir + "\\offsets.json";

        if (GetFileAttributesA(offsetsJson.c_str()) == INVALID_FILE_ATTRIBUTES) {
            std::cout << "[Updater] offsets.json не найден в " << outputDir << std::endl;
            return false;
        }

        std::ifstream file(offsetsJson);
        if (!file.is_open()) return false;

        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        file.close();

        auto extractOffset = [&](const std::string& name) -> uintptr_t {
            std::string search = "\"" + name + "\":";
            size_t pos = content.find(search);
            if (pos == std::string::npos) return 0;

            pos = content.find(":", pos);
            if (pos == std::string::npos) return 0;

            while (pos < content.length() && (content[pos] == ':' || content[pos] == ' ')) pos++;

            std::string numStr;
            while (pos < content.length() && (isxdigit(content[pos]) || content[pos] == 'x')) {
                numStr += content[pos];
                pos++;
            }

            if (numStr.find("0x") == 0 || numStr.find("0X") == 0) {
                return std::stoull(numStr, nullptr, 16);
            }
            return std::stoull(numStr);
            };

        uintptr_t dwEntityList = extractOffset("dwEntityList");
        uintptr_t dwLocalPlayerController = extractOffset("dwLocalPlayerController");
        uintptr_t dwLocalPlayerPawn = extractOffset("dwLocalPlayerPawn");

        // Для остальных оффсетов читаем client_dll.json
        std::string clientJson = outputDir + "\\client_dll.json";
        std::ifstream clientFile(clientJson);

        uintptr_t m_iIDEntIndex = 0, m_iHealth = 0, m_iTeamNum = 0, m_fFlags = 0, m_vecViewOffset = 0, m_hPlayerPawn = 0;

        if (clientFile.is_open()) {
            std::string clientContent((std::istreambuf_iterator<char>(clientFile)),
                std::istreambuf_iterator<char>());
            clientFile.close();

            // Функция для поиска оффсета в конкретном классе
            auto findInClass = [&](const std::string& className, const std::string& field) -> uintptr_t {
                std::string search = "\"" + className + "\"";
                size_t classPos = clientContent.find(search);
                if (classPos == std::string::npos) return 0;

                size_t fieldsPos = clientContent.find("\"fields\"", classPos);
                if (fieldsPos == std::string::npos) return 0;

                std::string fieldSearch = "\"" + field + "\"";
                size_t fieldPos = clientContent.find(fieldSearch, fieldsPos);
                if (fieldPos == std::string::npos) return 0;

                size_t colonPos = clientContent.find(":", fieldPos);
                if (colonPos == std::string::npos) return 0;

                while (colonPos < clientContent.length() && (clientContent[colonPos] == ':' || clientContent[colonPos] == ' ')) colonPos++;

                std::string numStr;
                while (colonPos < clientContent.length() && (isxdigit(clientContent[colonPos]) || clientContent[colonPos] == 'x')) {
                    numStr += clientContent[colonPos];
                    colonPos++;
                }

                if (numStr.find("0x") == 0 || numStr.find("0X") == 0) {
                    return std::stoull(numStr, nullptr, 16);
                }
                return std::stoull(numStr);
                };

            // Поиск m_fFlags в нужных классах
            m_fFlags = findInClass("C_CSPlayerPawn", "m_fFlags");
            if (m_fFlags == 0) m_fFlags = findInClass("C_BasePlayerPawn", "m_fFlags");

            // Поиск m_iIDEntIndex
            m_iIDEntIndex = findInClass("C_CCSPlayerPawnBase", "m_iIDEntIndex");
            if (m_iIDEntIndex == 0) m_iIDEntIndex = findInClass("C_CSPlayerPawn", "m_iIDEntIndex");

            // Поиск m_iHealth
            m_iHealth = findInClass("C_BasePlayerPawn", "m_iHealth");
            if (m_iHealth == 0) m_iHealth = findInClass("C_CSPlayerPawn", "m_iHealth");

            // Поиск m_iTeamNum
            m_iTeamNum = findInClass("C_BaseEntity", "m_iTeamNum");

            // Поиск m_vecViewOffset
            m_vecViewOffset = findInClass("C_BaseModelEntity", "m_vecViewOffset");

            // Поиск m_hPlayerPawn
            m_hPlayerPawn = findInClass("C_CSPlayerController", "m_hPlayerPawn");
        }

        // ПРОВЕРКА И ИСПРАВЛЕНИЕ НЕПРАВИЛЬНЫХ ОФФСЕТОВ
        if (m_fFlags == 0 || m_fFlags == 0x63 || m_fFlags > 0x1000) {
            std::cout << "[Updater] Исправлен m_fFlags: 0x" << std::hex << m_fFlags << " -> 0x" << CORRECT_MFFLAGS << std::dec << std::endl;
            m_fFlags = CORRECT_MFFLAGS;
        }

        if (m_iIDEntIndex == 0 || m_iIDEntIndex > 0x4000) {
            std::cout << "[Updater] Исправлен m_iIDEntIndex: 0x" << std::hex << m_iIDEntIndex << " -> 0x" << CORRECT_MIIDENTINDEX << std::dec << std::endl;
            m_iIDEntIndex = CORRECT_MIIDENTINDEX;
        }

        if (m_iHealth == 0 || m_iHealth > 0x1000) {
            m_iHealth = CORRECT_MIHEALTH;
        }

        if (m_iTeamNum == 0 || m_iTeamNum > 0x1000) {
            m_iTeamNum = CORRECT_MITEAMNUM;
        }

        if (m_vecViewOffset == 0) {
            m_vecViewOffset = CORRECT_MVECVIEWOFFSET;
        }

        if (m_hPlayerPawn == 0) {
            m_hPlayerPawn = CORRECT_MHPLAYERPAWN;
        }

        if (dwEntityList == 0 || dwLocalPlayerPawn == 0) {
            std::cout << "[Updater] Ошибка парсинга оффсетов!" << std::endl;
            return false;
        }

        std::ofstream outFile(offsetsFile);
        if (!outFile.is_open()) return false;

        outFile << "dwEntityList=" << dwEntityList << "\n";
        outFile << "dwLocalPlayerController=" << dwLocalPlayerController << "\n";
        outFile << "dwLocalPlayerPawn=" << dwLocalPlayerPawn << "\n";
        outFile << "m_fFlags=" << m_fFlags << "\n";
        outFile << "m_iIDEntIndex=" << m_iIDEntIndex << "\n";
        outFile << "m_iHealth=" << m_iHealth << "\n";
        outFile << "m_iTeamNum=" << m_iTeamNum << "\n";
        outFile << "m_vecViewOffset=" << m_vecViewOffset << "\n";
        outFile << "m_hPlayerPawn=" << m_hPlayerPawn << "\n";

        outFile.close();

        std::cout << "[Updater] Оффсеты сохранены:" << std::endl;
        std::cout << "  dwEntityList: 0x" << std::hex << dwEntityList << std::endl;
        std::cout << "  dwLocalPlayerPawn: 0x" << dwLocalPlayerPawn << std::endl;
        std::cout << "  m_fFlags: 0x" << m_fFlags << std::endl;
        std::cout << "  m_iIDEntIndex: 0x" << m_iIDEntIndex << std::endl;
        std::cout << "  m_iHealth: 0x" << m_iHealth << std::endl;
        std::cout << "  m_iTeamNum: 0x" << m_iTeamNum << std::dec << std::endl;

        return true;
    }

    bool UpdateOffsets() {
        std::cout << "\n========================================" << std::endl;
        std::cout << "     CS2 Offset Auto-Updater" << std::endl;
        std::cout << "========================================\n" << std::endl;

        if (!CheckAndUpdate()) {
            std::cout << "[Updater] ОШИБКА: Не удалось обновить дампер!" << std::endl;
            return false;
        }

        if (!RunDumper()) {
            std::cout << "[Updater] ОШИБКА: Дампер не запустился!" << std::endl;
            return false;
        }

        if (!ParseOffsets()) {
            std::cout << "[Updater] ОШИБКА: Не удалось распарсить оффсеты!" << std::endl;
            return false;
        }

        std::cout << "\n[Updater] Готово! Оффсеты обновлены.\n" << std::endl;
        return true;
    }
};
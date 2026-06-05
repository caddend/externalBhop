import json
import os
import subprocess
import time

# Скрипт определяет всё относительно своей папки
CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
EXE_PATH = os.path.join(CURRENT_DIR, "cs2-dumper.exe")
OUTPUT_DIR = os.path.join(CURRENT_DIR, "output")

OFFSETS_FILE = os.path.join(OUTPUT_DIR, "offsets.json")
CLIENT_DLL_FILE = os.path.join(OUTPUT_DIR, "client_dll.json") 
TXT_OUTPUT = os.path.join(CURRENT_DIR, "offsets.txt")

def main():
    if not os.path.exists(EXE_PATH):
        print(f"[-] Ошибка: Не найден {EXE_PATH}")
        return

    try:
        process = subprocess.Popen(EXE_PATH, cwd=CURRENT_DIR)
        print("[+] Дампер запущен. Сканирование памяти CS2...")
        process.wait()  
        time.sleep(1.5) 
    except Exception as e:
        print(f"[-] Не удалось запустить дампер: {e}")
        return

    if not os.path.exists(OFFSETS_FILE) or not os.path.exists(CLIENT_DLL_FILE):
        print("[-] Ошибка: JSON файлы не найдены!")
        return

    with open(OFFSETS_FILE, "r", encoding="utf-8") as f:
        offsets_data = json.load(f)
    with open(CLIENT_DLL_FILE, "r", encoding="utf-8") as f:
        client_data = json.load(f)

    client_offsets = offsets_data.get("client.dll", {})
    dwEntityList = client_offsets.get("dwEntityList", 0)
    dwLocalPlayerController = client_offsets.get("dwLocalPlayerController", 0)
    dwLocalPlayerPawn = client_offsets.get("dwLocalPlayerPawn", 0)

    m_fFlags = 0
    classes_source = client_data.get("client.dll", {}).get("classes", client_data)

    for class_name, class_info in classes_source.items():
        fields = class_info.get("fields", {})
        if class_name in ["C_BaseEntity", "C_CSPlayerPawnBase", "C_BasePlayerPawn"]:
            if "m_fFlags" in fields: 
                m_fFlags = fields["m_fFlags"]
                break

    if m_fFlags == 0: m_fFlags = 0x3F8 # Фолбэк

    # Записываем в простой конфиг-файл для C++
    with open(TXT_OUTPUT, "w") as f:
        f.write(f"dwEntityList={dwEntityList}\n")
        f.write(f"dwLocalPlayerController={dwLocalPlayerController}\n")
        f.write(f"dwLocalPlayerPawn={dwLocalPlayerPawn}\n")
        f.write(f"m_fFlags={m_fFlags}\n")

    print(f"[+] Данные успешно сохранены в {TXT_OUTPUT}")

if __name__ == "__main__":
    main()
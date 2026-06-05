import json
import os
import subprocess
import time

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
EXE_PATH = os.path.join(CURRENT_DIR, "cs2-dumper.exe")
OUTPUT_DIR = os.path.join(CURRENT_DIR, "output")

OFFSETS_FILE = os.path.join(OUTPUT_DIR, "offsets.json")
CLIENT_DLL_FILE = os.path.join(OUTPUT_DIR, "client_dll.json") 
TXT_OUTPUT = os.path.join(CURRENT_DIR, "offsets.txt")

def main():
    if not os.path.exists(EXE_PATH): return

    try:
        process = subprocess.Popen(EXE_PATH, cwd=CURRENT_DIR)
        process.wait()  
        time.sleep(1.5) 
    except:
        return

    if not os.path.exists(OFFSETS_FILE) or not os.path.exists(CLIENT_DLL_FILE): return

    with open(OFFSETS_FILE, "r", encoding="utf-8") as f: offsets_data = json.load(f)
    with open(CLIENT_DLL_FILE, "r", encoding="utf-8") as f: client_data = json.load(f)

    client_offsets = offsets_data.get("client.dll", {})
    dwEntityList = client_offsets.get("dwEntityList", 0)
    dwLocalPlayerController = client_offsets.get("dwLocalPlayerController", 0)
    dwLocalPlayerPawn = client_offsets.get("dwLocalPlayerPawn", 0)

    m_fFlags = 0
    m_iIDEntIndex = 0
    m_iHealth = 0
    m_iTeamNum = 0
    
    classes_source = client_data.get("client.dll", {}).get("classes", client_data)

    # Точный пошаговый поиск в специфичных классах CS2
    # 1. Проверяем главный базовый класс игрока CS2 для индекса прицела
    if "C_CCSPlayerPawnBase" in classes_source:
        fields = classes_source["C_CCSPlayerPawnBase"].get("fields", {})
        if "m_iIDEntIndex" in fields:
            m_iIDEntIndex = fields["m_iIDEntIndex"]

    # 2. Собираем остальные данные по цепочке наследования
    for class_name in ["C_CSPlayerPawn", "C_BasePlayerPawn", "C_BaseEntity"]:
        if class_name in classes_source:
            fields = classes_source[class_name].get("fields", {})
            if m_fFlags == 0 and "m_fFlags" in fields: m_fFlags = fields["m_fFlags"]
            if m_iHealth == 0 and "m_iHealth" in fields: m_iHealth = fields["m_iHealth"]
            if m_iTeamNum == 0 and "m_iTeamNum" in fields: m_iTeamNum = fields["m_iTeamNum"]
            if m_iIDEntIndex == 0 and "m_iIDEntIndex" in fields: m_iIDEntIndex = fields["m_iIDEntIndex"]

    # Жесткий фолбэк, если дампер выдал пустые структуры
    if m_fFlags == 0 or m_fFlags == 0x63: m_fFlags = 0x3F8
    if m_iIDEntIndex == 0: m_iIDEntIndex = 0x1444 # Настоящий оффсет из C_CCSPlayerPawnBase
    if m_iHealth == 0: m_iHealth = 0x34C
    if m_iTeamNum == 0: m_iTeamNum = 0x3EB

    with open(TXT_OUTPUT, "w") as f:
        f.write(f"dwEntityList={dwEntityList}\n")
        f.write(f"dwLocalPlayerController={dwLocalPlayerController}\n")
        f.write(f"dwLocalPlayerPawn={dwLocalPlayerPawn}\n")
        f.write(f"m_fFlags={m_fFlags}\n")
        f.write(f"m_iIDEntIndex={m_iIDEntIndex}\n")
        f.write(f"m_iHealth={m_iHealth}\n")
        f.write(f"m_iTeamNum={m_iTeamNum}\n")

    print("[+] Оффсеты успешно пересобраны.")

if __name__ == "__main__":
    main()
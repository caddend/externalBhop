#pragma once
#include <windows.h>
#include <thread>
#include "GameMemory.hpp"

struct Vec3 {
    float x, y, z;
};

class Triggerbot {
private:
    GameMemory& memory;
    bool isTriggerRunning = false;
    std::thread triggerThread;
    int triggerKey = VK_MENU;

    // Функции для работы с сущностями
    uintptr_t GetEntityFromIndex(int index);
    uintptr_t GetEntityPawn(uintptr_t controller);
    Vec3 GetEntityPosition(uintptr_t pawn);
    int GetEntityHealth(uintptr_t pawn);
    int GetEntityTeam(uintptr_t pawn);
    bool IsAlive(uintptr_t pawn);

    // Функции для локального игрока
    uintptr_t GetLocalPawn();
    Vec3 GetLocalOrigin(uintptr_t localPawn);
    Vec3 GetLocalEyePosition(uintptr_t localPawn);
    void GetLocalAngles(uintptr_t localPawn, float& yaw, float& pitch);

    // Вспомогательные функции
    float GetAngleToTarget(const Vec3& from, const Vec3& to, float yaw, float pitch);
    Vec3 AngleToDirection(float yaw, float pitch);
    float VectorLength(const Vec3& v);
    Vec3 NormalizeVector(const Vec3& v);

public:
    Triggerbot(GameMemory& mem) : memory(mem) {}
    ~Triggerbot() { Stop(); }

    void Start(int vKey);
    void Stop();
    void TriggerLoop();
};
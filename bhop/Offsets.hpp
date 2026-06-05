#pragma once
#include <windows.h>

namespace offsets
{
    constexpr uintptr_t dwEntityList = 0x24e6590;
    constexpr uintptr_t dwLocalPlayerController = 0x231f700;
    constexpr uintptr_t dwLocalPlayerPawn = 0x2340698;
    constexpr uintptr_t dwViewMatrix = 0x2345b30;

    namespace controller {
        constexpr uintptr_t m_hPawn = 0x6bc;
    }

    namespace pawn {
        constexpr uintptr_t m_iHealth = 0x34c;
        constexpr uintptr_t m_iTeamNum = 0x3eb;
        constexpr uintptr_t m_fFlags = 0x3f8;
        constexpr uintptr_t m_vecAbsVelocity = 0x3fc;
    }
}

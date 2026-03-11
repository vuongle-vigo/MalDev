#include "pch.h"
#include <windows.h>

extern "C" __declspec(dllexport) void Func1() {}
extern "C" __declspec(dllexport) void Func2() {}
extern "C" __declspec(dllexport) void Func3() {}

extern "C" __declspec(dllexport) void Func4() {
    MessageBoxA(nullptr, nullptr, nullptr, MB_OK);
    Sleep(100000000);
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
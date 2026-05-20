#pragma once
#include <Windows.h>
#include <string>
#include <TlHelp32.h>

HANDLE GetProcessHandleByNameW(const std::wstring& wsProcessName);
HANDLE GetProcessHandleByPid(DWORD dwPid);
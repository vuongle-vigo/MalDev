#pragma once
#include <Windows.h>

bool MappingShellcode(_In_ HANDLE hTargetProcess, _In_ LPVOID lpShellcode, _In_ DWORD dwShellSize, _Out_ LPVOID* lpRemoteAddr);
#pragma once
#include <Windows.h>
#include <string>

bool GetFileDataW(_In_ const std::wstring wsFilepath, _Out_ LPVOID* lpBuffer, _Out_ DWORD* dwFileSize);
bool MappingShellcode(_In_ HANDLE hTargetProcess, _In_ LPVOID lpShellcode, _In_ DWORD dwShellSize, _Out_ LPVOID* lpRemoteAddr, _In_ ULONG PageProtection);
#pragma once
#include <Windows.h>
#include <string>

bool GetFileDataW(_In_ const std::wstring wsFilepath, _Out_ LPVOID* lpBuffer, _Out_ DWORD* dwFileSize);
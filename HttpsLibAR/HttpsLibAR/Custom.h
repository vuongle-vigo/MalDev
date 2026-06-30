#pragma once
#include <Windows.h>

void PathAmsi();
wchar_t* CharToWChar(const char* str);
BOOL Exec(const wchar_t* command, wchar_t** output);
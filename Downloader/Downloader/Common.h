#pragma once
#include <string>
#include <windows.h>

std::string WstringToString(const std::wstring& wstr);
bool XorDecodeFileInplace(const std::wstring& filePath, BYTE key);
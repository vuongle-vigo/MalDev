#include "Common.h"

std::string WstringToString(const std::wstring& wstr) {
	if (wstr.empty()) return "";
	int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string result(size - 1, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, nullptr, nullptr);
	return result;
}

#include <windows.h>
#include <vector>
#include <string>

bool XorDecodeFileInplace(const std::wstring& filePath, BYTE key)
{
    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (fileSize == INVALID_FILE_SIZE && GetLastError() != NO_ERROR) {
        CloseHandle(hFile);
        return false;
    }

    if (fileSize == 0) {
        CloseHandle(hFile);
        return true;
    }

    std::vector<BYTE> buffer(fileSize);
    DWORD bytesRead = 0;

    if (!ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL) ||
        bytesRead != fileSize)
    {
        CloseHandle(hFile);
        return false;
    }

    for (DWORD i = 0; i < fileSize; i++) {
        buffer[i] ^= key;
    }

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    DWORD bytesWritten = 0;
    if (!WriteFile(hFile, buffer.data(), fileSize, &bytesWritten, NULL) ||
        bytesWritten != fileSize)
    {
        CloseHandle(hFile);
        return false;
    }

    CloseHandle(hFile);
    return true;
}

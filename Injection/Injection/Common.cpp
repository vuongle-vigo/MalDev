#include "Common.h"

bool GetFileDataW(_In_ const std::wstring wsFilepath, _Out_ LPVOID* lpBuffer, _Out_ DWORD* dwFileSize) {
	HANDLE hFile = CreateFileW(wsFilepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return false;
	}

	*dwFileSize = GetFileSize(hFile, NULL);
	if (*dwFileSize == INVALID_FILE_SIZE) {
		CloseHandle(hFile);
		return false;
	}

	*lpBuffer = VirtualAlloc(NULL, *dwFileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (*lpBuffer == nullptr) {
		CloseHandle(hFile);
		return false;
	}

	DWORD dwBytesRead;
	BOOL bResult = ReadFile(hFile, *lpBuffer, *dwFileSize, &dwBytesRead, NULL);
	CloseHandle(hFile);
	if (!bResult || dwBytesRead != *dwFileSize) {
		VirtualFree(*lpBuffer, 0, MEM_RELEASE);
		return false;
	}

	return true;
}
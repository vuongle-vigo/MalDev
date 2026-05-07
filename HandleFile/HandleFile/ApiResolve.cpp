#include "ApiResolve.h"
#include "HashString.h"
#include "CRT.h"

ApiResolve::ApiResolve() {
#ifdef _WIN64
	pPeb = (PPEB)__readgsqword(0x60);
#else
	pPeb = (PPEB)__readfsdword(0x30);
#endif
}

ApiResolve::~ApiResolve() {}

LPVOID ApiResolve::GetModuleBaseAddress(const LPWSTR lpwsModuleName) {
	PPEB_LDR_DATA pLdrData = pPeb->Ldr;
	LIST_ENTRY* fl = pLdrData->InMemoryOrderModuleList.Flink;
	while (fl != &pLdrData->InMemoryOrderModuleList) {
		fl = fl->Flink;
		LDR_DATA_TABLE_ENTRY* pEntry = (LDR_DATA_TABLE_ENTRY*)((DWORD64)fl - FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));
		PWSTR pDllPath = pEntry->FullDllName.Buffer;
		if (pDllPath == NULL) {
			break;
		}

		PWSTR pDllName = NULL;
		WCHAR wsPattern[] = { '\\' };
		if (!FindPatternW(pDllPath, wsPattern, 1, &pDllName)) {
			continue;
		}

		while (FindPatternW(pDllName, wsPattern, 1, &pDllName)) {
			pDllName = pDllName + 1;
		}

		WCHAR wsModuleName[MAX_PATH];
		CopyStringW(pDllName, wsModuleName, sizeof(wsModuleName));

		LowerStringW(wsModuleName);
		LowerStringW(lpwsModuleName);

		if (!VxCompareStringW(wsModuleName, lpwsModuleName)) {
			continue;
		}

		return pEntry->DllBase;
	}

	return NULL;
}

LPVOID ApiResolve::GetModuleBaseAddress(unsigned int hash) {
	PPEB_LDR_DATA pLdrData = pPeb->Ldr;
	LIST_ENTRY* fl = pLdrData->InMemoryOrderModuleList.Flink;
	while (fl != &pLdrData->InMemoryOrderModuleList) {
		fl = fl->Flink;
		LDR_DATA_TABLE_ENTRY* pEntry = (LDR_DATA_TABLE_ENTRY*)((DWORD64)fl - FIELD_OFFSET(LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks));
		PWSTR pDllPath = pEntry->FullDllName.Buffer;
		if (pDllPath == NULL) {
			break;
		}

		PWSTR pDllName = NULL;
		WCHAR wsPattern[] = { '\\' };
		if (!FindPatternW(pDllPath, wsPattern, 1, &pDllName)) {
			continue;
		}

		while (FindPatternW(pDllName, wsPattern, 1, &pDllName)) {
			pDllName = pDllName + 1;
		}

		WCHAR wsModuleName[MAX_PATH];
		CopyStringW(pDllName, wsModuleName, sizeof(wsModuleName));
		LowerStringW(pDllName);
		if (ComplexHashForWChar(pDllName) != hash) {
			continue;
		}

		return pEntry->DllBase;
	}

	return NULL;
}

LPVOID ApiResolve::GetApiAddress(LPVOID lpBaseAddress, const char* sApiName) {
	IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)lpBaseAddress;
	IMAGE_NT_HEADERS* pNtHeaders = (IMAGE_NT_HEADERS*)((DWORD64)lpBaseAddress + pDosHeader->e_lfanew);
	IMAGE_OPTIONAL_HEADER* pOptionalHeader = &pNtHeaders->OptionalHeader;
	IMAGE_DATA_DIRECTORY* pExportDataDirectory = &pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

	IMAGE_EXPORT_DIRECTORY* pExportDirectory = (IMAGE_EXPORT_DIRECTORY*)((DWORD64)lpBaseAddress + pExportDataDirectory->VirtualAddress);

	int index = 0;
	LPVOID pAddressOfName = (LPVOID)((DWORD64)lpBaseAddress + pExportDirectory->AddressOfNames);
	LPVOID pAddressOfOrdinal = (LPVOID)((DWORD64)lpBaseAddress + pExportDirectory->AddressOfNameOrdinals);
	LPVOID pAddressOfFunction = (LPVOID)((DWORD64)lpBaseAddress + pExportDirectory->AddressOfFunctions);

	for (int i = 0; i < pExportDirectory->NumberOfNames; i++) {
		DWORD rvaName = *(DWORD*)((DWORD64)pAddressOfName + i * sizeof(DWORD));
		char* sName = (char*)((DWORD64)lpBaseAddress + rvaName);
		if (!VxCompareStringA(sName, sApiName)) {
			continue;
		}

		WORD ordinal = *(WORD*)((DWORD64)pAddressOfOrdinal + i * sizeof(BYTE) * 2);
		DWORD rvaFunction = *(DWORD*)((DWORD64)pAddressOfFunction + ordinal * sizeof(DWORD));
		return (LPVOID)((DWORD64)lpBaseAddress + rvaFunction);
	}

	return NULL;
}

LPVOID ApiResolve::GetApiAddress(LPVOID lpBaseAddress, unsigned int hash) {
	IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)lpBaseAddress;
	IMAGE_NT_HEADERS* pNtHeaders = (IMAGE_NT_HEADERS*)((DWORD64)lpBaseAddress + pDosHeader->e_lfanew);
	IMAGE_OPTIONAL_HEADER* pOptionalHeader = &pNtHeaders->OptionalHeader;
	IMAGE_DATA_DIRECTORY* pExportDataDirectory = &pOptionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

	IMAGE_EXPORT_DIRECTORY* pExportDirectory = (IMAGE_EXPORT_DIRECTORY*)((DWORD64)lpBaseAddress + pExportDataDirectory->VirtualAddress);

	int index = 0;
	LPVOID pAddressOfName = (LPVOID)((DWORD64)lpBaseAddress + pExportDirectory->AddressOfNames);
	LPVOID pAddressOfOrdinal = (LPVOID)((DWORD64)lpBaseAddress + pExportDirectory->AddressOfNameOrdinals);
	LPVOID pAddressOfFunction = (LPVOID)((DWORD64)lpBaseAddress + pExportDirectory->AddressOfFunctions);

	for (int i = 0; i < pExportDirectory->NumberOfNames; i++) {
		DWORD rvaName = *(DWORD*)((DWORD64)pAddressOfName + i * sizeof(DWORD));
		char* sName = (char*)((DWORD64)lpBaseAddress + rvaName);
		if (ComplexHashForAnsi(sName) != hash) {
			continue;
		}

		WORD ordinal = *(WORD*)((DWORD64)pAddressOfOrdinal + i * sizeof(BYTE) * 2);
		DWORD rvaFunction = *(DWORD*)((DWORD64)pAddressOfFunction + ordinal * sizeof(DWORD));
		LPVOID lpFunction = (LPVOID)((DWORD64)lpBaseAddress + rvaFunction);
		if ((StrLen(sName) + 1) + sName == lpFunction) {
			char* s = (char*)lpFunction;
			int size = 0;
			for (int i = 0;;i++) {
				if (*s == '.') {
					break;
				}

				s++;
				size++;
			}

			char sDllName[MAX_PATH] = { 0 };
			CopyStringA((char*)lpFunction, sDllName, size + 4);
			char dll[] = { '.', 'd', 'l', 'l' };
			CopyStringA(dll, sDllName + size, 4);
			LowerStringA(sDllName);
			wchar_t wsDllName[MAX_PATH] = { 0 };
			StringAToW(sDllName, wsDllName);

			char sNewApiName[MAX_PATH] = { 0 };
			CopyStringA(s + 1, sNewApiName, MAX_PATH);
			LPVOID lpNewDll = GetModuleBaseAddress(ComplexHashForWChar(wsDllName));
			if (!lpNewDll) {
				constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
				LPVOID lpKernel32 = GetModuleBaseAddress(hashKernel32);
				if (!lpKernel32) { return NULL; }
				constexpr unsigned int hashLoadLibraryA = ComplexHashForAnsi("LoadLibraryA");
				typedef HMODULE(WINAPI* _LoadLibraryA)(LPCSTR lpLibFileName);
				_LoadLibraryA pLoadLibraryA = (_LoadLibraryA)GetApiAddress(lpKernel32, hashLoadLibraryA);
				if (!pLoadLibraryA) { return NULL; }
				HMODULE hModule = pLoadLibraryA(sDllName);
				if (!hModule) { return NULL; }
				lpNewDll = hModule;
			}

			return GetApiAddress(lpNewDll, ComplexHashForAnsi(sNewApiName));
		}

		return (LPVOID)((DWORD64)lpBaseAddress + rvaFunction);
	}

	return NULL;
}
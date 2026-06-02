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
		//WCHAR wsPattern[] = { '\\' };
		WCHAR wsPattern = '\\';
		//if (!FindPatternW(pDllPath, wsPattern, 1, &pDllName)) {
		//	continue;
		//}

		//while (FindPatternW(pDllName, wsPattern, 1, &pDllName)) {
		//	pDllName = pDllName + 1;
		//}
		pDllName = crt_wcsrchr(pDllName, wsPattern);
		if (pDllName == NULL) {
			continue;
		}

		pDllName = pDllPath + 1;
		WCHAR wsModuleName[MAX_PATH];
		//CopyStringW(pDllName, wsModuleName, sizeof(wsModuleName));
		crt_wcscpy(wsModuleName, pDllName);
		//LowerStringW(lpwsModuleName);
		crt_wcslwr(wsModuleName);

		//if (!CompareStringW(wsModuleName, lpwsModuleName)) {
		//	continue;
		//}

		if (!crt_wcscmp(wsModuleName, lpwsModuleName)) {
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
		//WCHAR wsPattern[] = { '\\' };
		WCHAR wsPattern = '\\';
		//if (!FindPatternW(pDllPath, wsPattern, 1, &pDllName)) {
		//	continue;
		//}
		pDllName = crt_wcsrchr(pDllPath, wsPattern);

		if (pDllName == NULL) {
			continue;
		}

		pDllName = pDllName + 1;


		WCHAR wsModuleName[MAX_PATH];
		//CopyStringW(pDllName, wsModuleName, sizeof(wsModuleName));
		crt_wcscpy(wsModuleName, pDllName);
		//LowerStringW(pDllName);
		crt_wcslwr(pDllName);
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
		//if (!CompareStringA(sName, sApiName)) {
		//	continue;
		//}
		if (!crt_strcmp(sName, sApiName)) {
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
		LPVOID addr = (LPVOID)((DWORD64)lpBaseAddress + rvaFunction);
		if (addr == sName + crt_strlen(sName) + 1) {
			char sDllName[50] = { 0 };
			char pattern = '.';
			char* p = crt_strchr((char*)addr, pattern);
			size_t size = (DWORD64)p - (DWORD64)addr;
			crt_strncpy(sDllName, (char*)addr, size + 1);
			char dll[] = { 'd', 'l', 'l', '\0' };
			crt_strcpy(sDllName + size + 1, dll);
			crt_strlwr(sDllName);
			unsigned int hashDll = ComplexHashForAnsi(sDllName);
			LPVOID lpDll = GetModuleBaseAddress(hashDll);
			char* sFuncName = (char*)addr + size + 1;
			unsigned int hashFuncName = ComplexHashForAnsi(sFuncName);
			return GetApiAddress(lpDll, hashFuncName);
		}
		
		return addr;
	}

	return NULL;
}
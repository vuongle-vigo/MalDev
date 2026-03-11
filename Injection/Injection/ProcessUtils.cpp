#include "ProcessUtils.h"
#include "ResolveApi.h"

HANDLE GetProcessHandleByNameW(const std::wstring wsProcessName) {
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE) {
		return nullptr;
	}
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hProcessSnap, &pe32)) {
		CloseHandle(hProcessSnap);
		return nullptr;
	}

	HANDLE hProcess = nullptr;
	CLIENT_ID clientId;
	NTSTATUS ntStatus;
	do {
		if (wsProcessName == pe32.szExeFile) {
			pNtOpenProcess ntOpenProcess = (pNtOpenProcess)GetProcAddress(GetModuleHandleA("ntdll"), "NtOpenProcess");
			if (ntOpenProcess == nullptr) {
				CloseHandle(hProcessSnap);
				return nullptr;
			}

			ntStatus = ntOpenProcess(&hProcess, PROCESS_ALL_ACCESS, FALSE, &clientId);
			if (!NT_SUCCESS(ntStatus) || hProcess == nullptr) {
				CloseHandle(hProcessSnap);
				return nullptr;
			}

			break;
		}
	} while (Process32Next(hProcessSnap, &pe32));
	CloseHandle(hProcessSnap);
	return hProcess;
}

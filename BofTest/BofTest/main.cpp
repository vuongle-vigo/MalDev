#include <Windows.h>
#include <psapi.h>

int main() {
	DWORD pids[1024], needed;
	if (!EnumProcesses(pids, sizeof(pids), &needed)) return 1;

	return 1;
}
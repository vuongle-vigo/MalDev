#pragma once

#include <Windows.h>

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

typedef NTSTATUS
(NTAPI*
    _NtTraceEvent)(
        _In_opt_ HANDLE TraceHandle,
        _In_ ULONG Flags,
        _In_ ULONG FieldSize,
        _In_ PVOID Fields
        );

// Function declarations
PVOID FindOpcode(PVOID pAddress, BYTE opcode, DWORD dwLengthSearch);
LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo);
int PatchEtw();
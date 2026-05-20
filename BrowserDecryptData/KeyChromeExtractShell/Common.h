#pragma once
#include <Windows.h>

#define TH32CS_SNAPPROCESS  0x00000002
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004)
#define STATUS_BUFFER_TOO_SMALL    ((NTSTATUS)0xC0000023)

typedef struct _PROCESS_HANDLE_TABLE_ENTRY_INFO {
    HANDLE HandleValue;
    ULONG_PTR HandleCount;
    ULONG_PTR PointerCount;
    ULONG GrantedAccess;
    ULONG ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} PROCESS_HANDLE_TABLE_ENTRY_INFO;

typedef struct _PROCESS_HANDLE_SNAPSHOT_INFORMATION {
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    PROCESS_HANDLE_TABLE_ENTRY_INFO Handles[1];
} PROCESS_HANDLE_SNAPSHOT_INFORMATION;

typedef struct tagPROCESSENTRY32W
{
    DWORD   dwSize;
    DWORD   cntUsage;
    DWORD   th32ProcessID;          // this process
    ULONG_PTR th32DefaultHeapID;
    DWORD   th32ModuleID;           // associated exe
    DWORD   cntThreads;
    DWORD   th32ParentProcessID;    // this process's parent process
    LONG    pcPriClassBase;         // Base priority of process's threads
    DWORD   dwFlags;
    WCHAR   szExeFile[MAX_PATH];    // Path
} PROCESSENTRY32W;
typedef PROCESSENTRY32W* PPROCESSENTRY32W;
typedef PROCESSENTRY32W* LPPROCESSENTRY32W;

size_t GetProcessIdsByName(
    const wchar_t* processName,
    DWORD* outPids,
    size_t maxCount
);

bool CreateCopyFile(char* filename, char* newFileName);
void DecryptKey();
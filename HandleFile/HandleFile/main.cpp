#include <windows.h>
#include <iostream>
#include <vector>
#include <TlHelp32.h>

#define ProcessHandleInformation 51
#define ObjectTypeInformation 2
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004)
#define STATUS_BUFFER_TOO_SMALL    ((NTSTATUS)0xC0000023)

typedef NTSTATUS(NTAPI* NtQueryInformationProcess_t)(
    HANDLE ProcessHandle,
    ULONG ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
    );

typedef NTSTATUS(NTAPI* NtQueryObject_t)(
    HANDLE Handle,
    ULONG ObjectInformationClass,
    PVOID ObjectInformation,
    ULONG ObjectInformationLength,
    PULONG ReturnLength
    );

typedef struct _UNICODE_STRING_T {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING_T;

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

std::wstring GetHandleType(HANDLE h)
{
    auto NtQueryObject = (NtQueryObject_t)GetProcAddress(
        GetModuleHandleW(L"ntdll.dll"),
        "NtQueryObject"
    );

    BYTE buffer[4096] = {};
    ULONG retLen = 0;

    NTSTATUS st = NtQueryObject(
        h,
        ObjectTypeInformation,
        buffer,
        sizeof(buffer),
        &retLen
    );

    if (st != 0)
        return L"";

    auto us = (UNICODE_STRING_T*)buffer;

    return std::wstring(
        us->Buffer,
        us->Length / sizeof(wchar_t)
    );
}

#include "HashString.h"
#include "ApiResolve.h"
#include "CRT.h"

size_t GetProcessIdsByName(
    const wchar_t* processName,
    DWORD* outPids,
    size_t maxCount
)
{   
    ApiResolve apiResolve;
    size_t count = 0;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    constexpr unsigned int hashCreateToolhelp32Snapshot = ComplexHashForAnsi("CreateToolhelp32Snapshot");
    typedef HANDLE(WINAPI* _CreateToolhelp32Snapshot)(DWORD, DWORD);
    _CreateToolhelp32Snapshot pCreateToolhelp32Snapshot = (_CreateToolhelp32Snapshot)apiResolve.GetApiAddress(lpKernel32, hashCreateToolhelp32Snapshot);
    HANDLE hSnap = pCreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        0
    );

    if (hSnap == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);

    constexpr unsigned int hashProcess32FirstW = ComplexHashForAnsi("Process32FirstW");
    typedef BOOL(WINAPI* _Process32FirstW)(HANDLE, LPPROCESSENTRY32W);
    _Process32FirstW pProcess32FirstW = (_Process32FirstW)apiResolve.GetApiAddress(lpKernel32, hashProcess32FirstW);

    constexpr unsigned int hashProcess32NextW = ComplexHashForAnsi("Process32NextW");
    typedef BOOL(WINAPI* _Process32NextW)(HANDLE, LPPROCESSENTRY32W);
    _Process32NextW pProcess32NextW = (_Process32NextW)apiResolve.GetApiAddress(lpKernel32, hashProcess32NextW);

    typedef BOOL(WINAPI* _CloseHandle)(HANDLE);
    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);

    if (pProcess32FirstW(hSnap, &pe))
    {
        do
        {
            if (VxCompareStringW(pe.szExeFile, processName))
            {
                if (count < maxCount)
                {
                    outPids[count++] =
                        pe.th32ProcessID;
                }
            }

        } while (pProcess32NextW(hSnap, &pe));
    }

    pCloseHandle(hSnap);

    return count;
}

int wmain(int argc, wchar_t* argv[])
{
    DWORD pids[256];

    wchar_t processName[] = {
    L'm', L's', L'e', L'd', L'g', L'e',
    L'.', L'e', L'x', L'e',
    L'\0'
    };
    size_t count = GetProcessIdsByName(
        processName,
        pids,
        256
    );

    auto NtQueryInformationProcess =
        (NtQueryInformationProcess_t)GetProcAddress(
            GetModuleHandleW(L"ntdll.dll"),
            "NtQueryInformationProcess"
        );

    for (size_t i = 0; i < count; i++) {
        DWORD pid = pids[i];
        HANDLE hProc = OpenProcess(
            PROCESS_ALL_ACCESS,
            FALSE,
            pid
        );

        if (!hProc) {
            std::wcout << L"OpenProcess failed: " << GetLastError() << L"\n";
            return 1;
        }



        ULONG size = 0x10000;
        std::vector<BYTE> buffer(size);

        ULONG retLen = 0;
        NTSTATUS st;

        while (true)
        {
            st = NtQueryInformationProcess(
                hProc,
                ProcessHandleInformation,
                buffer.data(),
                size,
                &retLen
            );

            if (st == 0)
                break;

            if (st == STATUS_INFO_LENGTH_MISMATCH ||
                st == STATUS_BUFFER_TOO_SMALL)
            {
                size *= 2;
                buffer.resize(size);
                continue;
            }

            std::wcout << L"NtQueryInformationProcess failed: 0x"
                << std::hex << st << std::dec << L"\n";

            CloseHandle(hProc);
            return 1;
        }

        auto info =
            (PROCESS_HANDLE_SNAPSHOT_INFORMATION*)buffer.data();

        std::wcout << L"PID: " << pid << L"\n";
        std::wcout << L"Handle count: " << info->NumberOfHandles << L"\n\n";

        for (ULONG_PTR i = 0; i < info->NumberOfHandles; i++)
        {
            auto& e = info->Handles[i];

            HANDLE hDup = NULL;

            BOOL ok = DuplicateHandle(
                hProc,
                e.HandleValue,
                GetCurrentProcess(),
                &hDup,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS
            );

            if (!ok)
                continue;

            DWORD fileType = GetFileType(hDup);

            if (fileType != FILE_TYPE_DISK) {
                CloseHandle(hDup);
                continue;
            }


            wchar_t path[4096] = {};

            DWORD len = GetFinalPathNameByHandleW(
                hDup,
                path,
                ARRAYSIZE(path),
                FILE_NAME_NORMALIZED
            );

            if (len > 0 && len < ARRAYSIZE(path))
            {
                std::wcout
                    << L"handle: 0x"
                    << std::hex
                    << (ULONG_PTR)e.HandleValue
                    << std::dec
                    << L"\n";

                std::wcout
                    << L"path: "
                    << path
                    << L"\n\n";
            }

            CloseHandle(hDup);
        }

        CloseHandle(hProc);
    }
    return 0;
}
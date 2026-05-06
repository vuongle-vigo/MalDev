#include <windows.h>
#include <winternl.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "ntdll.lib")

#define SystemExtendedHandleInformation 64
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004)

typedef NTSTATUS(NTAPI* NtQuerySystemInformation_t)(
    ULONG SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
    );

typedef NTSTATUS(NTAPI* NtQueryObject_t)(
    HANDLE Handle,
    ULONG ObjectInformationClass,
    PVOID ObjectInformation,
    ULONG ObjectInformationLength,
    PULONG ReturnLength
    );

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX {
    PVOID Object;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR HandleValue;
    ULONG GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX {
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX;

typedef struct _UNICODE_STRING_T {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING_T;

std::wstring GetHandleType(HANDLE h)
{
    auto NtQueryObject =
        (NtQueryObject_t)GetProcAddress(
            GetModuleHandleW(L"ntdll.dll"),
            "NtQueryObject"
        );

    BYTE buffer[4096] = {};
    ULONG returnLength = 0;

    NTSTATUS status = NtQueryObject(
        h,
        2, // ObjectTypeInformation
        buffer,
        sizeof(buffer),
        &returnLength
    );

    if (status != 0)
        return L"";

    auto str = (UNICODE_STRING_T*)buffer;

    return std::wstring(
        str->Buffer,
        str->Length / sizeof(wchar_t)
    );
}

int main()
{
    auto NtQuerySystemInformation =
        (NtQuerySystemInformation_t)GetProcAddress(
            GetModuleHandleW(L"ntdll.dll"),
            "NtQuerySystemInformation"
        );

    if (!NtQuerySystemInformation) {
        std::cout << "Failed to get NtQuerySystemInformation\n";
        return 1;
    }

    ULONG size = 0x100000;
    std::vector<BYTE> buffer(size);

    ULONG returnLength = 0;
    NTSTATUS status;

    while (true)
    {
        status = NtQuerySystemInformation(
            SystemExtendedHandleInformation,
            buffer.data(),
            size,
            &returnLength
        );

        if (status == 0)
            break;

        if (status != STATUS_INFO_LENGTH_MISMATCH) {
            std::cout << "NtQuerySystemInformation failed\n";
            return 1;
        }

        size *= 2;
        buffer.resize(size);
    }

    auto info =
        (SYSTEM_HANDLE_INFORMATION_EX*)buffer.data();

    std::wcout << L"Total handles: "
        << info->NumberOfHandles
        << L"\n\n";

    for (ULONG_PTR i = 0; i < info->NumberOfHandles; i++)
    {
        auto& h = info->Handles[i];

        DWORD pid = (DWORD)h.UniqueProcessId;

        HANDLE hProcess = OpenProcess(
            PROCESS_DUP_HANDLE,
            FALSE,
            pid
        );

        if (!hProcess)
            continue;

        HANDLE hDup = NULL;

        BOOL ok = DuplicateHandle(
            hProcess,
            (HANDLE)h.HandleValue,
            GetCurrentProcess(),
            &hDup,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
        );

        CloseHandle(hProcess);

        if (!ok)
            continue;

        std::wstring type = GetHandleType(hDup);

        if (type == L"File")
        {
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
                    << L"[PID "
                    << pid
                    << L"] Handle: 0x"
                    << std::hex
                    << (ULONG_PTR)h.HandleValue
                    << std::dec
                    << L"\n";

                std::wcout
                    << L"Path: "
                    << path
                    << L"\n\n";
            }
        }

        CloseHandle(hDup);
    }

    return 0;
}
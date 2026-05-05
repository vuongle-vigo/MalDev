
#include "ApiResolve.h"
#include "HashString.h"
#include "CRT.h"

#include <windows.h>
#include <iostream>



#pragma comment(lib, "ntdll.lib")

#define SystemExtendedHandleInformation 64
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)

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



int main() {

    HANDLE hFile = CreateFileA(
        "test.txt",
        GENERIC_READ | GENERIC_WRITE,
        0, // share = 0
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "Open test.txt failed: " << GetLastError() << "\n";
        return 1;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");

    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    constexpr unsigned int hashNtdll = ComplexHashForWChar(L"ntdll.dll");
    LPVOID lpNtdll = apiResolve.GetModuleBaseAddress(hashNtdll);

    constexpr unsigned int hashNtQuerySystemInformation = ComplexHashForAnsi("NtQuerySystemInformation");
    typedef NTSTATUS(NTAPI* _NtQuerySystemInformation)(
        ULONG,
        PVOID,
        ULONG,
        PULONG
        );

    _NtQuerySystemInformation pNtQuerySystemInformation = (_NtQuerySystemInformation)apiResolve.GetApiAddress(lpNtdll, hashNtQuerySystemInformation);

    ULONG size = 0x10000;
    LPVOID buffer = nullptr;
    NTSTATUS status;

    do {
        AllocMemory(size, &buffer);

        status = pNtQuerySystemInformation(
            SystemExtendedHandleInformation,
            buffer,
            size,
            &size
        );

        if (status == STATUS_INFO_LENGTH_MISMATCH)
            size *= 2;

    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    if (status < 0) {
        std::cout << "Query failed\n";
        CloseHandle(hFile);
        return 1;
    }

    auto info = (SYSTEM_HANDLE_INFORMATION_EX*)buffer;

    for (ULONG_PTR i = 0; i < info->NumberOfHandles; i++) {
        auto& h = info->Handles[i];

        HANDLE handle = (HANDLE)h.HandleValue;

        //if (GetFileType(handle) != FILE_TYPE_DISK)
        //    continue;
        HANDLE hCopy = NULL;
        constexpr unsigned int hashDuplicateHandle = ComplexHashForAnsi("DuplicateHandle");
        typedef BOOL(WINAPI* _DuplicateHandle)(
            HANDLE, HANDLE, HANDLE, LPHANDLE, DWORD, BOOL, DWORD
            );
        _DuplicateHandle pDuplicateHandle = (_DuplicateHandle)apiResolve.GetApiAddress(lpKernel32, hashDuplicateHandle);
        BOOL dupOk = pDuplicateHandle(
            (HANDLE) -1,
            handle,
            (HANDLE) -1,
            &hCopy,
            GENERIC_READ,
            FALSE,
            0
        );

        if (!dupOk) {
            //std::cout << "DuplicateHandle failed: " << GetLastError() << "\n";
            continue;
        }


        char path[MAX_PATH] = { 0 };
        constexpr unsigned int hashGetFinalPathNameByHandleA = ComplexHashForAnsi("GetFinalPathNameByHandleA");
        typedef DWORD(WINAPI* _GetFinalPathNameByHandleA)(
            HANDLE,
            LPSTR,
            DWORD,
            DWORD
            );
        _GetFinalPathNameByHandleA pGetFinalPathNameByHandleA = (_GetFinalPathNameByHandleA)apiResolve.GetApiAddress(lpKernel32, hashGetFinalPathNameByHandleA);
        DWORD len = pGetFinalPathNameByHandleA(
            hCopy,
            path,
            MAX_PATH,
            FILE_NAME_NORMALIZED
        );

        if (len == 0 || len >= MAX_PATH)
            continue;

        std::cout
            << "Handle: 0x" << std::hex << (ULONG_PTR)hCopy
            << " | Path: " << path
            << "\n";

        if (strstr(path, "test.txt") != nullptr) {
            constexpr unsigned int hashCreateFileA = ComplexHashForAnsi("CreateFileA");
            typedef HANDLE(WINAPI* _CreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
            _CreateFileA pCreateFileA = (_CreateFileA)apiResolve.GetApiAddress(lpKernel32, hashCreateFileA);
            HANDLE hOut = pCreateFileA(
                "text3.txt",
                GENERIC_WRITE,
                0,
                NULL,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );

            if (hOut == INVALID_HANDLE_VALUE)
                return false;

            char buffer[4096];
            DWORD bytesRead = 0;
            DWORD bytesWritten = 0;

            ULONGLONG offset = 0;

            while (true) {
                OVERLAPPED ov = {};
                ov.Offset = (DWORD)(offset & 0xFFFFFFFF);
                ov.OffsetHigh = (DWORD)(offset >> 32);
                
                constexpr unsigned int hashReadFile = ComplexHashForAnsi("ReadFile");
                typedef BOOL(WINAPI* _ReadFile)(
                    HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED
                    );
                _ReadFile pReadFile = (_ReadFile)apiResolve.GetApiAddress(lpKernel32, hashReadFile);
                BOOL ok = pReadFile(
                    hCopy,
                    buffer,
                    sizeof(buffer),
                    &bytesRead,
                    &ov
                );

                if (!ok) {
                    DWORD err = GetLastError();

                    if (err == ERROR_HANDLE_EOF)
                        break;

                    std::cout << "ReadFile failed: " << err << "\n";
                    CloseHandle(hOut);
                    return false;
                }

                if (bytesRead == 0)
                    break;

                constexpr unsigned int hashWriteFile = ComplexHashForAnsi("WriteFile");
                typedef BOOL(WINAPI* _WriteFile)(
                    HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED
                    );
                _WriteFile pWriteFile = (_WriteFile)apiResolve.GetApiAddress(lpKernel32, hashWriteFile);
                if (!pWriteFile(hOut, buffer, bytesRead, &bytesWritten, NULL)) {
                    CloseHandle(hOut);
                    return false;
                }

                offset += bytesRead;
            }

            CloseHandle(hOut);
            break;
        }
    }

    FreeMemory(buffer);
    CloseHandle(hFile);

    return 0;
}
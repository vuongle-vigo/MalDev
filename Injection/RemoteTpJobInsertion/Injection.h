#pragma once
#include <Windows.h>
#include <string>
#include "ApiResolve.h"


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


bool GetFileDataW(_In_ const std::wstring wsFilepath, _Out_ LPVOID* lpBuffer, _Out_ DWORD* dwFileSize);
HANDLE GetProcHandlebyName(LPWSTR procName);
bool MappingShellcode(_In_ HANDLE hTargetProcess, _In_ LPVOID lpShellcode, _In_ DWORD dwShellSize, _Out_ LPVOID* lpRemoteAddr, _In_ ULONG PageProtection);
bool RemoteTpJobInsertion(LPVOID lpShellcode, DWORD dwShellSize);
bool CreateRemoteThreadInject(LPVOID lpShellcode, DWORD dwShellSize, wchar_t* wsProcName);
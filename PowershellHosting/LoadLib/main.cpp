#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>

typedef BOOL(*fnPS_Init)(void);
typedef BOOL(*fnPS_IsInitialized)(void);
typedef BSTR(*fnPS_Execute)(LPCWSTR);
typedef void (*fnPS_Shutdown)(void);
typedef const wchar_t* (*fnPS_GetLastError)(void);

static void PrintBstr(const wchar_t* label, BSTR b) {
    if (b) {
        wprintf(L"%s%s\n", label, b);
        SysFreeString(b);
    } else {
        wprintf(L"%s<null>\n", label);
    }
}

int main() {
    HMODULE h = LoadLibraryA("powershellhosting.dll");
    if (!h) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }

    fnPS_Init          pInit          = (fnPS_Init)GetProcAddress(h, "PS_Init");
    fnPS_IsInitialized pIsInitialized = (fnPS_IsInitialized)GetProcAddress(h, "PS_IsInitialized");
    fnPS_Execute       pExecute       = (fnPS_Execute)GetProcAddress(h, "PS_Execute");
    fnPS_Shutdown      pShutdown      = (fnPS_Shutdown)GetProcAddress(h, "PS_Shutdown");
    fnPS_GetLastError  pGetLastError  = (fnPS_GetLastError)GetProcAddress(h, "PS_GetLastError");

    if (!pInit || !pIsInitialized || !pExecute || !pShutdown || !pGetLastError) {
        printf("GetProcAddress failed\n");
        FreeLibrary(h);
        return 1;
    }

    // 2) Init
    if (!pInit()) {
        const wchar_t* err = pGetLastError();
        wprintf(L"PS_Init failed: %s\n", err ? err : L"(no message)");
        FreeLibrary(h);
        return 1;
    }
    //wprintf(L"[2] After init - PS_IsInitialized() = %s\n",
    //    pIsInitialized() ? L"TRUE" : L"FALSE");
    std::wstring input;

    while (true) {
        std::cout << "PS> ";
        std::getline(std::wcin, input);
        // Exit on "exit" command
        if (input == L"exit" || input == L"quit") {
            break;
        }

        BSTR out = pExecute(input.c_str());
        if (out) {
            PrintBstr(L"", out);
        }
    }

    // 6) Get last error
    const wchar_t* lastErr = pGetLastError();
    wprintf(L"[6] Last error: %s\n", lastErr ? lastErr : L"(none)");

    // 7) Shutdown
    pShutdown();
    wprintf(L"[7] After shutdown - PS_IsInitialized() = %s\n",
        pIsInitialized() ? L"TRUE" : L"FALSE");

    FreeLibrary(h);
    return 0;
}
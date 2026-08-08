#include <windows.h>
#include <stdio.h>

typedef BOOL(*fnPS_Init)(void);
typedef BSTR(*fnPS_Execute)(LPCWSTR);
typedef BOOL(*fnPS_Reset)(void);
typedef void (*fnPS_Shutdown)(void);

int main() {
    HMODULE h = LoadLibraryA("powershellhosting.dll");
    if (!h) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }

    fnPS_Init      pInit = (fnPS_Init)GetProcAddress(h, "PS_Init");
    fnPS_Execute   pExecute = (fnPS_Execute)GetProcAddress(h, "PS_Execute");
    fnPS_Reset     pReset = (fnPS_Reset)GetProcAddress(h, "PS_Reset");
    fnPS_Shutdown  pShutdown = (fnPS_Shutdown)GetProcAddress(h, "PS_Shutdown");

    if (!pInit || !pExecute || !pReset || !pShutdown) {
        printf("GetProcAddress failed\n");
        FreeLibrary(h);
        return 1;
    }

    if (!pInit()) {
        printf("PS_Init failed\n");
        FreeLibrary(h);
        return 1;
    }

    // Chạy lệnh
    BSTR out = pExecute(L"Get-Process | Select-Object -First 5 Name,Id | Out-String");
    if (out) {
        wprintf(L"%s\n", out);
        SysFreeString(out);
    }

    // Chạy lệnh khác — session vẫn còn
    BSTR out2 = pExecute(L"$testString = \"AMSI Test Sample : \" + \"7e72c3ce - 861b - 4339 - 8740 - 0ac1484c1386\"");
    if (out2) {
        wprintf(L"%s\n", out2);
        SysFreeString(out2);
    }

    BSTR out3 = pExecute(L"Invoke-Expression $testString");
    if (out3) {
        wprintf(L"%s\n", out3);
        SysFreeString(out3);
    }

    pShutdown();
    FreeLibrary(h);
    return 0;
}
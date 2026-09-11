#include "apilex.h"

int main() {
    ApiResolve apiResolve{};
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    _LoadLibraryA pLoadLibraryA = (_LoadLibraryA)apiResolve.GetApiAddress(lpKernel32, hashLoadLibraryA);
    const char* sUser32 = "user32.dll";
    LPVOID lpUser32 = pLoadLibraryA(sUser32);
    _MessageBoxA pMessageBoxA = (_MessageBoxA)apiResolve.GetApiAddress(lpUser32, hashMessageBoxA);
	const char* msg = "Hello";
	pMessageBoxA(NULL, msg, msg, MB_OK);
	return 1;
}
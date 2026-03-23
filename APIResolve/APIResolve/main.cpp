#include "ApiResolve.h"
#include "CRT.h"

typedef int (WINAPI* _MessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
typedef HMODULE(WINAPI* _LoadLibraryA)(LPCSTR lpLibFileName);

int main() {
	ApiResolve apiResolve;
	WCHAR wsKernel32[] = {'k','e','r','n','e','l','3','2','.','d','l','l',L'\0'};
	LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress((LPWSTR)wsKernel32);
	CHAR wsLoadLibraryA[] = {'L','o','a','d','L','i','b','r','a','r','y','A',L'\0'};
	_LoadLibraryA loadLibFunction = (_LoadLibraryA)apiResolve.GetApiAddress(lpKernel32, wsLoadLibraryA);
	CHAR sUser32[] = { 'u','s','e','r','3','2','.','d','l','l',L'\0' };
	HMODULE hUser32 = loadLibFunction(sUser32);
	WCHAR wsUser32[] = { 'u','s','e','r','3','2','.','d','l','l',L'\0' };
	LPVOID lpUser32 = apiResolve.GetModuleBaseAddress((LPWSTR)wsUser32);
	CHAR sMessageBoxA[] = { 'M','e','s','s','a','g','e','B','o','x','A',L'\0' };
	_MessageBoxA msgBoxFunc = (_MessageBoxA)apiResolve.GetApiAddress(lpUser32, sMessageBoxA);
	CHAR sHello[] = { 'H','e','l','l','o',L'\0' };
	CHAR sApiResolve[] = { 'A','P','I',' ','R','e','s','o','l','v','e',L'\0' };
	msgBoxFunc(NULL, (LPCSTR)sHello, (LPCSTR)sApiResolve, MB_OK);
	return 1;

}
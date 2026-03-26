#include "CRT.h"
#include "ApiResolve.h"
#include "HashString.h"

bool LowerStringA(char* s) {
	for (int i = 0; ; i++) {
		if (s[i] == '\0') {
			break;
		}

		if (s[i] >= 'A' && s[i] <= 'Z') {
			s[i] += 32;
		}
	}

	return true;
}

bool LowerStringW(wchar_t* s) {
	for (int i = 0; ; i++) {
		if (s[i] == L'\0') {
			break;
		}
		if (s[i] >= L'A' && s[i] <= L'Z') {
			s[i] += 32;
		}
	}

	return true;
}

bool UpperStringA(char* s) {
	for (int i = 0; ; i++) {
		if (s[i] == '\0') {
			break;
		}
		if (s[i] >= 'a' && s[i] <= 'z') {
			s[i] -= 32;
		}
	}

	return true;
}

bool UpperStringW(wchar_t* s) {
	for (int i = 0; ; i++) {
		if (s[i] == L'\0') {
			break;
		}
		if (s[i] >= L'a' && s[i] <= L'z') {
			s[i] -= 32;
		}
	}

	return true;
}

bool CompareStringA(const char* s1, const char* s2) {
	for (int i = 0; ; i++) {
		if (s1[i] == '\0' && s2[i] == '\0') {
			return true;
		}
		if (s1[i] != s2[i]) {
			return false;
		}
	}
}

bool CompareStringW(const wchar_t* s1, const wchar_t* s2) {
	for (int i = 0; ; i++) {
		if (s1[i] == L'\0' && s2[i] == L'\0') {
			return true;
		}
		if (s1[i] != s2[i]) {
			return false;
		}
	}
}

bool FindPatternA(const char* s, const char* pattern, size_t sizePattern, char** result) {
	for (int i = 0; ; i++) {
		if (s[i] == '\0') {
			return false;
		}

		char* p = (char*)(s + i);
		for (int j = 0; j < sizePattern; j++) {
			if (pattern[j] == '\0') {
				*result = p;
				return true;
			}

			if (p[j] != pattern[j]) {
				break;
			}

			if (p[j] == '\0') {
				return false;
			}

			if (j == sizePattern - 1) {
				*result = p;
				return true;
			}
		}
	}
}

bool FindPatternW(const wchar_t* s, const wchar_t* pattern, size_t sizePattern, wchar_t** result) {
	for (int i = 0; ; i++) {
		if (s[i] == L'\0') {
			return false;
		}
		wchar_t* p = (wchar_t*)(s + i);
		for (int j = 0; j < sizePattern; j++) {
			if (pattern[j] == L'\0') {
				*result = p;
				return true;
			}
			if (p[j] != pattern[j]) {
				break;
			}
			if (p[j] == L'\0') {
				return false;
			}
			if (j == sizePattern - 1) {
				*result = p;
				return true;
			}
		}
	}
}

bool CopyStringA(const char* src, char* dst, size_t sizeDst) {
	for (int i = 0; i < sizeDst; i++) {
		dst[i] = src[i];
		if (src[i] == '\0') {
			return true;
		}
	}
	return false;
}

bool CopyStringW(const wchar_t* src, wchar_t* dst, size_t sizeDst) {
	for (int i = 0; i < sizeDst; i++) {
		dst[i] = src[i];
		if (src[i] == L'\0') {
			return true;
		}
	}
	return false;
}

size_t StrLen(char* s) {
	size_t size = 0;
	while (true) {
		if (s[size] == '\0') {
			break;
		}
		
		size++;
	}

	return size;
}

bool CopyMemoryV(void* dst, const void* src, SIZE_T size) {
	BYTE* d = (BYTE*)dst;
	BYTE* s = (BYTE*)src;

	for (SIZE_T i = 0; i < size; i++) {
		d[i] = s[i];
	}

	return TRUE;
}

bool AllocMemory(size_t size, LPVOID* result) {
	typedef LPVOID(NTAPI* _RtlAllocateHeap)(
		PVOID HeapHandle,
		ULONG Flags,
		SIZE_T Size
		);
	typedef HANDLE(WINAPI* _GetProcessHeap)();
	ApiResolve apiResolver;
	constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
	LPVOID lpKernel32 = apiResolver.GetModuleBaseAddress(hashKernel32);
	if (lpKernel32 == NULL) {
		return false;
	}

	constexpr unsigned int hashGetProcessHeap = ComplexHashForAnsi("GetProcessHeap");
	_GetProcessHeap pGetProcessHeap = (_GetProcessHeap)apiResolver.GetApiAddress(lpKernel32, hashGetProcessHeap);
	if (pGetProcessHeap == NULL) {
		return false;
	}

	constexpr unsigned int hashNtdll = ComplexHashForWChar(L"ntdll.dll");
	LPVOID lpNtdll = apiResolver.GetModuleBaseAddress(hashNtdll);
	if (lpNtdll == NULL) {
		return false;
	}

	constexpr unsigned int hashRtlAllocateHeap = ComplexHashForAnsi("RtlAllocateHeap");
	_RtlAllocateHeap pRtlAllocateHeap = (_RtlAllocateHeap)apiResolver.GetApiAddress(lpNtdll, hashRtlAllocateHeap);
	if (pRtlAllocateHeap == NULL) {
		return false;
	}

	HANDLE hHeap = pGetProcessHeap();
	if (hHeap == NULL) {
		return false;
	}

	*result = pRtlAllocateHeap(hHeap, HEAP_ZERO_MEMORY, size);
	return true;
}

bool FreeMemory(LPVOID lpMemory) {
	typedef BOOL(WINAPI* _HeapFree)(
		HANDLE,
		DWORD,
		LPVOID
		);
	typedef HANDLE(WINAPI* _GetProcessHeap)();
	ApiResolve apiResolve;
	LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(ComplexHashForWChar(L"kernel32.dll"));
	if (!lpKernel32) {
		return false;
	}

	constexpr unsigned int hashGetProcessHeap = ComplexHashForAnsi("GetProcessHeap");
	_GetProcessHeap pGetProcessHeap = (_GetProcessHeap)apiResolve.GetApiAddress(lpKernel32, hashGetProcessHeap);
	if (!pGetProcessHeap) {
		return false;
	}

	constexpr unsigned int hashHeapFree = ComplexHashForAnsi("HeapFree");
	_HeapFree pHeapFree = (_HeapFree)apiResolve.GetApiAddress(lpKernel32, hashHeapFree);
	if (!pHeapFree) {
		return false;
	}

	HANDLE hHeap = pGetProcessHeap();
	if (hHeap == NULL) {
		return false;
	}

	if (!pHeapFree(hHeap, 0, lpMemory)) {
		return false;
	}

	return true;
}
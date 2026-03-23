#include "CRT.h"

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

bool FindPatternA(const char* s, const char* pattern, size_t sizePattern ,char** result) {
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
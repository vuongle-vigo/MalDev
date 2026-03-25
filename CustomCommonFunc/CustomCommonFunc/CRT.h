#pragma once
#include <Windows.h>

bool LowerStringA(char* s);
bool LowerStringW(wchar_t* s);
bool UpperStringA(char* s);
bool UpperStringW(wchar_t* s);
bool CompareStringA(const char* s1, const char* s2);
bool CompareStringW(const wchar_t* s1, const wchar_t* s2);

bool FindPatternA(const char* s, const char* pattern, size_t sizePattern, char** result);
bool FindPatternW(const wchar_t* s, const wchar_t* pattern, size_t sizePattern, wchar_t** result);

bool CopyStringA(const char* src, char* dst, size_t sizeDst);
bool CopyStringW(const wchar_t* src, wchar_t* dst, size_t sizeDst);

bool AllocMemory(size_t size, LPVOID* result);
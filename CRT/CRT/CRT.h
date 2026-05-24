#pragma once

#define CRT_VERSION "1.0.0"

#include <windows.h>

//=============================================================================
// MEMORY ALLOCATION FUNCTIONS
//=============================================================================

_Post_equal_to_(0) _Ret_maybenull_ _Post_writable_byte_size_(Size) 
void* __cdecl crt_malloc(SIZE_T Size);

_Post_equal_to_(0) _Ret_maybenull_ _Post_writable_byte_size_(Count * Size) 
void* __cdecl crt_calloc(SIZE_T Count, SIZE_T Size);

_Ret_maybenull_ _Post_writable_byte_size_(NewSize)
void* __cdecl crt_realloc(void* Block, SIZE_T NewSize);

void __cdecl crt_free(void* Block);

//=============================================================================
// STRING FUNCTIONS (CHAR)
//=============================================================================

// String length
SIZE_T __cdecl crt_strlen(const char* str);

// String copy
char* __cdecl crt_strcpy(char* dest, const char* src);
char* __cdecl crt_strncpy(char* dest, const char* src, SIZE_T count);

// String concatenation
char* __cdecl crt_strcat(char* dest, const char* src);
char* __cdecl crt_strncat(char* dest, const char* src, SIZE_T count);

// String comparison
int __cdecl crt_strcmp(const char* str1, const char* str2);
int __cdecl crt_strncmp(const char* str1, const char* str2, SIZE_T count);
int __cdecl crt_strcmpi(const char* str1, const char* str2);
int __cdecl crt_strncmpi(const char* str1, const char* str2, SIZE_T count);

// String searching
char* __cdecl crt_strchr(const char* str, int ch);
char* __cdecl crt_strrchr(const char* str, int ch);
SIZE_T __cdecl crt_strspn(const char* str, const char* accept);
SIZE_T __cdecl crt_strcspn(const char* str, const char* reject);
char* __cdecl crt_strpbrk(const char* str, const char* accept);
char* __cdecl crt_strstr(const char* str, const char* substr);

// String transformation
char* __cdecl crt_strupr(char* str);
char* __cdecl crt_strlwr(char* str);

// String splitting
char* __cdecl crt_strtok(char* str, const char* delim);

// Memory functions for char strings
void* __cdecl crt_memset(void* dest, int val, SIZE_T count);
void* __cdecl crt_memcpy(void* dest, const void* src, SIZE_T count);
void* __cdecl crt_memmove(void* dest, const void* src, SIZE_T count);
int __cdecl crt_memcmp(const void* buf1, const void* buf2, SIZE_T count);
void* __cdecl crt_memchr(const void* buf, int ch, SIZE_T count);

//=============================================================================
// STRING FUNCTIONS (WCHAR)
//=============================================================================

// Wide string length
SIZE_T __cdecl crt_wcslen(const wchar_t* str);

// Wide string copy
wchar_t* __cdecl crt_wcscpy(wchar_t* dest, const wchar_t* src);
wchar_t* __cdecl crt_wcsncpy(wchar_t* dest, const wchar_t* src, SIZE_T count);

// Wide string concatenation
wchar_t* __cdecl crt_wcscat(wchar_t* dest, const wchar_t* src);
wchar_t* __cdecl crt_wcsncat(wchar_t* dest, const wchar_t* src, SIZE_T count);

// Wide string comparison
int __cdecl crt_wcscmp(const wchar_t* str1, const wchar_t* str2);
int __cdecl crt_wcsncmp(const wchar_t* str1, const wchar_t* str2, SIZE_T count);
int __cdecl crt_wcscmpi(const wchar_t* str1, const wchar_t* str2);
int __cdecl crt_wcsncmpi(const wchar_t* str1, const wchar_t* str2, SIZE_T count);

// Wide string searching
wchar_t* __cdecl crt_wcschr(const wchar_t* str, wchar_t ch);
wchar_t* __cdecl crt_wcsrchr(const wchar_t* str, wchar_t ch);
SIZE_T __cdecl crt_wcsspn(const wchar_t* str, const wchar_t* accept);
SIZE_T __cdecl crt_wcscspn(const wchar_t* str, const wchar_t* reject);
wchar_t* __cdecl crt_wcspbrk(const wchar_t* str, const wchar_t* accept);
wchar_t* __cdecl crt_wcsstr(const wchar_t* str, const wchar_t* substr);

// Wide string transformation
wchar_t* __cdecl crt_wcsupr(wchar_t* str);
wchar_t* __cdecl crt_wcslwr(wchar_t* str);

// Wide string splitting
wchar_t* __cdecl crt_wcstok(wchar_t* str, const wchar_t* delim);

//=============================================================================
// STRING CONVERSION FUNCTIONS
//=============================================================================

// Char to Wchar conversion
wchar_t* __cdecl crt_atowc(const char* str);
char* __cdecl crt_wctoa(const wchar_t* str);

// String to Number conversion (char)
int __cdecl crt_atoi(const char* str);
long __cdecl crt_atol(const char* str);
long long __cdecl crt_atoll(const char* str);
unsigned int __cdecl crt_atoui(const char* str);
unsigned long __cdecl crt_atoul(const char* str);
unsigned long long __cdecl crt_atoull(const char* str);

// String to Number conversion (wchar)
int __cdecl crt_wtoi(const wchar_t* str);
long __cdecl crt_wtol(const wchar_t* str);
long long __cdecl crt_wtoll(const wchar_t* str);
unsigned int __cdecl crt_wtoui(const wchar_t* str);
unsigned long __cdecl crt_wtoul(const wchar_t* str);
unsigned long long __cdecl crt_wtoull(const wchar_t* str);

// Number to String conversion (char)
char* __cdecl crt_itoa(int value, char* str, int radix);
char* __cdecl crt_ltoa(long value, char* str, int radix);
char* __cdecl crt_lltoa(long long value, char* str, int radix);
char* __cdecl crt_uitoa(unsigned int value, char* str, int radix);
char* __cdecl crt_ultoa(unsigned long value, char* str, int radix);
char* __cdecl crt_ulltoa(unsigned long long value, char* str, int radix);

// Number to String conversion (wchar)
wchar_t* __cdecl crt_itow(int value, wchar_t* str, int radix);
wchar_t* __cdecl crt_ltow(long value, wchar_t* str, int radix);
wchar_t* __cdecl crt_lltow(long long value, wchar_t* str, int radix);
wchar_t* __cdecl crt_uwtow(unsigned int value, wchar_t* str, int radix);
wchar_t* __cdecl crt_ultow(unsigned long value, wchar_t* str, int radix);
wchar_t* __cdecl crt_ulltow(unsigned long long value, wchar_t* str, int radix);

//=============================================================================
// UTILITY FUNCTIONS
//=============================================================================

void __cdecl crt_qsort(void* base, SIZE_T num, SIZE_T width, 
                       int (__cdecl* compare)(const void*, const void*));

void* __cdecl crt_bsearch(const void* key, const void* base, 
                          SIZE_T num, SIZE_T width,
                          int (__cdecl* compare)(const void*, const void*));

void __cdecl crt_srand(unsigned int seed);
int __cdecl crt_rand(void);

int __cdecl crt_abs(int value);
long __cdecl crt_labs(long value);
long long __cdecl crt_llabs(long long value);

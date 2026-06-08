#pragma once

#define CRT_EOF (-1)
#define CRT_WEOF ((wint_t)(-1))

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

// String to Float conversion (char)
float __cdecl crt_strtof(const char* str, char** endptr);
double __cdecl crt_strtod(const char* str, char** endptr);
long double __cdecl crt_strtold(const char* str, char** endptr);

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
// CHARACTER CLASSIFICATION FUNCTIONS (CHAR)
//=============================================================================

int __cdecl crt_isdigit(int ch);
int __cdecl crt_isalpha(int ch);
int __cdecl crt_isalnum(int ch);
int __cdecl crt_isspace(int ch);
int __cdecl crt_isupper(int ch);
int __cdecl crt_islower(int ch);
int __cdecl crt_isxdigit(int ch);
int __cdecl crt_isprint(int ch);
int __cdecl crt_ispunct(int ch);
int __cdecl crt_iscntrl(int ch);
int __cdecl crt_isgraph(int ch);
int __cdecl crt_toupper(int ch);
int __cdecl crt_tolower(int ch);

//=============================================================================
// WIDE-CHARACTER CLASSIFICATION FUNCTIONS
//=============================================================================

int __cdecl crt_iswalnum(wint_t ch);
int __cdecl crt_iswalpha(wint_t ch);
int __cdecl crt_iswdigit(wint_t ch);
int __cdecl crt_iswspace(wint_t ch);
int __cdecl crt_iswupper(wint_t ch);
int __cdecl crt_iswlower(wint_t ch);
int __cdecl crt_iswxdigit(wint_t ch);
wint_t __cdecl crt_towupper(wint_t ch);
wint_t __cdecl crt_towlower(wint_t ch);

//=============================================================================
// WIDE-CHARACTER TO NUMBER CONVERSION (wcstox family)
//=============================================================================

unsigned long __cdecl crt_wcstoul(const wchar_t* nptr, wchar_t** endptr, int base);
unsigned int  __cdecl crt_wcstoui(const wchar_t* nptr, wchar_t** endptr, int base);
long           __cdecl crt_wcstol(const wchar_t* nptr, wchar_t** endptr, int base);
long long      __cdecl crt_wcstoll(const wchar_t* nptr, wchar_t** endptr, int base);
unsigned long long __cdecl crt_wcstoull(const wchar_t* nptr, wchar_t** endptr, int base);
unsigned long  __cdecl crt_wcstoull_val(const wchar_t* nptr, wchar_t** endptr, int base);

//=============================================================================
// FORMATTED OUTPUT FUNCTIONS
//=============================================================================

// Wide-character
int __cdecl crt_swprintf(wchar_t* buf, SIZE_T count, const wchar_t* fmt, ...);
int __cdecl crt_vswprintf(wchar_t* buf, SIZE_T count, const wchar_t* fmt, va_list args);
int __cdecl crt_swprintf_s(wchar_t* buf, SIZE_T bufSize, const wchar_t* fmt, ...);
int __cdecl crt_vswprintf_s(wchar_t* buf, SIZE_T bufSize, const wchar_t* fmt, va_list args);

// Char
int __cdecl crt_printf(const char* fmt, ...);
int __cdecl crt_vprintf(const char* fmt, va_list args);
int __cdecl crt_sprintf(char* buf, const char* fmt, ...);
int __cdecl crt_vsprintf(char* buf, const char* fmt, va_list args);
int __cdecl crt_snprintf(char* buf, SIZE_T count, const char* fmt, ...);
int __cdecl crt_vsnprintf(char* buf, SIZE_T count, const char* fmt, va_list args);

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

//=============================================================================
// FILE I/O FUNCTIONS (WinAPI-based)
//=============================================================================

typedef struct crt_FILE crt_FILE;

crt_FILE* __cdecl crt_fopen(const char* filename, const char* mode);
crt_FILE* __cdecl crt_wfopen(const wchar_t* filename, const wchar_t* mode);
int       __cdecl crt_fclose(crt_FILE* stream);

SIZE_T    __cdecl crt_fread(void* buffer, SIZE_T size, SIZE_T count, crt_FILE* stream);
SIZE_T    __cdecl crt_fwrite(const void* buffer, SIZE_T size, SIZE_T count, crt_FILE* stream);

int       __cdecl crt_fseek(crt_FILE* stream, long offset, int origin);
long      __cdecl crt_ftell(crt_FILE* stream);
void      __cdecl crt_rewind(crt_FILE* stream);
int       __cdecl crt_fflush(crt_FILE* stream);

int       __cdecl crt_feof(crt_FILE* stream);
int       __cdecl crt_ferror(crt_FILE* stream);
void      __cdecl crt_clearerr(crt_FILE* stream);

int       __cdecl crt_remove(const char* filename);
int       __cdecl crt_wremove(const wchar_t* filename);
int       __cdecl crt_rename(const char* oldname, const char* newname);
int       __cdecl crt_wrename(const wchar_t* oldname, const wchar_t* newname);

BOOL      __cdecl crt_fileexists(const char* filename);
BOOL      __cdecl crt_wfileexists(const wchar_t* filename);
long long __cdecl crt_filesize(const char* filename);
long long __cdecl crt_wfilesize(const wchar_t* filename);

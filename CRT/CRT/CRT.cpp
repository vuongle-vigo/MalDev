#include "CRT.h"
#include "HashString.h"
#include "ApiResolve.h"

//=============================================================================
// MEMORY ALLOCATION FUNCTIONS
//=============================================================================

void* __cdecl crt_malloc(SIZE_T Size) {
    if (Size == 0) {
        Size = 1;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    constexpr unsigned int hashGetProcessHeap = ComplexHashForAnsi("GetProcessHeap");
    typedef HANDLE
        (WINAPI*
        _GetProcessHeap)(
            VOID
        );

    _GetProcessHeap pGetProcessHeap = (_GetProcessHeap)apiResolve.GetApiAddress(lpKernel32, hashGetProcessHeap);
    typedef LPVOID
        (WINAPI*
        _HeapAlloc)(
            _In_ HANDLE hHeap,
            _In_ DWORD dwFlags,
            _In_ SIZE_T dwBytes
        );
    constexpr unsigned int hashHeapAlloc = ComplexHashForAnsi("HeapAlloc");
    _HeapAlloc pHeapAlloc = (_HeapAlloc)apiResolve.GetApiAddress(lpKernel32, hashHeapAlloc);
    return pHeapAlloc(pGetProcessHeap(), HEAP_ZERO_MEMORY, Size);
}

void* __cdecl crt_calloc(SIZE_T Count, SIZE_T Size) {
    if (Count == 0 || Size == 0) {
        return NULL;
    }
    SIZE_T totalSize = Count * Size;
    if (totalSize / Count != Size) {
        return NULL;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    constexpr unsigned int hashGetProcessHeap = ComplexHashForAnsi("GetProcessHeap");
    typedef HANDLE
    (WINAPI*
        _GetProcessHeap)(
            VOID
            );
    _GetProcessHeap pGetProcessHeap = (_GetProcessHeap)apiResolve.GetApiAddress(lpKernel32, hashGetProcessHeap);
    typedef LPVOID
    (WINAPI*
        _HeapAlloc)(
            _In_ HANDLE hHeap,
            _In_ DWORD dwFlags,
            _In_ SIZE_T dwBytes
            );
    constexpr unsigned int hashHeapAlloc = ComplexHashForAnsi("HeapAlloc");
    _HeapAlloc pHeapAlloc = (_HeapAlloc)apiResolve.GetApiAddress(lpKernel32, hashHeapAlloc);
    return pHeapAlloc(pGetProcessHeap(), HEAP_ZERO_MEMORY, totalSize);
}

void* __cdecl crt_realloc(void* Block, SIZE_T NewSize) {
    if (Block == NULL) {
        return crt_malloc(NewSize);
    }
    if (NewSize == 0) {
        crt_free(Block);
        return NULL;
    }
    
    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    constexpr unsigned int hashGetProcessHeap = ComplexHashForAnsi("GetProcessHeap");
    typedef HANDLE
    (WINAPI*
        _GetProcessHeap)(
            VOID
            );
    _GetProcessHeap pGetProcessHeap = (_GetProcessHeap)apiResolve.GetApiAddress(lpKernel32, hashGetProcessHeap);
    typedef SIZE_T
        (WINAPI*
        _HeapSize)(
            _In_ HANDLE hHeap,
            _In_ DWORD dwFlags,
            _In_ LPCVOID lpMem
        );
    constexpr unsigned int hashHeapSize = ComplexHashForAnsi("HeapSize");
    _HeapSize pHeapSize = (_HeapSize)apiResolve.GetApiAddress(lpKernel32, hashHeapSize);
    SIZE_T oldSize = pHeapSize(pGetProcessHeap(), 0, Block);
    typedef LPVOID
    (WINAPI*
        _HeapAlloc)(
            _In_ HANDLE hHeap,
            _In_ DWORD dwFlags,
            _In_ SIZE_T dwBytes
            );
    constexpr unsigned int hashHeapAlloc = ComplexHashForAnsi("HeapAlloc");
    _HeapAlloc pHeapAlloc = (_HeapAlloc)apiResolve.GetApiAddress(lpKernel32, hashHeapAlloc);
    void* newBlock = pHeapAlloc(pGetProcessHeap(), HEAP_ZERO_MEMORY, NewSize);
    
    if (newBlock == NULL) {
        return NULL;
    }
    
    if (oldSize != (SIZE_T)-1) {
        SIZE_T copySize = (oldSize < NewSize) ? oldSize : NewSize;
        crt_memcpy(newBlock, Block, copySize);
        typedef BOOL
            (WINAPI*
            _HeapFree)(
                _Inout_ HANDLE hHeap,
                _In_ DWORD dwFlags,
                __drv_freesMem(Mem) _Frees_ptr_opt_ LPVOID lpMem
            );
        constexpr unsigned int hashHeapFree = ComplexHashForAnsi("HeapFree");
        _HeapFree pHeapFree = (_HeapFree)apiResolve.GetApiAddress(lpKernel32, hashHeapFree);
        pHeapFree(pGetProcessHeap(), 0, Block);
    }
    
    return newBlock;
}

void __cdecl crt_free(void* Block) {
    if (Block != NULL) {
        ApiResolve apiResolve;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
        typedef BOOL
        (WINAPI*
            _HeapFree)(
                _Inout_ HANDLE hHeap,
                _In_ DWORD dwFlags,
                __drv_freesMem(Mem) _Frees_ptr_opt_ LPVOID lpMem
                );
        constexpr unsigned int hashHeapFree = ComplexHashForAnsi("HeapFree");
        _HeapFree pHeapFree = (_HeapFree)apiResolve.GetApiAddress(lpKernel32, hashHeapFree);
        constexpr unsigned int hashGetProcessHeap = ComplexHashForAnsi("GetProcessHeap");
        typedef HANDLE
        (WINAPI*
            _GetProcessHeap)(
                VOID
                );
        _GetProcessHeap pGetProcessHeap = (_GetProcessHeap)apiResolve.GetApiAddress(lpKernel32, hashGetProcessHeap);
        pHeapFree(pGetProcessHeap(), 0, Block);
    }
}

//=============================================================================
// STRING FUNCTIONS (CHAR)
//=============================================================================

SIZE_T __cdecl crt_strlen(const char* str) {
    const char* s = str;
    if (s == NULL) {
        return 0;
    }
    while (*s) {
        s++;
    }
    return (SIZE_T)(s - str);
}

char* __cdecl crt_strcpy(char* dest, const char* src) {
    char* originalDest = dest;
    if (dest == NULL || src == NULL) {
        return dest;
    }
    while ((*dest++ = *src++) != '\0') {
    }
    return originalDest;
}

char* __cdecl crt_strncpy(char* dest, const char* src, SIZE_T count) {
    char* originalDest = dest;
    if (dest == NULL || src == NULL) {
        return dest;
    }
    while (count > 0 && *src != '\0') {
        *dest++ = *src++;
        count--;
    }
    while (count > 0) {
        *dest++ = '\0';
        count--;
    }
    return originalDest;
}

char* __cdecl crt_strcat(char* dest, const char* src) {
    char* originalDest = dest;
    if (dest == NULL || src == NULL) {
        return dest;
    }
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++) != '\0') {
    }
    return originalDest;
}

char* __cdecl crt_strncat(char* dest, const char* src, SIZE_T count) {
    char* originalDest = dest;
    if (dest == NULL || src == NULL) {
        return dest;
    }
    while (*dest) {
        dest++;
    }
    while (count > 0 && *src != '\0') {
        *dest++ = *src++;
        count--;
    }
    *dest = '\0';
    return originalDest;
}

int __cdecl crt_strcmp(const char* str1, const char* str2) {
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    while (*str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return (*str1 - *str2);
}

int __cdecl crt_strncmp(const char* str1, const char* str2, SIZE_T count) {
    if (count == 0) return 0;
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    while (count > 1 && *str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
        count--;
    }
    return (*str1 - *str2);
}

static unsigned char to_lower_char(unsigned char c) {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

int __cdecl crt_strcmpi(const char* str1, const char* str2) {
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    while (*str1 && *str2 && to_lower_char(*str1) == to_lower_char(*str2)) {
        str1++;
        str2++;
    }
    return (to_lower_char(*str1) - to_lower_char(*str2));
}

int __cdecl crt_strncmpi(const char* str1, const char* str2, SIZE_T count) {
    if (count == 0) return 0;
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    while (count > 1 && *str1 && *str2 && 
           to_lower_char(*str1) == to_lower_char(*str2)) {
        str1++;
        str2++;
        count--;
    }
    return (to_lower_char(*str1) - to_lower_char(*str2));
}

char* __cdecl crt_strchr(const char* str, int ch) {
    if (str == NULL) {
        return NULL;
    }
    unsigned char target = (unsigned char)ch;
    while (*str) {
        if ((unsigned char)*str == target) {
            return (char*)str;
        }
        str++;
    }
    if (ch == 0) {
        return (char*)str;
    }
    return NULL;
}

char* __cdecl crt_strrchr(const char* str, int ch) {
    if (str == NULL) {
        return NULL;
    }
    const char* last = NULL;
    unsigned char target = (unsigned char)ch;
    
    do {
        if ((unsigned char)*str == target) {
            last = str;
        }
    } while (*str++);
    
    return (char*)last;
}

SIZE_T __cdecl crt_strspn(const char* str, const char* accept) {
    if (str == NULL || accept == NULL) {
        return 0;
    }
    
    const char* p = str;
    while (*p) {
        const char* a = accept;
        BOOL found = FALSE;
        while (*a) {
            if (*p == *a) {
                found = TRUE;
                break;
            }
            a++;
        }
        if (!found) {
            break;
        }
        p++;
    }
    return (SIZE_T)(p - str);
}

SIZE_T __cdecl crt_strcspn(const char* str, const char* reject) {
    if (str == NULL || reject == NULL) {
        return crt_strlen(str);
    }
    
    const char* p = str;
    while (*p) {
        const char* r = reject;
        while (*r) {
            if (*p == *r) {
                return (SIZE_T)(p - str);
            }
            r++;
        }
        p++;
    }
    return (SIZE_T)(p - str);
}

char* __cdecl crt_strpbrk(const char* str, const char* accept) {
    if (str == NULL || accept == NULL) {
        return NULL;
    }
    
    while (*str) {
        const char* a = accept;
        while (*a) {
            if (*str == *a) {
                return (char*)str;
            }
            a++;
        }
        str++;
    }
    return NULL;
}

char* __cdecl crt_strstr(const char* str, const char* substr) {
    if (str == NULL || substr == NULL) {
        return NULL;
    }
    
    if (*substr == '\0') {
        return (char*)str;
    }
    
    while (*str) {
        const char* s1 = str;
        const char* s2 = substr;
        
        while (*s1 && *s2 && *s1 == *s2) {
            s1++;
            s2++;
        }
        
        if (*s2 == '\0') {
            return (char*)str;
        }
        str++;
    }
    
    return NULL;
}

char* __cdecl crt_strupr(char* str) {
    if (str == NULL) {
        return NULL;
    }
    char* original = str;
    while (*str) {
        if (*str >= 'a' && *str <= 'z') {
            *str = *str - 32;
        }
        str++;
    }
    return original;
}

char* __cdecl crt_strlwr(char* str) {
    if (str == NULL) {
        return NULL;
    }
    char* original = str;
    while (*str) {
        if (*str >= 'A' && *str <= 'Z') {
            *str = *str + 32;
        }
        str++;
    }
    return original;
}

//static char* strtok_context = NULL;

char* __cdecl crt_strtok(char* str, const char* delim) {
    if (delim == NULL) {
        return NULL;
    }

    char* strtok_context = NULL;
    char* token;
    
    if (str != NULL) {
        token = str;
    } else {
        token = strtok_context;
    }
    
    if (token == NULL) {
        return NULL;
    }
    
    while (*token) {
        const char* d = delim;
        BOOL isDelim = FALSE;
        while (*d) {
            if (*token == *d) {
                isDelim = TRUE;
                break;
            }
            d++;
        }
        if (!isDelim) {
            break;
        }
        token++;
    }
    
    if (*token == '\0') {
        strtok_context = NULL;
        return NULL;
    }
    
    char* start = token;
    
    while (*token) {
        const char* d = delim;
        while (*d) {
            if (*token == *d) {
                *token = '\0';
                strtok_context = token + 1;
                return start;
            }
            d++;
        }
        token++;
    }
    
    strtok_context = NULL;
    return start;
}

//=============================================================================
// MEMORY FUNCTIONS
//=============================================================================

void* __cdecl crt_memset(void* dest, int val, SIZE_T count) {
    unsigned char* p = (unsigned char*)dest;
    unsigned char value = (unsigned char)val;
    while (count--) {
        *p++ = value;
    }
    return dest;
}

void* __cdecl crt_memcpy(void* dest, const void* src, SIZE_T count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (count--) {
        *d++ = *s++;
    }
    return dest;
}

void* __cdecl crt_memmove(void* dest, const void* src, SIZE_T count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    if (d == s) {
        return dest;
    }
    
    if (d < s) {
        while (count--) {
            *d++ = *s++;
        }
    } else {
        d += count;
        s += count;
        while (count--) {
            *--d = *--s;
        }
    }
    return dest;
}

int __cdecl crt_memcmp(const void* buf1, const void* buf2, SIZE_T count) {
    const unsigned char* b1 = (const unsigned char*)buf1;
    const unsigned char* b2 = (const unsigned char*)buf2;
    
    while (count--) {
        if (*b1 != *b2) {
            return (*b1 - *b2);
        }
        b1++;
        b2++;
    }
    return 0;
}

void* __cdecl crt_memchr(const void* buf, int ch, SIZE_T count) {
    const unsigned char* p = (const unsigned char*)buf;
    unsigned char target = (unsigned char)ch;
    
    while (count--) {
        if (*p == target) {
            return (void*)p;
        }
        p++;
    }
    return NULL;
}

//=============================================================================
// WSTRING FUNCTIONS (WCHAR)
//=============================================================================

SIZE_T __cdecl crt_wcslen(const wchar_t* str) {
    const wchar_t* s = str;
    if (s == NULL) {
        return 0;
    }
    while (*s) {
        s++;
    }
    return (SIZE_T)(s - str);
}

wchar_t* __cdecl crt_wcscpy(wchar_t* dest, const wchar_t* src) {
    wchar_t* originalDest = dest;
    if (dest == NULL || src == NULL) {
        return dest;
    }
    while ((*dest++ = *src++) != L'\0') {
    }
    return originalDest;
}

wchar_t* __cdecl crt_wcsncpy(wchar_t* dest, const wchar_t* src, SIZE_T count) {
    wchar_t* originalDest = dest;
    if (dest == NULL || src == NULL) {
        return dest;
    }
    while (count > 0 && *src != L'\0') {
        *dest++ = *src++;
        count--;
    }
    while (count > 0) {
        *dest++ = L'\0';
        count--;
    }
    return originalDest;
}

wchar_t* __cdecl crt_wcscat(wchar_t* dest, const wchar_t* src) {
    wchar_t* originalDest = dest;
    if (dest == NULL || src == NULL) {
        return dest;
    }
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++) != L'\0') {
    }
    return originalDest;
}

wchar_t* __cdecl crt_wcsncat(wchar_t* dest, const wchar_t* src, SIZE_T count) {
    wchar_t* originalDest = dest;
    if (dest == NULL || src == NULL) {
        return dest;
    }
    while (*dest) {
        dest++;
    }
    while (count > 0 && *src != L'\0') {
        *dest++ = *src++;
        count--;
    }
    *dest = L'\0';
    return originalDest;
}

int __cdecl crt_wcscmp(const wchar_t* str1, const wchar_t* str2) {
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    while (*str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
    }
    return (*str1 - *str2);
}

int __cdecl crt_wcsncmp(const wchar_t* str1, const wchar_t* str2, SIZE_T count) {
    if (count == 0) return 0;
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    while (count > 1 && *str1 && *str2 && *str1 == *str2) {
        str1++;
        str2++;
        count--;
    }
    return (*str1 - *str2);
}

int __cdecl crt_wcscmpi(const wchar_t* str1, const wchar_t* str2) {
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    while (*str1 && *str2 && 
           to_lower_char((unsigned char)*str1) == to_lower_char((unsigned char)*str2)) {
        str1++;
        str2++;
    }
    return (to_lower_char((unsigned char)*str1) - to_lower_char((unsigned char)*str2));
}

int __cdecl crt_wcsncmpi(const wchar_t* str1, const wchar_t* str2, SIZE_T count) {
    if (count == 0) return 0;
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    while (count > 1 && *str1 && *str2 && 
           to_lower_char((unsigned char)*str1) == to_lower_char((unsigned char)*str2)) {
        str1++;
        str2++;
        count--;
    }
    return (to_lower_char((unsigned char)*str1) - to_lower_char((unsigned char)*str2));
}

wchar_t* __cdecl crt_wcschr(const wchar_t* str, wchar_t ch) {
    if (str == NULL) {
        return NULL;
    }
    while (*str) {
        if (*str == ch) {
            return (wchar_t*)str;
        }
        str++;
    }
    if (ch == L'\0') {
        return (wchar_t*)str;
    }
    return NULL;
}

wchar_t* __cdecl crt_wcsrchr(const wchar_t* str, wchar_t ch) {
    if (str == NULL) {
        return NULL;
    }
    const wchar_t* last = NULL;
    
    do {
        if (*str == ch) {
            last = str;
        }
    } while (*str++);
    
    return (wchar_t*)last;
}

SIZE_T __cdecl crt_wcsspn(const wchar_t* str, const wchar_t* accept) {
    if (str == NULL || accept == NULL) {
        return 0;
    }
    
    const wchar_t* p = str;
    while (*p) {
        const wchar_t* a = accept;
        BOOL found = FALSE;
        while (*a) {
            if (*p == *a) {
                found = TRUE;
                break;
            }
            a++;
        }
        if (!found) {
            break;
        }
        p++;
    }
    return (SIZE_T)(p - str);
}

SIZE_T __cdecl crt_wcscspn(const wchar_t* str, const wchar_t* reject) {
    if (str == NULL || reject == NULL) {
        return crt_wcslen(str);
    }
    
    const wchar_t* p = str;
    while (*p) {
        const wchar_t* r = reject;
        while (*r) {
            if (*p == *r) {
                return (SIZE_T)(p - str);
            }
            r++;
        }
        p++;
    }
    return (SIZE_T)(p - str);
}

wchar_t* __cdecl crt_wcspbrk(const wchar_t* str, const wchar_t* accept) {
    if (str == NULL || accept == NULL) {
        return NULL;
    }
    
    while (*str) {
        const wchar_t* a = accept;
        while (*a) {
            if (*str == *a) {
                return (wchar_t*)str;
            }
            a++;
        }
        str++;
    }
    return NULL;
}

wchar_t* __cdecl crt_wcsstr(const wchar_t* str, const wchar_t* substr) {
    if (str == NULL || substr == NULL) {
        return NULL;
    }
    
    if (*substr == L'\0') {
        return (wchar_t*)str;
    }
    
    while (*str) {
        const wchar_t* s1 = str;
        const wchar_t* s2 = substr;
        
        while (*s1 && *s2 && *s1 == *s2) {
            s1++;
            s2++;
        }
        
        if (*s2 == L'\0') {
            return (wchar_t*)str;
        }
        str++;
    }
    
    return NULL;
}

wchar_t* __cdecl crt_wcsupr(wchar_t* str) {
    if (str == NULL) {
        return NULL;
    }
    wchar_t* original = str;
    while (*str) {
        if (*str >= L'a' && *str <= L'z') {
            *str = *str - 32;
        }
        str++;
    }
    return original;
}

wchar_t* __cdecl crt_wcslwr(wchar_t* str) {
    if (str == NULL) {
        return NULL;
    }
    wchar_t* original = str;
    while (*str) {
        if (*str >= L'A' && *str <= L'Z') {
            *str = *str + 32;
        }
        str++;
    }
    return original;
}

//static wchar_t* wcstok_context = NULL;

wchar_t* __cdecl crt_wcstok(wchar_t* str, const wchar_t* delim) {
    if (delim == NULL) {
        return NULL;
    }
    
    wchar_t* wcstok_context = NULL;
    wchar_t* token;
    
    if (str != NULL) {
        token = str;
    } else {
        token = wcstok_context;
    }
    
    if (token == NULL) {
        return NULL;
    }
    
    while (*token) {
        const wchar_t* d = delim;
        BOOL isDelim = FALSE;
        while (*d) {
            if (*token == *d) {
                isDelim = TRUE;
                break;
            }
            d++;
        }
        if (!isDelim) {
            break;
        }
        token++;
    }
    
    if (*token == L'\0') {
        wcstok_context = NULL;
        return NULL;
    }
    
    wchar_t* start = token;
    
    while (*token) {
        const wchar_t* d = delim;
        while (*d) {
            if (*token == *d) {
                *token = L'\0';
                wcstok_context = token + 1;
                return start;
            }
            d++;
        }
        token++;
    }
    
    wcstok_context = NULL;
    return start;
}

//=============================================================================
// STRING CONVERSION FUNCTIONS
//=============================================================================

wchar_t* __cdecl crt_atowc(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    
    SIZE_T len = crt_strlen(str) + 1;
    wchar_t* result = (wchar_t*)crt_malloc(len * sizeof(wchar_t));
    
    if (result == NULL) {
        return NULL;
    }
    
    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef int
        (WINAPI*
        _MultiByteToWideChar)(
            _In_ UINT CodePage,
            _In_ DWORD dwFlags,
            _In_NLS_string_(cbMultiByte) LPCCH lpMultiByteStr,
            _In_ int cbMultiByte,
            _Out_writes_to_opt_(cchWideChar, return) LPWSTR lpWideCharStr,
            _In_ int cchWideChar
        );
    constexpr unsigned int hashMultiByteToWideChar = ComplexHashForAnsi("MultiByteToWideChar");
    _MultiByteToWideChar pMultiByteToWideChar = (_MultiByteToWideChar)apiResolve.GetApiAddress(lpKernel32, hashMultiByteToWideChar);
    pMultiByteToWideChar(CP_ACP, 0, str, -1, result, (int)len);
    return result;
}

char* __cdecl crt_wctoa(const wchar_t* str) {
    if (str == NULL) {
        return NULL;
    }
    
    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef int
    (WINAPI*
        _WideCharToMultiByte)(
            _In_ UINT CodePage,
            _In_ DWORD dwFlags,
            _In_NLS_string_(cchWideChar) LPCWCH lpWideCharStr,
            _In_ int cchWideChar,
            _Out_writes_bytes_to_opt_(cbMultiByte, return) LPSTR lpMultiByteStr,
            _In_ int cbMultiByte,
            _In_opt_ LPCCH lpDefaultChar,
            _Out_opt_ LPBOOL lpUsedDefaultChar
            );
    constexpr unsigned int hashWideCharToMultiByte = ComplexHashForAnsi("WideCharToMultiByte");
    _WideCharToMultiByte pWideCharToMultiByte = (_WideCharToMultiByte)apiResolve.GetApiAddress(lpKernel32, hashWideCharToMultiByte);
    int len = pWideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    if (len == 0) {
        return NULL;
    }
    
    char* result = (char*)crt_malloc(len);
    if (result == NULL) {
        return NULL;
    }
  
    pWideCharToMultiByte(CP_ACP, 0, str, -1, result, len, NULL, NULL);
    return result;
}

static int parse_sign_and_digits(const char* str, const char** endPtr, BOOL* overflow) {
    int sign = 1;
    unsigned int result = 0;
    *overflow = FALSE;
    
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') {
        str++;
    }
    
    const char* p = str;
    while (*p >= '0' && *p <= '9') {
        unsigned int digit = *p - '0';
        if (result > (UINT_MAX - digit) / 10) {
            *overflow = TRUE;
        }
        result = result * 10 + digit;
        p++;
    }
    
    if (endPtr) {
        *endPtr = p;
    }
    
    return sign * (int)result;
}

int __cdecl crt_atoi(const char* str) {
    if (str == NULL) {
        return 0;
    }
    BOOL overflow;
    return parse_sign_and_digits(str, NULL, &overflow);
}

long __cdecl crt_atol(const char* str) {
    if (str == NULL) {
        return 0;
    }
    BOOL overflow;
    return (long)parse_sign_and_digits(str, NULL, &overflow);
}

long long __cdecl crt_atoll(const char* str) {
    if (str == NULL) {
        return 0;
    }
    BOOL overflow;
    return (long long)parse_sign_and_digits(str, NULL, &overflow);
}

unsigned int __cdecl crt_atoui(const char* str) {
    if (str == NULL) {
        return 0;
    }
    const char* end;
    parse_sign_and_digits(str, &end, NULL);
    return (unsigned int)crt_atol(str);
}

unsigned long __cdecl crt_atoul(const char* str) {
    if (str == NULL) {
        return 0;
    }
    return (unsigned long)crt_atoll(str);
}

unsigned long long __cdecl crt_atoull(const char* str) {
    if (str == NULL) {
        return 0;
    }
    return (unsigned long long)crt_atoll(str);
}

int __cdecl crt_wtoi(const wchar_t* str) {
    char* ansiStr = crt_wctoa(str);
    if (ansiStr == NULL) {
        return 0;
    }
    int result = crt_atoi(ansiStr);
    crt_free(ansiStr);
    return result;
}

long __cdecl crt_wtol(const wchar_t* str) {
    char* ansiStr = crt_wctoa(str);
    if (ansiStr == NULL) {
        return 0;
    }
    long result = crt_atol(ansiStr);
    crt_free(ansiStr);
    return result;
}

long long __cdecl crt_wtoll(const wchar_t* str) {
    char* ansiStr = crt_wctoa(str);
    if (ansiStr == NULL) {
        return 0;
    }
    long long result = crt_atoll(ansiStr);
    crt_free(ansiStr);
    return result;
}

unsigned int __cdecl crt_wtoui(const wchar_t* str) {
    return (unsigned int)crt_wtoi(str);
}

unsigned long __cdecl crt_wtoul(const wchar_t* str) {
    return (unsigned long)crt_wtoll(str);
}

unsigned long long __cdecl crt_wtoull(const wchar_t* str) {
    return (unsigned long long)crt_wtoll(str);
}

char* __cdecl crt_itoa(int value, char* str, int radix) {
    if (str == NULL || radix < 2 || radix > 36) {
        return NULL;
    }
    
    unsigned int uvalue;
    int negative = FALSE;
    
    if (value < 0 && radix == 10) {
        negative = TRUE;
        uvalue = (unsigned int)(-(int)(value + 1)) + 1;
    } else {
        uvalue = (unsigned int)value;
    }
    
    char buffer[34];
    int i = 0;
    
    if (uvalue == 0) {
        buffer[i++] = '0';
    } else {
        while (uvalue > 0) {
            int digit = uvalue % radix;
            if (digit < 10) {
                buffer[i++] = '0' + digit;
            } else {
                buffer[i++] = 'a' + (digit - 10);
            }
            uvalue /= radix;
        }
    }
    
    if (negative) {
        buffer[i++] = '-';
    }
    
    int j = 0;
    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = '\0';
    
    return str;
}

char* __cdecl crt_ltoa(long value, char* str, int radix) {
    return crt_itoa((int)value, str, radix);
}

char* __cdecl crt_lltoa(long long value, char* str, int radix) {
    if (str == NULL || radix < 2 || radix > 36) {
        return NULL;
    }
    
    unsigned long long uvalue;
    int negative = FALSE;
    
    if (value < 0 && radix == 10) {
        negative = TRUE;
        uvalue = (unsigned long long)(-(long long)(value + 1)) + 1;
    } else {
        uvalue = (unsigned long long)value;
    }
    
    char buffer[66];
    int i = 0;
    
    if (uvalue == 0) {
        buffer[i++] = '0';
    } else {
        while (uvalue > 0) {
            int digit = (int)(uvalue % (unsigned long long)radix);
            if (digit < 10) {
                buffer[i++] = '0' + digit;
            } else {
                buffer[i++] = 'a' + (digit - 10);
            }
            uvalue /= (unsigned long long)radix;
        }
    }
    
    if (negative) {
        buffer[i++] = '-';
    }
    
    int j = 0;
    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = '\0';
    
    return str;
}

char* __cdecl crt_uitoa(unsigned int value, char* str, int radix) {
    return crt_itoa((int)value, str, radix);
}

char* __cdecl crt_ultoa(unsigned long value, char* str, int radix) {
    return crt_lltoa((long long)value, str, radix);
}

char* __cdecl crt_ulltoa(unsigned long long value, char* str, int radix) {
    return crt_lltoa((long long)value, str, radix);
}

wchar_t* __cdecl crt_itow(int value, wchar_t* str, int radix) {
    if (str == NULL || radix < 2 || radix > 36) {
        return NULL;
    }
    
    unsigned int uvalue;
    int negative = FALSE;
    
    if (value < 0 && radix == 10) {
        negative = TRUE;
        uvalue = (unsigned int)(-(int)(value + 1)) + 1;
    } else {
        uvalue = (unsigned int)value;
    }
    
    wchar_t buffer[34];
    int i = 0;
    
    if (uvalue == 0) {
        buffer[i++] = L'0';
    } else {
        while (uvalue > 0) {
            int digit = uvalue % radix;
            if (digit < 10) {
                buffer[i++] = L'0' + digit;
            } else {
                buffer[i++] = L'a' + (digit - 10);
            }
            uvalue /= radix;
        }
    }
    
    if (negative) {
        buffer[i++] = L'-';
    }
    
    int j = 0;
    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = L'\0';
    
    return str;
}

wchar_t* __cdecl crt_ltow(long value, wchar_t* str, int radix) {
    return crt_itow((int)value, str, radix);
}

wchar_t* __cdecl crt_lltow(long long value, wchar_t* str, int radix) {
    if (str == NULL || radix < 2 || radix > 36) {
        return NULL;
    }
    
    unsigned long long uvalue;
    int negative = FALSE;
    
    if (value < 0 && radix == 10) {
        negative = TRUE;
        uvalue = (unsigned long long)(-(long long)(value + 1)) + 1;
    } else {
        uvalue = (unsigned long long)value;
    }
    
    wchar_t buffer[66];
    int i = 0;
    
    if (uvalue == 0) {
        buffer[i++] = L'0';
    } else {
        while (uvalue > 0) {
            int digit = (int)(uvalue % (unsigned long long)radix);
            if (digit < 10) {
                buffer[i++] = L'0' + digit;
            } else {
                buffer[i++] = L'a' + (digit - 10);
            }
            uvalue /= (unsigned long long)radix;
        }
    }
    
    if (negative) {
        buffer[i++] = L'-';
    }
    
    int j = 0;
    while (i > 0) {
        str[j++] = buffer[--i];
    }
    str[j] = L'\0';
    
    return str;
}

wchar_t* __cdecl crt_uwtow(unsigned int value, wchar_t* str, int radix) {
    return crt_itow((int)value, str, radix);
}

wchar_t* __cdecl crt_ultow(unsigned long value, wchar_t* str, int radix) {
    return crt_lltow((long long)value, str, radix);
}

wchar_t* __cdecl crt_ulltow(unsigned long long value, wchar_t* str, int radix) {
    return crt_lltow((long long)value, str, radix);
}

//=============================================================================
// WIDE-CHARACTER CLASSIFICATION FUNCTIONS
//=============================================================================

int __cdecl crt_iswalnum(wint_t ch) {
    return crt_iswalpha(ch) || crt_iswdigit(ch);
}

int __cdecl crt_iswalpha(wint_t ch) {
    if (ch == WEOF) return 0;
    wchar_t wch = (wchar_t)ch;
    if ((wch >= L'a' && wch <= L'z') || (wch >= L'A' && wch <= L'Z')) {
        return 1;
    }
    return 0;
}

int __cdecl crt_iswdigit(wint_t ch) {
    if (ch == WEOF) return 0;
    wchar_t wch = (wchar_t)ch;
    if (wch >= L'0' && wch <= L'9') {
        return 1;
    }
    return 0;
}

int __cdecl crt_iswspace(wint_t ch) {
    if (ch == WEOF) return 0;
    wchar_t wch = (wchar_t)ch;
    if (wch == L' ' || wch == L'\t' || wch == L'\n' || wch == L'\r' || wch == L'\f' || wch == L'\v') {
        return 1;
    }
    return 0;
}

int __cdecl crt_iswupper(wint_t ch) {
    if (ch == WEOF) return 0;
    wchar_t wch = (wchar_t)ch;
    if (wch >= L'A' && wch <= L'Z') {
        return 1;
    }
    return 0;
}

int __cdecl crt_iswlower(wint_t ch) {
    if (ch == WEOF) return 0;
    wchar_t wch = (wchar_t)ch;
    if (wch >= L'a' && wch <= L'z') {
        return 1;
    }
    return 0;
}

int __cdecl crt_iswxdigit(wint_t ch) {
    if (ch == WEOF) return 0;
    wchar_t wch = (wchar_t)ch;
    if ((wch >= L'0' && wch <= L'9') ||
        (wch >= L'a' && wch <= L'f') ||
        (wch >= L'A' && wch <= L'F')) {
        return 1;
    }
    return 0;
}

wint_t __cdecl crt_towupper(wint_t ch) {
    if (ch == WEOF) return WEOF;
    wchar_t wch = (wchar_t)ch;
    if (wch >= L'a' && wch <= L'z') {
        return (wint_t)(wch - 32);
    }
    return ch;
}

wint_t __cdecl crt_towlower(wint_t ch) {
    if (ch == WEOF) return WEOF;
    wchar_t wch = (wchar_t)ch;
    if (wch >= L'A' && wch <= L'Z') {
        return (wint_t)(wch + 32);
    }
    return ch;
}

//=============================================================================
// WIDE-CHARACTER TO NUMBER CONVERSION (wcstox family)
//=============================================================================

static const wchar_t* crt_wcskip_isspace(const wchar_t* str) {
    while (crt_iswspace(*str)) {
        str++;
    }
    return str;
}

static unsigned long crt_wcstoul_base(const wchar_t* nptr, wchar_t** endptr, int base) {
    if (nptr == NULL) {
        if (endptr) *endptr = (wchar_t*)nptr;
        return 0;
    }

    const wchar_t* p = crt_wcskip_isspace(nptr);

    int negative = 0;
    if (*p == L'-') {
        negative = 1;
        p++;
    } else if (*p == L'+') {
        p++;
    }

    if (base == 0) {
        base = 10;
        if (*p == L'0') {
            if (p[1] == L'x' || p[1] == L'X') {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        }
    } else if (base == 16) {
        if (*p == L'0' && (p[1] == L'x' || p[1] == L'X')) {
            p += 2;
        }
    }

    unsigned long result = 0;
    int overflow = 0;

    while (*p) {
        wchar_t c = *p;
        int digit;

        if (c >= L'0' && c <= L'9') {
            digit = c - L'0';
        } else if (c >= L'a' && c <= L'z') {
            digit = c - L'a' + 10;
        } else if (c >= L'A' && c <= L'Z') {
            digit = c - L'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        if (result > (ULONG_MAX - digit) / base) {
            overflow = 1;
        }
        result = result * base + digit;
        p++;
    }

    if (endptr) {
        *endptr = (wchar_t*)p;
    }

    if (overflow) {
        return ULONG_MAX;
    }

    return negative ? (unsigned long)(-(long)result) : result;
}

unsigned long __cdecl crt_wcstoul(const wchar_t* nptr, wchar_t** endptr, int base) {
    return crt_wcstoul_base(nptr, endptr, base);
}

unsigned int __cdecl crt_wcstoui(const wchar_t* nptr, wchar_t** endptr, int base) {
    return (unsigned int)crt_wcstoul_base(nptr, endptr, base);
}

long __cdecl crt_wcstol(const wchar_t* nptr, wchar_t** endptr, int base) {
    if (nptr == NULL) {
        if (endptr) *endptr = (wchar_t*)nptr;
        return 0;
    }

    const wchar_t* p = crt_wcskip_isspace(nptr);

    int negative = 0;
    if (*p == L'-') {
        negative = 1;
        p++;
    } else if (*p == L'+') {
        p++;
    }

    if (base == 0) {
        base = 10;
        if (*p == L'0') {
            if (p[1] == L'x' || p[1] == L'X') {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        }
    } else if (base == 16) {
        if (*p == L'0' && (p[1] == L'x' || p[1] == L'X')) {
            p += 2;
        }
    }

    unsigned long result = 0;
    int overflow = 0;

    while (*p) {
        wchar_t c = *p;
        int digit;

        if (c >= L'0' && c <= L'9') {
            digit = c - L'0';
        } else if (c >= L'a' && c <= L'z') {
            digit = c - L'a' + 10;
        } else if (c >= L'A' && c <= L'Z') {
            digit = c - L'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        if (result > (ULONG_MAX - digit) / base) {
            overflow = 1;
        }
        result = result * base + digit;
        p++;
    }

    if (endptr) {
        *endptr = (wchar_t*)p;
    }

    if (overflow) {
        return (negative ? LONG_MIN : LONG_MAX);
    }

    return negative ? (long)(-(long)result) : (long)result;
}

long long __cdecl crt_wcstoll(const wchar_t* nptr, wchar_t** endptr, int base) {
    if (nptr == NULL) {
        if (endptr) *endptr = (wchar_t*)nptr;
        return 0;
    }

    const wchar_t* p = crt_wcskip_isspace(nptr);

    int negative = 0;
    if (*p == L'-') {
        negative = 1;
        p++;
    } else if (*p == L'+') {
        p++;
    }

    if (base == 0) {
        base = 10;
        if (*p == L'0') {
            if (p[1] == L'x' || p[1] == L'X') {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        }
    } else if (base == 16) {
        if (*p == L'0' && (p[1] == L'x' || p[1] == L'X')) {
            p += 2;
        }
    }

    unsigned long long result = 0;
    int overflow = 0;

    while (*p) {
        wchar_t c = *p;
        int digit;

        if (c >= L'0' && c <= L'9') {
            digit = c - L'0';
        } else if (c >= L'a' && c <= L'z') {
            digit = c - L'a' + 10;
        } else if (c >= L'A' && c <= L'Z') {
            digit = c - L'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        if (result > (ULLONG_MAX - digit) / base) {
            overflow = 1;
        }
        result = result * base + digit;
        p++;
    }

    if (endptr) {
        *endptr = (wchar_t*)p;
    }

    if (overflow) {
        return (negative ? LLONG_MIN : LLONG_MAX);
    }

    return negative ? (long long)(-(long long)result) : (long long)result;
}

unsigned long long __cdecl crt_wcstoull(const wchar_t* nptr, wchar_t** endptr, int base) {
    if (nptr == NULL) {
        if (endptr) *endptr = (wchar_t*)nptr;
        return 0;
    }

    const wchar_t* p = crt_wcskip_isspace(nptr);

    if (*p == L'+') {
        p++;
    }

    if (base == 0) {
        base = 10;
        if (*p == L'0') {
            if (p[1] == L'x' || p[1] == L'X') {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        }
    } else if (base == 16) {
        if (*p == L'0' && (p[1] == L'x' || p[1] == L'X')) {
            p += 2;
        }
    }

    unsigned long long result = 0;
    int overflow = 0;

    while (*p) {
        wchar_t c = *p;
        int digit;

        if (c >= L'0' && c <= L'9') {
            digit = c - L'0';
        } else if (c >= L'a' && c <= L'z') {
            digit = c - L'a' + 10;
        } else if (c >= L'A' && c <= L'Z') {
            digit = c - L'A' + 10;
        } else {
            break;
        }

        if (digit >= base) {
            break;
        }

        if (result > (ULLONG_MAX - digit) / base) {
            overflow = 1;
        }
        result = result * base + digit;
        p++;
    }

    if (endptr) {
        *endptr = (wchar_t*)p;
    }

    if (overflow) {
        return ULLONG_MAX;
    }

    return result;
}

unsigned long __cdecl crt_wcstoull_val(const wchar_t* nptr, wchar_t** endptr, int base) {
    return (unsigned long)crt_wcstoull(nptr, endptr, base);
}

//=============================================================================
// WIDE-CHARACTER FORMATTED OUTPUT (swprintf)
//=============================================================================

static const wchar_t* crt_wcformat_next(const wchar_t* fmt, int* width, int* precision, int* flags, wchar_t* spec) {
    *width = 0;
    *precision = -1;
    *flags = 0;
    *spec = L'\0';

    if (*fmt != L'%') return fmt;
    fmt++;

    while (*fmt)
    {
        if (*fmt == L'-')
        {
            *flags |= 1;
            fmt++;
        }
        else if (*fmt == L'+')
        {
            *flags |= 2;
            fmt++;
        }
        else if (*fmt == L' ')
        {
            *flags |= 4;
            fmt++;
        }
        else if (*fmt == L'0')
        {
            *flags |= 8;
            fmt++;
        }
        else if (*fmt == L'#')
        {
            *flags |= 16;
            fmt++;
        }
        else
        {
            goto width_parsing;
        }
    }

width_parsing:
    if (*fmt >= L'0' && *fmt <= L'9') {
        int w = 0;
        while (*fmt >= L'0' && *fmt <= L'9') {
            w = w * 10 + (*fmt - L'0');
            fmt++;
        }
        *width = w;
    }

    if (*fmt == L'.') {
        fmt++;
        int p = 0;
        int has_digit = 0;
        while (*fmt >= L'0' && *fmt <= L'9') {
            p = p * 10 + (*fmt - L'0');
            has_digit = 1;
            fmt++;
        }
        *precision = has_digit ? p : 0;
    }

    *spec = *fmt;
    return fmt + 1;
}

static int crt_wcputint(wchar_t* buf, unsigned long long val, int width, int prec, int flags, int negative, int upper) {
    wchar_t tmp[32];
    int i = 0;
    int base = 10;

    if (val == 0) {
        tmp[i++] = L'0';
    } else {
        while (val > 0) {
            int d = val % base;
            if (d < 10) {
                tmp[i++] = (wchar_t)(L'0' + d);
            } else {
                tmp[i++] = (wchar_t)((upper ? L'A' : L'a') + d - 10);
            }
            val /= base;
        }
    }

    if (prec > i) {
        prec = i;
    }

    int total_width = i;
    if (negative) total_width++;
    if ((flags & 8) && !negative && prec < width) {
        total_width = width;
    }

    int out_i = 0;

    if (!(flags & 1) && (flags & 8) && !negative) {
        while (total_width > i) {
            buf[out_i++] = L'0';
            total_width--;
        }
    }

    if (negative) {
        buf[out_i++] = L'-';
    } else if ((flags & 2) && !negative) {
        buf[out_i++] = L'+';
    } else if ((flags & 4) && !negative) {
        buf[out_i++] = L' ';
    }

    int pad = width - i;
    if ((flags & 8) && !negative && prec >= width) {
        pad = 0;
    }

    if (flags & 1) {
        while (i > 0) {
            buf[out_i++] = tmp[--i];
        }
        while (pad > 0) {
            buf[out_i++] = L' ';
            pad--;
        }
    } else {
        while (pad > 0 && !(flags & 8)) {
            buf[out_i++] = L' ';
            pad--;
        }
        while (i > 0) {
            buf[out_i++] = tmp[--i];
        }
    }

    return out_i;
}

static int crt_wcputunsigned(wchar_t* buf, unsigned long long val, int width, int prec, int flags, int upper, int base) {
    wchar_t tmp[32];
    int i = 0;

    if (val == 0) {
        tmp[i++] = L'0';
    } else {
        while (val > 0) {
            int d = val % base;
            if (d < 10) {
                tmp[i++] = (wchar_t)(L'0' + d);
            } else {
                tmp[i++] = (wchar_t)((upper ? L'A' : L'a') + d - 10);
            }
            val /= base;
        }
    }

    int out_i = 0;
    int pad = width - i;

    if (!(flags & 1) && (flags & 8)) {
        while (pad > 0) {
            buf[out_i++] = L'0';
            pad--;
        }
    }

    while (i > 0) {
        buf[out_i++] = tmp[--i];
    }

    if (flags & 1) {
        while (pad > 0) {
            buf[out_i++] = L' ';
            pad--;
        }
    }

    return out_i;
}

static int crt_wcputhex(wchar_t* buf, unsigned long long val, int width, int prec, int flags, int upper) {
    wchar_t tmp[32];
    int i = 0;

    if (val == 0) {
        tmp[i++] = L'0';
    } else {
        while (val > 0) {
            int d = val % 16;
            if (d < 10) {
                tmp[i++] = (wchar_t)(L'0' + d);
            } else {
                tmp[i++] = (wchar_t)((upper ? L'A' : L'a') + d - 10);
            }
            val /= 16;
        }
    }

    int out_i = 0;

    if ((flags & 16) && val == 0 && i > 0) {
        buf[out_i++] = upper ? L'X' : L'x';
        buf[out_i++] = L'0';
    }

    int pad = width - i;
    if (!(flags & 1) && (flags & 8)) {
        while (pad > 0) {
            buf[out_i++] = L'0';
            pad--;
        }
    }

    while (i > 0) {
        buf[out_i++] = tmp[--i];
    }

    if (flags & 1) {
        while (pad > 0) {
            buf[out_i++] = L' ';
            pad--;
        }
    }

    return out_i;
}

static int crt_wcputpointer(wchar_t* buf, void* ptr) {
    unsigned long long val = (unsigned long long)(SIZE_T)ptr;
    wchar_t tmp[32];
    int i = 0;

    if (val == 0) {
        tmp[i++] = L'0';
    } else {
        while (val > 0) {
            int d = val % 16;
            if (d < 10) {
                tmp[i++] = (wchar_t)(L'0' + d);
            } else {
                tmp[i++] = (wchar_t)(L'a' + d - 10);
            }
            val /= 16;
        }
    }

    int out_i = 0;
    buf[out_i++] = L'0';
    buf[out_i++] = L'x';

    while (i > 0) {
        buf[out_i++] = tmp[--i];
    }

    return out_i;
}

static int crt_wcputstring(wchar_t* buf, const wchar_t* str, int width, int prec) {
    int len = 0;
    if (str != NULL) {
        while (str[len] != L'\0') {
            len++;
        }
    }

    if (prec > 0 && prec < len) {
        len = prec;
    }

    int out_i = 0;
    int pad = width - len;

    if (!(pad < 0)) {
        while (pad > 0) {
            buf[out_i++] = L' ';
            pad--;
        }
    }

    if (str != NULL) {
        for (int i = 0; i < len; i++) {
            buf[out_i++] = str[i];
        }
    }

    return out_i;
}

int __cdecl crt_swprintf(wchar_t* buf, SIZE_T count, const wchar_t* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = crt_vswprintf(buf, count, fmt, args);
    va_end(args);
    return result;
}

int __cdecl crt_vswprintf(wchar_t* buf, SIZE_T count, const wchar_t* fmt, va_list args) {
    if (buf == NULL || fmt == NULL || count == 0) {
        return -1;
    }

    wchar_t temp[64];
    const wchar_t* p = fmt;
    wchar_t* out = buf;
    SIZE_T remaining = count;
    int total = 0;

    while (*p && remaining > 1) {
        if (*p != L'%') {
            *out++ = *p++;
            remaining--;
            total++;
            continue;
        }

        int width = 0, prec = -1, flags = 0;
        wchar_t spec = L'\0';
        const wchar_t* next = crt_wcformat_next(p, &width, &prec, &flags, &spec);
        int chars = 0;

        if (spec == L'd' || spec == L'i')
        {
            int v = va_arg(args, int);
            unsigned int uv;
            int neg = 0;

            if (v < 0)
            {
                uv = 0u - (unsigned int)v;
                neg = 1;
            }
            else
            {
                uv = (unsigned int)v;
            }

            chars = crt_wcputint(temp, uv, width, prec, flags, neg, 0);
        }
        else if (spec == L'u')
        {
            unsigned int v = va_arg(args, unsigned int);
            chars = crt_wcputunsigned(temp, v, width, prec, flags, 0, 10);
        }
        else if (spec == L'o')
        {
            unsigned int v = va_arg(args, unsigned int);
            chars = crt_wcputunsigned(temp, v, width, prec, flags, 0, 8);
        }
        else if (spec == L'x' || spec == L'X')
        {
            unsigned int v = va_arg(args, unsigned int);
            chars = crt_wcputhex(temp, v, width, prec, flags,
                (spec == L'X') ? 1 : 0);
        }
        else if (spec == L'c')
        {
            wchar_t c = (wchar_t)va_arg(args, int);
            temp[0] = c;
            chars = 1;
        }
        else if (spec == L's')
        {
            wchar_t* s = va_arg(args, wchar_t*);
            chars = crt_wcputstring(temp, s, width, prec);
        }
        else if (spec == L'p')
        {
            void* ptr = va_arg(args, void*);
            chars = crt_wcputpointer(temp, ptr);
        }
        else if (spec == L'%')
        {
            temp[0] = L'%';
            chars = 1;
        }
        else if (spec == L'n')
        {
            int* np = va_arg(args, int*);
            if (np)
                *np = total;

            chars = 0;
        }
        else if (spec == L'l')
        {
            if (p[1] == L'd' || p[1] == L'i')
            {
                long v = va_arg(args, long);
                unsigned long uv;
                int neg = 0;

            if (v < 0)
            {
                uv = 0ul - (unsigned long)v;
                neg = 1;
            }
            else
            {
                uv = (unsigned long)v;
            }

            chars = crt_wcputint(temp, uv, width, prec, flags, neg, 0);
        }
        else if (p[1] == L'u')
        {
            unsigned long v = va_arg(args, unsigned long);
            chars = crt_wcputunsigned(temp, v, width, prec, flags, 0, 10);
        }
        else if (p[1] == L'l' && (p[2] == L'd' || p[2] == L'i'))
        {
            long long v = va_arg(args, long long);
            unsigned long long uv;
            int neg = 0;

            if (v < 0)
            {
                uv = 0ull - (unsigned long long)v;
                neg = 1;
            }
            else
            {
                uv = (unsigned long long)v;
            }

            chars = crt_wcputint(temp, uv, width, prec, flags, neg, 0);
            p += 2;
            }
            else if (p[1] == L'l' && p[2] == L'u')
            {
                unsigned long long v =
                    va_arg(args, unsigned long long);

                chars = crt_wcputunsigned(
                    temp,
                    v,
                    width,
                    prec,
                    flags,
                    0,
                    10);

                p += 2;
            }
            else
            {
                temp[0] = L'l';
                chars = 1;
            }
        }
        else if (spec == L'z')
        {
            if (p[1] == L'u')
            {
                SIZE_T v = va_arg(args, SIZE_T);

                chars = crt_wcputunsigned(
                    temp,
                    (unsigned long long)v,
                    width,
                    prec,
                    flags,
                    0,
                    10);
            }
            else if (p[1] == L'd' || p[1] == L'i')
            {
                SIZE_T v = va_arg(args, SIZE_T);
                unsigned long long uv = (unsigned long long)v;
                int neg = 0;

                if ((long)v < 0)
                {
                    uv = 0ull - (unsigned long long)v;
                    neg = 1;
                }

                chars = crt_wcputint(
                    temp,
                    uv,
                    width,
                    prec,
                    flags,
                    neg,
                    0);
            }
            else
            {
                temp[0] = L'z';
                chars = 1;
            }
        }
        else
        {
            temp[0] = *p;
            chars = 1;
        }

        for (int i = 0; i < chars && remaining > 1; i++) {
            *out++ = temp[i];
            remaining--;
            total++;
        }

        p = next;
    }

    *out = L'\0';
    return total;
}

int __cdecl crt_swprintf_s(wchar_t* buf, SIZE_T bufSize, const wchar_t* fmt, ...) {
    if (buf == NULL || bufSize == 0 || fmt == NULL) {
        return -1;
    }

    va_list args;
    va_start(args, fmt);
    int result = crt_vswprintf(buf, bufSize, fmt, args);
    va_end(args);
    return result;
}

int __cdecl crt_vswprintf_s(wchar_t* buf, SIZE_T bufSize, const wchar_t* fmt, va_list args) {
    if (buf == NULL || bufSize == 0 || fmt == NULL) {
        return -1;
    }

    wchar_t temp[64];
    const wchar_t* p = fmt;
    wchar_t* out = buf;
    SIZE_T remaining = bufSize;
    int total = 0;

    while (*p && remaining > 1) {
        if (*p != L'%') {
            *out++ = *p++;
            remaining--;
            total++;
            continue;
        }

        int width = 0, prec = -1, flags = 0;
        wchar_t spec = L'\0';
        const wchar_t* next = crt_wcformat_next(p, &width, &prec, &flags, &spec);
        int chars = 0;

        if (spec == L's') {
            wchar_t* s = va_arg(args, wchar_t*);
            if (s == NULL) {
                wchar_t wsNull[] = {L'n', L'u', L'l', L'l', L'\0'};
                crt_wcsncpy(out, wsNull, remaining);
                SIZE_T null_len = crt_wcslen(wsNull);
                if (null_len >= remaining) {
                    buf[bufSize - 1] = L'\0';
                    int fmt_remaining = 0;
                    const wchar_t* r = next;
                    while (r && *r) { fmt_remaining++; r++; }
                    return total + fmt_remaining;
                }
                out += null_len;
                remaining -= null_len;
                total += null_len;
                p = next;
                continue;
            }
            SIZE_T s_len = crt_wcslen(s);
            if (prec >= 0 && (SIZE_T)prec < s_len) {
                s_len = prec;
            }
            if (s_len >= remaining) {
                crt_wcsncpy(out, s, remaining - 1);
                buf[bufSize - 1] = L'\0';
                int fmt_remaining = 0;
                const wchar_t* r = next;
                while (r && *r) { fmt_remaining++; r++; }
                return total + fmt_remaining;
            }
            crt_wcsncpy(out, s, s_len);
            out += s_len;
            remaining -= s_len;
            total += s_len;
            p = next;
            continue;
        }

        if (spec == L'd' || spec == L'i')
        {
            int v = va_arg(args, int);
            unsigned int uv;
            int neg = 0;

            if (v < 0)
            {
                uv = 0u - (unsigned int)v;
                neg = 1;
            }
            else
            {
                uv = (unsigned int)v;
            }

            chars = crt_wcputint(temp, uv, width, prec, flags, neg, 0);
        }
        else if (spec == L'u' || spec == L'o' || spec == L'x' || spec == L'X')
        {
            unsigned int v = va_arg(args, unsigned int);

            if (spec == L'o')
            {
                chars = crt_wcputunsigned(temp, v, width, prec, flags, 0, 8);
            }
            else
            {
                chars = crt_wcputhex(temp, v, width, prec, flags,
                    (spec == L'X') ? 1 : 0);
            }
        }
        else if (spec == L'c')
        {
            temp[0] = (wchar_t)va_arg(args, int);
            chars = 1;
        }
        else if (spec == L'p')
        {
            void* ptr = va_arg(args, void*);
            chars = crt_wcputpointer(temp, ptr);
        }
        else if (spec == L'%')
        {
            temp[0] = L'%';
            chars = 1;
        }
        else if (spec == L'n')
        {
            int* np = va_arg(args, int*);
            if (np)
                *np = total;

            chars = 0;
        }
        else if (spec == L'l')
        {
            if (p[1] == L'd' || p[1] == L'i')
            {
                long v = va_arg(args, long);
                unsigned long uv;
                int neg = 0;

                if (v < 0)
                {
                    uv = 0ul - (unsigned long)v;
                    neg = 1;
                }
                else
                {
                    uv = (unsigned long)v;
                }

                chars = crt_wcputint(temp, uv, width, prec, flags, neg, 0);
            }
            else if (p[1] == L'u')
            {
                unsigned long v = va_arg(args, unsigned long);
                chars = crt_wcputunsigned(temp, v, width, prec, flags, 0, 10);
            }
            else
            {
                temp[0] = L'l';
                chars = 1;
            }
        }
        else if (spec == L'z')
        {
            if (p[1] == L'u')
            {
                SIZE_T v = va_arg(args, SIZE_T);

                chars = crt_wcputunsigned(
                    temp,
                    (unsigned long long)v,
                    width,
                    prec,
                    flags,
                    0,
                    10);
            }
            else
            {
                temp[0] = L'z';
                chars = 1;
            }
        }
        else
        {
            temp[0] = *p;
            chars = 1;
        }

        if (chars >= (int)remaining) {
            crt_wcsncpy(out, temp, remaining - 1);
            buf[bufSize - 1] = L'\0';
            int fmt_remaining = 0;
            const wchar_t* r = next;
            while (r && *r) { fmt_remaining++; r++; }
            return total + fmt_remaining;
        }

        for (int i = 0; i < chars; i++) {
            *out++ = temp[i];
            remaining--;
            total++;
        }

        p = next;
    }

    *out = L'\0';
    return total;
}

//=============================================================================
// UTILITY FUNCTIONS
//=============================================================================

void __cdecl crt_srand(unsigned int seed) {
    unsigned int rand_seed = 1;
    rand_seed = seed;
}

int __cdecl crt_rand(void) {
    unsigned int rand_seed = 1;
    rand_seed = rand_seed * 1103515245 + 12345;
    return (int)((rand_seed >> 16) & 0x7FFF);
}

int __cdecl crt_abs(int value) {
    if (value < 0) {
        return -value;
    }
    return value;
}

long __cdecl crt_labs(long value) {
    if (value < 0) {
        return -value;
    }
    return value;
}

long long __cdecl crt_llabs(long long value) {
    if (value < 0) {
        return -value;
    }
    return value;
}

//=============================================================================
// FILE I/O FUNCTIONS (WinAPI-based)
//=============================================================================

#define CRT_FILE_FLAG_READ   0x01
#define CRT_FILE_FLAG_WRITE  0x02
#define CRT_FILE_FLAG_APPEND 0x04
#define CRT_FILE_FLAG_BINARY 0x08
#define CRT_FILE_FLAG_EOF    0x10
#define CRT_FILE_FLAG_ERROR  0x20

typedef struct crt_FILE {
    HANDLE handle;
    unsigned char flags;
    wchar_t lastChar;
} crt_FILE;

static DWORD crt_access_mode_to_winapi_a(const char* mode) {
    if (mode[0] == 'r' || mode[0] == 'R') {
        if (mode[1] == '+') return GENERIC_READ | GENERIC_WRITE;
        return GENERIC_READ;
    }
    if (mode[0] == 'w' || mode[0] == 'W') {
        if (mode[1] == '+') return GENERIC_READ | GENERIC_WRITE;
        return GENERIC_WRITE;
    }
    if (mode[0] == 'a' || mode[0] == 'A') {
        if (mode[1] == '+') return GENERIC_READ | GENERIC_WRITE;
        return GENERIC_WRITE;
    }
    return GENERIC_READ | GENERIC_WRITE;
}

static DWORD crt_access_mode_to_winapi_w(const wchar_t* mode) {
    if (mode[0] == L'r' || mode[0] == L'R') {
        if (mode[1] == L'+') return GENERIC_READ | GENERIC_WRITE;
        return GENERIC_READ;
    }
    if (mode[0] == L'w' || mode[0] == L'W') {
        if (mode[1] == L'+') return GENERIC_READ | GENERIC_WRITE;
        return GENERIC_WRITE;
    }
    if (mode[0] == L'a' || mode[0] == L'A') {
        if (mode[1] == L'+') return GENERIC_READ | GENERIC_WRITE;
        return GENERIC_WRITE;
    }
    return GENERIC_READ | GENERIC_WRITE;
}

static DWORD crt_creation_mode_a(const char* mode) {
    if (mode[0] == 'r' || mode[0] == 'R') {
        return OPEN_EXISTING;
    }
    if (mode[0] == 'w' || mode[0] == 'W') {
        return CREATE_ALWAYS;
    }
    if (mode[0] == 'a' || mode[0] == 'A') {
        return OPEN_ALWAYS;
    }
    return OPEN_EXISTING;
}

static DWORD crt_creation_mode_w(const wchar_t* mode) {
    if (mode[0] == L'r' || mode[0] == L'R') {
        return OPEN_EXISTING;
    }
    if (mode[0] == L'w' || mode[0] == L'W') {
        return CREATE_ALWAYS;
    }
    if (mode[0] == L'a' || mode[0] == L'A') {
        return OPEN_ALWAYS;
    }
    return OPEN_EXISTING;
}

static DWORD crt_file_share_mode(void) {
    return FILE_SHARE_READ | FILE_SHARE_WRITE;
}

static HANDLE crt_create_file_common(HANDLE hFile, crt_FILE* f, const wchar_t* mode) {
    if ((mode[0] == L'a' || mode[0] == L'A') &&
        (mode[1] != L'+')) {
        ApiResolve apiResolve2;
        constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
        LPVOID lpKernel322 = apiResolve2.GetModuleBaseAddress(hashKernel32);
        typedef DWORD (WINAPI* _SetFilePointer)(
            _In_ HANDLE hFile,
            _In_ LONG lDistanceToMove,
            _In_opt_ PLONG lpDistanceToMoveHigh,
            _In_ DWORD dwMoveMethod
        );
        constexpr unsigned int hashSetFilePointer = ComplexHashForAnsi("SetFilePointer");
        _SetFilePointer pSetFilePointer = (_SetFilePointer)apiResolve2.GetApiAddress(lpKernel322, hashSetFilePointer);
        pSetFilePointer(hFile, 0, NULL, FILE_END);
    }

    f->handle = hFile;
    f->flags = 0;
    f->lastChar = 0;

    if (mode[0] == L'r' || mode[0] == L'R') {
        f->flags = (unsigned char)(f->flags | CRT_FILE_FLAG_READ);
    }
    if (mode[0] == L'w' || mode[0] == L'W') {
        f->flags = (unsigned char)(f->flags | CRT_FILE_FLAG_WRITE);
    }
    if (mode[0] == L'a' || mode[0] == L'A') {
        f->flags = (unsigned char)(f->flags | CRT_FILE_FLAG_WRITE | CRT_FILE_FLAG_APPEND);
    }
    if (crt_wcschr(mode, L'+') != NULL) {
        f->flags = (unsigned char)(f->flags | CRT_FILE_FLAG_READ | CRT_FILE_FLAG_WRITE);
    }
    return hFile;
}

crt_FILE* __cdecl crt_fopen(const char* filename, const char* mode) {
    if (filename == NULL || mode == NULL) {
        return NULL;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);

    typedef HANDLE (WINAPI* _CreateFileW)(
        _In_ LPCWSTR lpFileName,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition,
        _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile
    );
    constexpr unsigned int hashCreateFileW = ComplexHashForAnsi("CreateFileW");
    _CreateFileW pCreateFileW = (_CreateFileW)apiResolve.GetApiAddress(lpKernel32, hashCreateFileW);
    typedef BOOL (WINAPI* _CloseHandle)(HANDLE hObject);
    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);

    wchar_t* wFilename = crt_atowc(filename);
    wchar_t* wMode = crt_atowc(mode);

    if (wFilename == NULL || wMode == NULL) {
        if (wFilename) crt_free(wFilename);
        if (wMode) crt_free(wMode);
        return NULL;
    }

    DWORD dwAccess = crt_access_mode_to_winapi_w(wMode);
    DWORD dwCreation = crt_creation_mode_w(wMode);

    HANDLE hFile = pCreateFileW(
        wFilename,
        dwAccess,
        crt_file_share_mode(),
        NULL,
        dwCreation,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        crt_free(wFilename);
        crt_free(wMode);
        return NULL;
    }

    crt_FILE* f = (crt_FILE*)crt_malloc(sizeof(crt_FILE));
    if (f == NULL) {
        pCloseHandle(hFile);
        crt_free(wFilename);
        crt_free(wMode);
        return NULL;
    }

    crt_create_file_common(hFile, f, wMode);
    crt_free(wFilename);
    crt_free(wMode);
    return f;
}

crt_FILE* __cdecl crt_wfopen(const wchar_t* filename, const wchar_t* mode) {
    if (filename == NULL || mode == NULL) {
        return NULL;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);

    typedef HANDLE (WINAPI* _CreateFileW)(
        _In_ LPCWSTR lpFileName,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition,
        _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile
    );
    constexpr unsigned int hashCreateFileW = ComplexHashForAnsi("CreateFileW");
    _CreateFileW pCreateFileW = (_CreateFileW)apiResolve.GetApiAddress(lpKernel32, hashCreateFileW);
    typedef BOOL (WINAPI* _CloseHandle)(HANDLE hObject);
    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);

    DWORD dwAccess = crt_access_mode_to_winapi_w(mode);
    DWORD dwCreation = crt_creation_mode_w(mode);

    HANDLE hFile = pCreateFileW(
        filename,
        dwAccess,
        crt_file_share_mode(),
        NULL,
        dwCreation,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return NULL;
    }

    crt_FILE* f = (crt_FILE*)crt_malloc(sizeof(crt_FILE));
    if (f == NULL) {
        pCloseHandle(hFile);
        return NULL;
    }

    crt_create_file_common(hFile, f, mode);
    return f;
}

int __cdecl crt_fclose(crt_FILE* stream) {
    if (stream == NULL) {
        return CRT_EOF;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef BOOL (WINAPI* _CloseHandle)(HANDLE hObject);
    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);

    BOOL result = pCloseHandle(stream->handle);
    crt_free(stream);
    return result ? 0 : CRT_EOF;
}

SIZE_T __cdecl crt_fread(void* buffer, SIZE_T size, SIZE_T count, crt_FILE* stream) {
    if (buffer == NULL || stream == NULL || size == 0) {
        return 0;
    }

    SIZE_T totalBytes = size * count;
    if (totalBytes == 0) {
        return 0;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef BOOL (WINAPI* _ReadFile)(
        _In_ HANDLE hFile,
        _Out_writes_bytes_to_(nNumberOfBytesToRead, *lpNumberOfBytesRead) LPVOID lpBuffer,
        _In_ DWORD nNumberOfBytesToRead,
        _Out_opt_ LPDWORD lpNumberOfBytesRead,
        _Inout_opt_ LPOVERLAPPED lpOverlapped
    );
    constexpr unsigned int hashReadFile = ComplexHashForAnsi("ReadFile");
    _ReadFile pReadFile = (_ReadFile)apiResolve.GetApiAddress(lpKernel32, hashReadFile);

    DWORD bytesRead = 0;
    BOOL result = pReadFile(stream->handle, buffer, (DWORD)totalBytes, &bytesRead, NULL);

    if (!result) {
        stream->flags |= CRT_FILE_FLAG_ERROR;
        return 0;
    }

    if (bytesRead < totalBytes) {
        stream->flags |= CRT_FILE_FLAG_EOF;
    }

    return bytesRead / size;
}

SIZE_T __cdecl crt_fwrite(const void* buffer, SIZE_T size, SIZE_T count, crt_FILE* stream) {
    if (buffer == NULL || stream == NULL || size == 0) {
        return 0;
    }

    SIZE_T totalBytes = size * count;
    if (totalBytes == 0) {
        return 0;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef BOOL (WINAPI* _WriteFile)(
        _In_ HANDLE hFile,
        _In_reads_bytes_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
        _In_ DWORD nNumberOfBytesToWrite,
        _Out_opt_ LPDWORD lpNumberOfBytesWritten,
        _Inout_opt_ LPOVERLAPPED lpOverlapped
    );
    constexpr unsigned int hashWriteFile = ComplexHashForAnsi("WriteFile");
    _WriteFile pWriteFile = (_WriteFile)apiResolve.GetApiAddress(lpKernel32, hashWriteFile);

    DWORD bytesWritten = 0;
    BOOL result = pWriteFile(stream->handle, buffer, (DWORD)totalBytes, &bytesWritten, NULL);

    if (!result) {
        stream->flags |= CRT_FILE_FLAG_ERROR;
        return 0;
    }

    return bytesWritten / size;
}

int __cdecl crt_fseek(crt_FILE* stream, long offset, int origin) {
    if (stream == NULL) {
        return -1;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef DWORD (WINAPI* _SetFilePointer)(
        _In_ HANDLE hFile,
        _In_ LONG lDistanceToMove,
        _In_opt_ PLONG lpDistanceToMoveHigh,
        _In_ DWORD dwMoveMethod
    );
    constexpr unsigned int hashSetFilePointer = ComplexHashForAnsi("SetFilePointer");
    _SetFilePointer pSetFilePointer = (_SetFilePointer)apiResolve.GetApiAddress(lpKernel32, hashSetFilePointer);

    DWORD dwOrigin;
    switch (origin) {
        case SEEK_SET: dwOrigin = FILE_BEGIN;   break;
        case SEEK_CUR: dwOrigin = FILE_CURRENT; break;
        case SEEK_END: dwOrigin = FILE_END;     break;
        default: return -1;
    }

    DWORD result = pSetFilePointer(stream->handle, offset, NULL, dwOrigin);
    if (result == INVALID_SET_FILE_POINTER) {
        return -1;
    }

    stream->flags &= ~CRT_FILE_FLAG_EOF;
    return 0;
}

long __cdecl crt_ftell(crt_FILE* stream) {
    if (stream == NULL) {
        return -1;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef DWORD (WINAPI* _SetFilePointer)(
        _In_ HANDLE hFile,
        _In_ LONG lDistanceToMove,
        _In_opt_ PLONG lpDistanceToMoveHigh,
        _In_ DWORD dwMoveMethod
    );
    constexpr unsigned int hashSetFilePointer = ComplexHashForAnsi("SetFilePointer");
    _SetFilePointer pSetFilePointer = (_SetFilePointer)apiResolve.GetApiAddress(lpKernel32, hashSetFilePointer);

    DWORD pos = pSetFilePointer(stream->handle, 0, NULL, FILE_CURRENT);
    if (pos == INVALID_SET_FILE_POINTER) {
        return -1;
    }

    return (long)pos;
}

void __cdecl crt_rewind(crt_FILE* stream) {
    if (stream != NULL) {
        crt_fseek(stream, 0, SEEK_SET);
        stream->flags &= ~CRT_FILE_FLAG_ERROR;
    }
}

int __cdecl crt_fflush(crt_FILE* stream) {
    if (stream == NULL) {
        return 0;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef BOOL (WINAPI* _FlushFileBuffers)(_In_ HANDLE hFile);
    constexpr unsigned int hashFlushFileBuffers = ComplexHashForAnsi("FlushFileBuffers");
    _FlushFileBuffers pFlushFileBuffers = (_FlushFileBuffers)apiResolve.GetApiAddress(lpKernel32, hashFlushFileBuffers);

    BOOL result = pFlushFileBuffers(stream->handle);
    return result ? 0 : CRT_EOF;
}

int __cdecl crt_feof(crt_FILE* stream) {
    if (stream == NULL) {
        return 0;
    }
    return (stream->flags & CRT_FILE_FLAG_EOF) ? 1 : 0;
}

int __cdecl crt_ferror(crt_FILE* stream) {
    if (stream == NULL) {
        return 0;
    }
    return (stream->flags & CRT_FILE_FLAG_ERROR) ? 1 : 0;
}

void __cdecl crt_clearerr(crt_FILE* stream) {
    if (stream != NULL) {
        stream->flags &= ~(CRT_FILE_FLAG_EOF | CRT_FILE_FLAG_ERROR);
    }
}

int __cdecl crt_remove(const char* filename) {
    if (filename == NULL) {
        return -1;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef BOOL (WINAPI* _DeleteFileW)(_In_ LPCWSTR lpFileName);
    constexpr unsigned int hashDeleteFileW = ComplexHashForAnsi("DeleteFileW");
    _DeleteFileW pDeleteFileW = (_DeleteFileW)apiResolve.GetApiAddress(lpKernel32, hashDeleteFileW);

    wchar_t* wFilename = crt_atowc(filename);
    if (wFilename == NULL) {
        return -1;
    }

    BOOL result = pDeleteFileW(wFilename);
    crt_free(wFilename);
    return result ? 0 : -1;
}

int __cdecl crt_wremove(const wchar_t* filename) {
    if (filename == NULL) {
        return -1;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef BOOL (WINAPI* _DeleteFileW)(_In_ LPCWSTR lpFileName);
    constexpr unsigned int hashDeleteFileW = ComplexHashForAnsi("DeleteFileW");
    _DeleteFileW pDeleteFileW = (_DeleteFileW)apiResolve.GetApiAddress(lpKernel32, hashDeleteFileW);

    BOOL result = pDeleteFileW(filename);
    return result ? 0 : -1;
}

int __cdecl crt_rename(const char* oldname, const char* newname) {
    if (oldname == NULL || newname == NULL) {
        return -1;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef BOOL (WINAPI* _MoveFileExW)(
        _In_ LPCWSTR lpExistingFileName,
        _In_opt_ LPCWSTR lpNewFileName,
        _In_ DWORD dwFlags
    );
    constexpr unsigned int hashMoveFileExW = ComplexHashForAnsi("MoveFileExW");
    _MoveFileExW pMoveFileExW = (_MoveFileExW)apiResolve.GetApiAddress(lpKernel32, hashMoveFileExW);

    wchar_t* wOldname = crt_atowc(oldname);
    wchar_t* wNewname = crt_atowc(newname);

    if (wOldname == NULL || wNewname == NULL) {
        if (wOldname) crt_free(wOldname);
        if (wNewname) crt_free(wNewname);
        return -1;
    }

    BOOL result = pMoveFileExW(wOldname, wNewname, MOVEFILE_REPLACE_EXISTING);
    crt_free(wOldname);
    crt_free(wNewname);
    return result ? 0 : -1;
}

int __cdecl crt_wrename(const wchar_t* oldname, const wchar_t* newname) {
    if (oldname == NULL || newname == NULL) {
        return -1;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef BOOL (WINAPI* _MoveFileExW)(
        _In_ LPCWSTR lpExistingFileName,
        _In_opt_ LPCWSTR lpNewFileName,
        _In_ DWORD dwFlags
    );
    constexpr unsigned int hashMoveFileExW = ComplexHashForAnsi("MoveFileExW");
    _MoveFileExW pMoveFileExW = (_MoveFileExW)apiResolve.GetApiAddress(lpKernel32, hashMoveFileExW);

    BOOL result = pMoveFileExW(oldname, newname, MOVEFILE_REPLACE_EXISTING);
    return result ? 0 : -1;
}

BOOL __cdecl crt_fileexists(const char* filename) {
    if (filename == NULL) {
        return FALSE;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef HANDLE (WINAPI* _CreateFileW)(
        _In_ LPCWSTR lpFileName,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition,
        _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile
    );
    constexpr unsigned int hashCreateFileW = ComplexHashForAnsi("CreateFileW");
    _CreateFileW pCreateFileW = (_CreateFileW)apiResolve.GetApiAddress(lpKernel32, hashCreateFileW);
    typedef BOOL (WINAPI* _CloseHandle)(HANDLE hObject);
    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);

    wchar_t* wFilename = crt_atowc(filename);
    if (wFilename == NULL) {
        return FALSE;
    }

    HANDLE hFile = pCreateFileW(wFilename, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, 0, NULL);
    crt_free(wFilename);

    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    pCloseHandle(hFile);
    return TRUE;
}

BOOL __cdecl crt_wfileexists(const wchar_t* filename) {
    if (filename == NULL) {
        return FALSE;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef HANDLE (WINAPI* _CreateFileW)(
        _In_ LPCWSTR lpFileName,
        _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode,
        _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition,
        _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile
    );
    constexpr unsigned int hashCreateFileW = ComplexHashForAnsi("CreateFileW");
    _CreateFileW pCreateFileW = (_CreateFileW)apiResolve.GetApiAddress(lpKernel32, hashCreateFileW);
    typedef BOOL (WINAPI* _CloseHandle)(HANDLE hObject);
    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);

    HANDLE hFile = pCreateFileW(filename, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    pCloseHandle(hFile);
    return TRUE;
}

long long __cdecl crt_filesize(const char* filename) {
    if (filename == NULL) {
        return -1;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef HANDLE (WINAPI* _CreateFileW)(
        _In_ LPCWSTR lpFileName, _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode, _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition, _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile
    );
    constexpr unsigned int hashCreateFileW = ComplexHashForAnsi("CreateFileW");
    _CreateFileW pCreateFileW = (_CreateFileW)apiResolve.GetApiAddress(lpKernel32, hashCreateFileW);
    typedef BOOL (WINAPI* _CloseHandle)(HANDLE hObject);
    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);

    wchar_t* wFilename = crt_atowc(filename);
    if (wFilename == NULL) {
        return -1;
    }

    HANDLE hFile = pCreateFileW(wFilename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, 0, NULL);
    crt_free(wFilename);

    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    typedef DWORD (WINAPI* _GetFileSizeEx)(HANDLE hFile, PLARGE_INTEGER lpFileSize);
    constexpr unsigned int hashGetFileSizeEx = ComplexHashForAnsi("GetFileSizeEx");
    _GetFileSizeEx pGetFileSizeEx = (_GetFileSizeEx)apiResolve.GetApiAddress(lpKernel32, hashGetFileSizeEx);

    LARGE_INTEGER size;
    long long result = -1;
    if (pGetFileSizeEx(hFile, &size)) {
        result = size.QuadPart;
    }

    pCloseHandle(hFile);
    return result;
}

long long __cdecl crt_wfilesize(const wchar_t* filename) {
    if (filename == NULL) {
        return -1;
    }

    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef HANDLE (WINAPI* _CreateFileW)(
        _In_ LPCWSTR lpFileName, _In_ DWORD dwDesiredAccess,
        _In_ DWORD dwShareMode, _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        _In_ DWORD dwCreationDisposition, _In_ DWORD dwFlagsAndAttributes,
        _In_opt_ HANDLE hTemplateFile
    );
    constexpr unsigned int hashCreateFileW = ComplexHashForAnsi("CreateFileW");
    _CreateFileW pCreateFileW = (_CreateFileW)apiResolve.GetApiAddress(lpKernel32, hashCreateFileW);
    typedef BOOL (WINAPI* _CloseHandle)(HANDLE hObject);
    constexpr unsigned int hashCloseHandle = ComplexHashForAnsi("CloseHandle");
    _CloseHandle pCloseHandle = (_CloseHandle)apiResolve.GetApiAddress(lpKernel32, hashCloseHandle);

    HANDLE hFile = pCreateFileW(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        return -1;
    }

    typedef DWORD (WINAPI* _GetFileSizeEx)(HANDLE hFile, PLARGE_INTEGER lpFileSize);
    constexpr unsigned int hashGetFileSizeEx = ComplexHashForAnsi("GetFileSizeEx");
    _GetFileSizeEx pGetFileSizeEx = (_GetFileSizeEx)apiResolve.GetApiAddress(lpKernel32, hashGetFileSizeEx);

    LARGE_INTEGER size;
    long long result = -1;
    if (pGetFileSizeEx(hFile, &size)) {
        result = size.QuadPart;
    }

    pCloseHandle(hFile);
    return result;
}

//=============================================================================
// FORMATTED PRINT FUNCTIONS (char)
//=============================================================================

static int crt_cputchar(char c) {
    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef HANDLE (WINAPI* _GetStdHandle)(_In_ DWORD nStdHandle);
    constexpr unsigned int hashGetStdHandle = ComplexHashForAnsi("GetStdHandle");
    _GetStdHandle pGetStdHandle = (_GetStdHandle)apiResolve.GetApiAddress(lpKernel32, hashGetStdHandle);
    typedef BOOL (WINAPI* _WriteFile)(
        _In_ HANDLE hFile, _In_reads_bytes_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
        _In_ DWORD nNumberOfBytesToWrite, _Out_opt_ LPDWORD lpNumberOfBytesWritten,
        _Inout_opt_ LPOVERLAPPED lpOverlapped
    );
    constexpr unsigned int hashWriteFile = ComplexHashForAnsi("WriteFile");
    _WriteFile pWriteFile = (_WriteFile)apiResolve.GetApiAddress(lpKernel32, hashWriteFile);

    HANDLE hStdout = pGetStdHandle(STD_OUTPUT_HANDLE);
    char buf[2] = { c, '\0' };
    DWORD written = 0;
    pWriteFile(hStdout, buf, 1, &written, NULL);
    return (int)written;
}

static int crt_cputs(const char* str) {
    if (str == NULL) return -1;
    int count = 0;
    while (*str) {
        count += crt_cputchar(*str++);
    }
    return count;
}

static int crt_cpputs(const char* str, SIZE_T len) {
    if (str == NULL || len == 0) return 0;
    ApiResolve apiResolve;
    constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
    LPVOID lpKernel32 = apiResolve.GetModuleBaseAddress(hashKernel32);
    typedef HANDLE (WINAPI* _GetStdHandle)(_In_ DWORD nStdHandle);
    constexpr unsigned int hashGetStdHandle = ComplexHashForAnsi("GetStdHandle");
    _GetStdHandle pGetStdHandle = (_GetStdHandle)apiResolve.GetApiAddress(lpKernel32, hashGetStdHandle);
    typedef BOOL (WINAPI* _WriteFile)(
        _In_ HANDLE hFile, _In_reads_bytes_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
        _In_ DWORD nNumberOfBytesToWrite, _Out_opt_ LPDWORD lpNumberOfBytesWritten,
        _Inout_opt_ LPOVERLAPPED lpOverlapped
    );
    constexpr unsigned int hashWriteFile = ComplexHashForAnsi("WriteFile");
    _WriteFile pWriteFile = (_WriteFile)apiResolve.GetApiAddress(lpKernel32, hashWriteFile);

    HANDLE hStdout = pGetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written = 0;
    pWriteFile(hStdout, str, (DWORD)len, &written, NULL);
    return (int)written;
}

static const char* crt_format_next(const char* fmt, int* width, int* precision, int* flags, char* spec) {
    *width = 0;
    *precision = -1;
    *flags = 0;
    *spec = '\0';

    if (*fmt != '%') return fmt;
    fmt++;

    while (*fmt) {
        switch (*fmt) {
        case '-': *flags |= 1; fmt++; break;
        case '+': *flags |= 2; fmt++; break;
        case ' ': *flags |= 4; fmt++; break;
        case '0': *flags |= 8; fmt++; break;
        case '#': *flags |= 16; fmt++; break;
        default: goto width_parse;
        }
    }

width_parse:
    if (*fmt >= '0' && *fmt <= '9') {
        int w = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            w = w * 10 + (*fmt - '0');
            fmt++;
        }
        *width = w;
    }

    if (*fmt == '.') {
        fmt++;
        int p = 0;
        int has_digit = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            p = p * 10 + (*fmt - '0');
            has_digit = 1;
            fmt++;
        }
        *precision = has_digit ? p : 0;
    }

    *spec = *fmt;
    return fmt + 1;
}

static int crt_putint(char* buf, unsigned long long val, int width, int prec, int flags, int negative, int upper, int base) {
    char tmp[32];
    int i = 0;

    if (val == 0) {
        tmp[i++] = '0';
    } else {
        while (val > 0) {
            int d = (int)(val % (unsigned long long)base);
            if (d < 10) {
                tmp[i++] = (char)('0' + d);
            } else {
                tmp[i++] = (char)((upper ? 'A' : 'a') + d - 10);
            }
            val /= (unsigned long long)base;
        }
    }

    int out_i = 0;

    if (!(flags & 1) && (flags & 8) && !negative) {
        while (i < width) {
            buf[out_i++] = '0';
            i++;
        }
    }

    if (negative) {
        buf[out_i++] = '-';
    } else if ((flags & 2)) {
        buf[out_i++] = '+';
    } else if ((flags & 4)) {
        buf[out_i++] = ' ';
    }

    int pad = width - i;
    if ((flags & 1)) {
        while (i > 0) {
            buf[out_i++] = tmp[--i];
        }
        while (pad > 0) {
            buf[out_i++] = ' ';
            pad--;
        }
    } else {
        while (pad > 0 && !(flags & 8)) {
            buf[out_i++] = ' ';
            pad--;
        }
        while (i > 0) {
            buf[out_i++] = tmp[--i];
        }
    }

    return out_i;
}

static int crt_puthex(char* buf, unsigned long long val, int width, int prec, int flags, int upper) {
    char tmp[32];
    int i = 0;

    if (val == 0) {
        tmp[i++] = '0';
    } else {
        while (val > 0) {
            int d = (int)(val % 16);
            if (d < 10) {
                tmp[i++] = (char)('0' + d);
            } else {
                tmp[i++] = (char)((upper ? 'A' : 'a') + d - 10);
            }
            val /= 16;
        }
    }

    int out_i = 0;

    if ((flags & 16) && val == 0 && i > 0) {
        buf[out_i++] = upper ? 'X' : 'x';
        buf[out_i++] = '0';
    }

    int pad = width - i;
    if (!(flags & 1) && (flags & 8)) {
        while (pad > 0) {
            buf[out_i++] = '0';
            pad--;
        }
    }

    while (i > 0) {
        buf[out_i++] = tmp[--i];
    }

    if (flags & 1) {
        while (pad > 0) {
            buf[out_i++] = ' ';
            pad--;
        }
    }

    return out_i;
}

static int crt_putpointer(char* buf, void* ptr) {
    unsigned long long val = (unsigned long long)(SIZE_T)ptr;
    char tmp[32];
    int i = 0;

    if (val == 0) {
        tmp[i++] = '0';
    } else {
        while (val > 0) {
            int d = (int)(val % 16);
            if (d < 10) {
                tmp[i++] = (char)('0' + d);
            } else {
                tmp[i++] = (char)('a' + d - 10);
            }
            val /= 16;
        }
    }

    int out_i = 0;
    buf[out_i++] = '0';
    buf[out_i++] = 'x';

    while (i > 0) {
        buf[out_i++] = tmp[--i];
    }

    return out_i;
}

static int crt_vsnprintf_impl(char* buf, SIZE_T count, const char* fmt, va_list args) {
    if (buf == NULL || count == 0 || fmt == NULL) {
        return -1;
    }

    if (count == (SIZE_T)(-1)) {
        count = (SIZE_T)0x7FFFFFFF;
    }

    char temp[64];
    const char* p = fmt;
    char* out = buf;
    SIZE_T remaining = count;
    int total = 0;

    while (*p && remaining > 1) {
        if (*p != '%') {
            *out++ = *p++;
            remaining--;
            total++;
            continue;
        }

        int width = 0, prec = -1, flags = 0;
        char spec = '\0';
        const char* next = crt_format_next(p, &width, &prec, &flags, &spec);
        int chars = 0;

        if (spec == 'd' || spec == 'i')
        {
            int v = va_arg(args, int);
            unsigned int uv;
            int neg = 0;

            if (v < 0)
            {
                uv = 0u - v;
                neg = 1;
            }
            else
            {
                uv = (unsigned int)v;
            }

            chars = crt_putint(temp, uv, width, prec, flags, neg, 0, 10);
        }
        else if (spec == 'u')
        {
            unsigned int v = va_arg(args, unsigned int);
            chars = crt_putint(temp, v, width, prec, flags, 0, 0, 10);
        }
        else if (spec == 'o')
        {
            unsigned int v = va_arg(args, unsigned int);
            chars = crt_putint(temp, v, width, prec, flags, 0, 0, 8);
        }
        else if (spec == 'x' || spec == 'X')
        {
            unsigned int v = va_arg(args, unsigned int);
            chars = crt_puthex(temp, v, width, prec, flags, (spec == 'X') ? 1 : 0);
        }
        else if (spec == 'c')
        {
            temp[0] = (char)va_arg(args, int);
            chars = 1;
        }
        else if (spec == 's')
        {
            const char* s = va_arg(args, char*);
            if (s == NULL) { 
                char sNull[] = {'(', 'n' , 'u', 'l', 'l', '\0'};
                s = sNull; 
            }
            int len = 0;
            while (s[len]) len++;
            if (prec >= 0 && prec < len) len = prec;
            if (len >= (int)(SIZE_T)(remaining)) {
                if (remaining > 1) {
                    crt_strncpy(out, s, remaining - 1);
                }
                buf[count - 1] = '\0';
                int fmt_remaining = 0;
                const char* r = next;
                while (r && *r) { fmt_remaining++; r++; }
                return total + fmt_remaining;
            }
            crt_strncpy(out, s, len);
            out = out + len;
            remaining = remaining - (SIZE_T)len;
            total = total + len;
            p = next;
            continue;
        }
        else if (spec == 'p')
        {
            void* ptr = va_arg(args, void*);
            chars = crt_putpointer(temp, ptr);
        }
        else if (spec == '%')
        {
            temp[0] = '%';
            chars = 1;
        }
        else if (spec == 'n')
        {
            int* np = va_arg(args, int*);
            if (np)
                *np = total;

            chars = 0;
        }
        else if (spec == 'l')
        {
            if (p[1] == 'd' || p[1] == 'i')
            {
                long v = va_arg(args, long);
                unsigned long uv;
                int neg = 0;

                if (v < 0)
                {
                    uv = 0ul - v;
                    neg = 1;
                }
                else
                {
                    uv = (unsigned long)v;
                }

                chars = crt_putint(temp, uv, width, prec, flags, neg, 0, 10);
            }
            else if (p[1] == 'u')
            {
                unsigned long v = va_arg(args, unsigned long);
                chars = crt_putint(temp, v, width, prec, flags, 0, 0, 10);
            }
            else if (p[1] == 'l' && (p[2] == 'd' || p[2] == 'i'))
            {
                long long v = va_arg(args, long long);
                unsigned long long uv;
                int neg = 0;

                if (v < 0)
                {
                    uv = 0ull - v;
                    neg = 1;
                }
                else
                {
                    uv = (unsigned long long)v;
                }

                chars = crt_putint(temp, uv, width, prec, flags, neg, 0, 10);
                p += 2;
            }
            else if (p[1] == 'l' && p[2] == 'u')
            {
                unsigned long long v = va_arg(args, unsigned long long);
                chars = crt_putint(temp, v, width, prec, flags, 0, 0, 10);
                p += 2;
            }
            else
            {
                temp[0] = 'l';
                chars = 1;
            }
        }
        else if (spec == 'z')
        {
            if (p[1] == 'u')
            {
                SIZE_T v = va_arg(args, SIZE_T);
                chars = crt_putint(temp, (unsigned long long)v, width, prec, flags, 0, 0, 10);
            }
            else if (p[1] == 'd' || p[1] == 'i')
            {
                SIZE_T v = va_arg(args, SIZE_T);
                unsigned long long uv = (unsigned long long)v;
                int neg = 0;

                if ((long)v < 0)
                {
                    uv = 0ull - v;
                    neg = 1;
                }

                chars = crt_putint(temp, uv, width, prec, flags, neg, 0, 10);
            }
            else
            {
                temp[0] = 'z';
                chars = 1;
            }
        }
        else
        {
            temp[0] = *p;
            chars = 1;
        }

        if (chars >= (int)remaining) {
            crt_strncpy(out, temp, remaining - 1);
            buf[count - 1] = '\0';
            int fmt_remaining = 0;
            const char* r = next;
            while (r && *r) { fmt_remaining++; r++; }
            return total + fmt_remaining;
        }

        for (int i = 0; i < chars && remaining > 1; i++) {
            *out++ = temp[i];
            remaining--;
            total++;
        }

        p = next;
    }

    if (remaining > 0) {
        *out = '\0';
    } else {
        buf[count - 1] = '\0';
    }
    return total;
}

int __cdecl crt_vsnprintf(char* buf, SIZE_T count, const char* fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    int result = crt_vsnprintf_impl(buf, count, fmt, args_copy);
    va_end(args_copy);
    return result;
}

int __cdecl crt_vprintf(const char* fmt, va_list args) {
    char buffer[512];
    int len = crt_vsnprintf(buffer, sizeof(buffer), fmt, args);
    if (len > 0) {
        crt_cpputs(buffer, len);
    }
    return len;
}

int __cdecl crt_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = crt_vprintf(fmt, args);
    va_end(args);
    return result;
}

int __cdecl crt_sprintf(char* buf, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = crt_vsnprintf_impl(buf, (SIZE_T)(-1), fmt, args);
    va_end(args);
    return result;
}

int __cdecl crt_vsprintf(char* buf, const char* fmt, va_list args) {
    return crt_vsnprintf_impl(buf, (SIZE_T)(-1), fmt, args);
}

int __cdecl crt_snprintf(char* buf, SIZE_T count, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = crt_vsnprintf_impl(buf, count, fmt, args);
    va_end(args);
    return result;
}

static void quick_sort_helper(void* base, SIZE_T num, SIZE_T width,
                              int (__cdecl* compare)(const void*, const void*),
                              char* work_buffer) {
    if (num <= 1) {
        return;
    }
    
    char* arr = (char*)base;
    SIZE_T i, j;
    
    char* pivot = arr + (num / 2) * width;
    
    char* left = arr;
    char* right = arr + (num - 1) * width;
    
    while (left <= right) {
        while (compare(left, pivot) < 0) {
            left += width;
        }
        while (compare(right, pivot) > 0) {
            right -= width;
        }
        
        if (left <= right) {
            crt_memcpy(work_buffer, left, width);
            crt_memcpy(left, right, width);
            crt_memcpy(right, work_buffer, width);
            
            if (left == pivot) {
                pivot = right;
            } else if (right == pivot) {
                pivot = left;
            }
            
            left += width;
            right -= width;
        }
    }
    
    SIZE_T leftSize = (SIZE_T)((right - arr) / width) + 1;
    SIZE_T rightSize = num - leftSize;
    
    if (leftSize > 1) {
        quick_sort_helper(arr, leftSize, width, compare, work_buffer);
    }
    
    if (rightSize > 1) {
        quick_sort_helper(left, rightSize, width, compare, work_buffer);
    }
}

void __cdecl crt_qsort(void* base, SIZE_T num, SIZE_T width,
                       int (__cdecl* compare)(const void*, const void*)) {
    if (base == NULL || num <= 1 || width == 0 || compare == NULL) {
        return;
    }
    
    char* work_buffer = (char*)crt_malloc(width);
    if (work_buffer == NULL) {
        return;
    }
    
    quick_sort_helper(base, num, width, compare, work_buffer);
    
    crt_free(work_buffer);
}

void* __cdecl crt_bsearch(const void* key, const void* base,
                          SIZE_T num, SIZE_T width,
                          int (__cdecl* compare)(const void*, const void*)) {
    if (key == NULL || base == NULL || num == 0 || width == 0 || compare == NULL) {
        return NULL;
    }
    
    const char* arr = (const char*)base;
    SIZE_T left = 0;
    SIZE_T right = num;
    
    while (left < right) {
        SIZE_T mid = left + (right - left) / 2;
        const char* middle = arr + mid * width;
        
        int cmp = compare(key, middle);
        
        if (cmp == 0) {
            return (void*)middle;
        } else if (cmp < 0) {
            if (mid == 0) break;
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    
    return NULL;
}

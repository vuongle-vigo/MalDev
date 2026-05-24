#include "CRT.h"

//=============================================================================
// MEMORY ALLOCATION FUNCTIONS
//=============================================================================

void*    crt_malloc(SIZE_T Size) {
    if (Size == 0) {
        Size = 1;
    }
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
}

void* __cdecl crt_calloc(SIZE_T Count, SIZE_T Size) {
    if (Count == 0 || Size == 0) {
        return NULL;
    }
    SIZE_T totalSize = Count * Size;
    if (totalSize / Count != Size) {
        return NULL;
    }
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, totalSize);
}

void* __cdecl crt_realloc(void* Block, SIZE_T NewSize) {
    if (Block == NULL) {
        return crt_malloc(NewSize);
    }
    if (NewSize == 0) {
        crt_free(Block);
        return NULL;
    }
    
    SIZE_T oldSize = HeapSize(GetProcessHeap(), 0, Block);
    void* newBlock = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NewSize);
    
    if (newBlock == NULL) {
        return NULL;
    }
    
    if (oldSize != (SIZE_T)-1) {
        SIZE_T copySize = (oldSize < NewSize) ? oldSize : NewSize;
        crt_memcpy(newBlock, Block, copySize);
        HeapFree(GetProcessHeap(), 0, Block);
    }
    
    return newBlock;
}

void __cdecl crt_free(void* Block) {
    if (Block != NULL) {
        HeapFree(GetProcessHeap(), 0, Block);
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

static char* strtok_context = NULL;

char* __cdecl crt_strtok(char* str, const char* delim) {
    if (delim == NULL) {
        return NULL;
    }
    
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

static wchar_t* wcstok_context = NULL;

wchar_t* __cdecl crt_wcstok(wchar_t* str, const wchar_t* delim) {
    if (delim == NULL) {
        return NULL;
    }
    
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
    
    MultiByteToWideChar(CP_ACP, 0, str, -1, result, (int)len);
    return result;
}

char* __cdecl crt_wctoa(const wchar_t* str) {
    if (str == NULL) {
        return NULL;
    }
    
    int len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    if (len == 0) {
        return NULL;
    }
    
    char* result = (char*)crt_malloc(len);
    if (result == NULL) {
        return NULL;
    }
    
    WideCharToMultiByte(CP_ACP, 0, str, -1, result, len, NULL, NULL);
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
        uvalue = (unsigned int)(-(value + 1)) + 1;
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
        uvalue = (unsigned long long)(-(value + 1)) + 1;
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
        uvalue = (unsigned int)(-(value + 1)) + 1;
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
        uvalue = (unsigned long long)(-(value + 1)) + 1;
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
// UTILITY FUNCTIONS
//=============================================================================

static unsigned int rand_seed = 1;

void __cdecl crt_srand(unsigned int seed) {
    rand_seed = seed;
}

int __cdecl crt_rand(void) {
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

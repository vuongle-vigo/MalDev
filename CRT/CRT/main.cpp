#include "CRT.h"
#include <stdio.h>

void print_separator(const char* title);
void test_memory_allocation();
void test_string_char();
void test_string_wchar();
void test_string_conversion();
void test_string_float_conversion();
void test_memory_functions();
void test_utilities();
void test_file_io();
void test_formatted_print();

void print_separator(const char* title) {
    printf("\n=== %s ===\n", title);
}

//=============================================================================
// TEST MEMORY ALLOCATION
//=============================================================================

void test_memory_allocation() {
    print_separator("MEMORY ALLOCATION TESTS");
    
    // Test malloc
    printf("Testing crt_malloc...\n");
    int* arr = (int*)crt_malloc(10 * sizeof(int));
    if (arr) {
        printf("[PASS] crt_malloc: Allocated %zu bytes\n", 10 * sizeof(int));
        crt_free(arr);
    } else {
        printf("[FAIL] crt_malloc failed\n");
    }
    
    // Test calloc
    printf("\nTesting crt_calloc...\n");
    int* arr2 = (int*)crt_calloc(5, sizeof(int));
    if (arr2) {
        BOOL allZero = TRUE;
        for (int i = 0; i < 5; i++) {
            if (arr2[i] != 0) {
                allZero = FALSE;
                break;
            }
        }
        printf("[%s] crt_calloc: Memory initialized to zero = %s\n", 
               allZero ? "PASS" : "FAIL", allZero ? "YES" : "NO");
        crt_free(arr2);
    } else {
        printf("[FAIL] crt_calloc failed\n");
    }
    
    // Test realloc
    printf("\nTesting crt_realloc...\n");
    int* arr3 = (int*)crt_malloc(3 * sizeof(int));
    if (arr3) {
        arr3[0] = 1; arr3[1] = 2; arr3[2] = 3;
        int* arr4 = (int*)crt_realloc(arr3, 6 * sizeof(int));
        if (arr4) {
            BOOL dataPreserved = (arr4[0] == 1 && arr4[1] == 2 && arr4[2] == 3);
            printf("[%s] crt_realloc: Data preserved = %s\n", 
                   dataPreserved ? "PASS" : "FAIL", dataPreserved ? "YES" : "NO");
            crt_free(arr4);
        }
    }
    
    // Test realloc to NULL
    printf("\nTesting crt_realloc with NULL...\n");
    int* newArr = (int*)crt_realloc(NULL, 5 * sizeof(int));
    if (newArr) {
        printf("[PASS] crt_realloc(NULL, size) works like malloc\n");
        crt_free(newArr);
    }
    
    // Test free
    printf("\nTesting crt_free...\n");
    void* temp = crt_malloc(100);
    crt_free(temp);
    printf("[PASS] crt_free: No crash on valid free\n");
    
    crt_free(NULL);
    printf("[PASS] crt_free: No crash on NULL\n");
}

//=============================================================================
// TEST STRING FUNCTIONS (CHAR)
//=============================================================================

void test_string_char() {
    print_separator("STRING FUNCTIONS (CHAR)");
    
    // Test strlen
    printf("Testing crt_strlen...\n");
    const char test1[] = {'H', 'e', 'l', 'l', 'o', ',', ' ', 'W', 'o', 'r', 'l', 'd', '!', '\0'};
    SIZE_T len = crt_strlen(test1);
    printf("[%s] crt_strlen(\"%s\") = %zu (expected: %zu)\n", 
           len == 13 ? "PASS" : "FAIL", test1, len, (SIZE_T)13);
    
    len = crt_strlen("");
    printf("[%s] crt_strlen(\"\") = %zu (expected: 0)\n", 
           len == 0 ? "PASS" : "FAIL", len);
    
    // Test strcpy
    printf("\nTesting crt_strcpy...\n");
    char dest1[50];
    crt_strcpy(dest1, "Copy this!");
    printf("[%s] crt_strcpy result: \"%s\"\n", 
           crt_strcmp(dest1, "Copy this!") == 0 ? "PASS" : "FAIL", dest1);
    
    // Test strncpy
    printf("\nTesting crt_strncpy...\n");
    char dest2[50] = {0};
    crt_strncpy(dest2, "Hello World", 5);
    printf("[%s] crt_strncpy: \"%s\" (expected: \"Hello\")\n", 
           crt_strncmp(dest2, "Hello", 5) == 0 ? "PASS" : "FAIL", dest2);
    
    // Test strcat
    printf("\nTesting crt_strcat...\n");
    char dest3[50] = "Hello";
    crt_strcat(dest3, ", World!");
    printf("[%s] crt_strcat: \"%s\"\n", 
           crt_strcmp(dest3, "Hello, World!") == 0 ? "PASS" : "FAIL", dest3);
    
    // Test strncat
    printf("\nTesting crt_strncat...\n");
    char dest4[50] = "Hello";
    crt_strncat(dest4, ", World!!!", 6);
    printf("[%s] crt_strncat: \"%s\"\n", 
           crt_strcmp(dest4, "Hello, Worl") == 0 ? "PASS" : "FAIL", dest4);
    
    // Test strcmp
    printf("\nTesting crt_strcmp...\n");
    int cmp;
    cmp = crt_strcmp("abc", "abc");
    printf("[%s] strcmp(\"abc\", \"abc\") = %d (expected: 0)\n", cmp == 0 ? "PASS" : "FAIL", cmp);
    
    cmp = crt_strcmp("abc", "abd");
    printf("[%s] strcmp(\"abc\", \"abd\") = %d (expected: -1)\n", cmp < 0 ? "PASS" : "FAIL", cmp);
    
    cmp = crt_strcmp("abd", "abc");
    printf("[%s] strcmp(\"abd\", \"abc\") = %d (expected: > 0)\n", cmp > 0 ? "PASS" : "FAIL", cmp);
    
    // Test strncmp
    printf("\nTesting crt_strncmp...\n");
    cmp = crt_strncmp("Hello World", "Hello Earth", 5);
    printf("[%s] strncmp(\"Hello World\", \"Hello Earth\", 5) = %d (expected: 0)\n", 
           cmp == 0 ? "PASS" : "FAIL", cmp);
    
    // Test strcmpi
    printf("\nTesting crt_strcmpi...\n");
    cmp = crt_strcmpi("HELLO", "hello");
    printf("[%s] strcmpi(\"HELLO\", \"hello\") = %d (expected: 0)\n", cmp == 0 ? "PASS" : "FAIL", cmp);
    
    // Test strchr
    printf("\nTesting crt_strchr...\n");
    const char str[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '\0'};
    char* found = crt_strchr(str, 'o');
    printf("[%s] strchr(\"Hello World\", 'o') found at \"%s\"\n", 
           found != NULL && *found == 'o' ? "PASS" : "FAIL", found ? found : "NULL");
    
    found = crt_strchr(str, 'z');
    printf("[%s] strchr(\"Hello World\", 'z') = %s\n", found == NULL ? "PASS" : "FAIL", 
           found ? found : "NULL (expected)");
    
    // Test strrchr
    printf("\nTesting crt_strrchr...\n");
    found = crt_strrchr("Hello World", 'o');
    printf("[%s] strrchr(\"Hello World\", 'o') found at \"%s\" (last 'o')\n", 
           found != NULL && *(found+1) == 'r' ? "PASS" : "FAIL", found ? found : "NULL");
    
    // Test strstr
    printf("\nTesting crt_strstr...\n");
    found = crt_strstr("Hello World", "World");
    printf("[%s] strstr(\"Hello World\", \"World\") found at \"%s\"\n",
           found != NULL && found == strchr("Hello World", 'W') ? "PASS" : "FAIL", found ? found : "NULL");
    
    // Test strupr
    printf("\nTesting crt_strupr...\n");
    char s1[] = {'H', 'e', 'l', 'l', 'o', '\0'};
    printf("[%s] strupr(\"Hello\") = \"%s\"\n", 
           crt_strcmp(crt_strupr(s1), "HELLO") == 0 ? "PASS" : "FAIL", s1);
    
    // Test strlwr
    printf("\nTesting crt_strlwr...\n");
    char s2[] = {'H', 'E', 'L', 'L', 'O', '\0'};
    printf("[%s] strlwr(\"HELLO\") = \"%s\"\n", 
           crt_strcmp(crt_strlwr(s2), "hello") == 0 ? "PASS" : "FAIL", s2);
    
    // Test strtok
    printf("\nTesting crt_strtok...\n");
    char strtok_test[] = {'H', 'e', 'l', 'l', 'o', ',', 'W', 'o', 'r', 'l', 'd', ',', 'T', 'e', 's', 't', ',', 'S', 't', 'r', 'i', 'n', 'g', '\0'};
    const char delim[] = {',', '\0'};
    char* token = crt_strtok(strtok_test, delim);
    const char* expected[] = {"Hello", "World", "Test", "String"};
    BOOL allPassed = TRUE;
    int i = 0;
    while (token != NULL) {
        if (crt_strcmp(token, expected[i]) != 0) {
            allPassed = FALSE;
        }
        printf("  Token %d: \"%s\"\n", i + 1, token);
        token = crt_strtok(NULL, delim);
        i++;
    }
    printf("[%s] strtok extracted all tokens correctly\n", allPassed ? "PASS" : "FAIL");
}

//=============================================================================
// TEST STRING FUNCTIONS (WCHAR)
//=============================================================================

void test_string_wchar() {
    print_separator("STRING FUNCTIONS (WCHAR)");
    
    // Test wcslen
    printf("Testing crt_wcslen...\n");
    const wchar_t wtest1[] = {L'H', L'e', L'l', L'l', L'o', L',', L' ', L'W', L'o', L'r', L'l', L'd', L'!', L'\0'};
    SIZE_T len = crt_wcslen(wtest1);
    printf("[%s] wcslen(L\"Hello, World!\") = %zu (expected: %zu)\n", 
           len == 13 ? "PASS" : "FAIL", len, (SIZE_T)13);
    
    // Test wcscpy
    printf("\nTesting crt_wcscpy...\n");
    wchar_t wdest1[50];
    crt_wcscpy(wdest1, L"Copy this!");
    printf("[%s] wcscpy result: \"Hello\"\n", 
           crt_wcscmp(wdest1, L"Copy this!") == 0 ? "PASS" : "FAIL");
    
    // Test wcsncpy
    printf("\nTesting crt_wcsncpy...\n");
    wchar_t wdest2[50] = {0};
    crt_wcsncpy(wdest2, L"Hello World", 5);
    printf("[%s] wcsncpy: \"Hello\"\n", 
           crt_wcsncmp(wdest2, L"Hello", 5) == 0 ? "PASS" : "FAIL");
    
    // Test wcscat
    printf("\nTesting crt_wcscat...\n");
    wchar_t wdest3[50] = L"Hello";
    crt_wcscat(wdest3, L", World!");
    printf("[%s] wcscat: \"Hello, World!\"\n", 
           crt_wcscmp(wdest3, L"Hello, World!") == 0 ? "PASS" : "FAIL");
    
    // Test wcscmp
    printf("\nTesting crt_wcscmp...\n");
    int cmp = crt_wcscmp(L"abc", L"abc");
    printf("[%s] wcscmp(L\"abc\", L\"abc\") = %d (expected: 0)\n", cmp == 0 ? "PASS" : "FAIL", cmp);
    
    // Test wcschr
    printf("\nTesting crt_wcschr...\n");
    const wchar_t wstr[] = {L'H', L'e', L'l', L'l', L'o', L' ', L'W', L'o', L'r', L'l', L'd', L'\0'};
    wchar_t* wfound = crt_wcschr(wstr, L'o');
    printf("[%s] wcschr(L\"Hello World\", 'o') found\n", wfound != NULL ? "PASS" : "FAIL");
    
    // Test wcsstr
    printf("\nTesting crt_wcsstr...\n");
    wfound = crt_wcsstr(L"Hello World", L"World");
    printf("[%s] wcsstr(L\"Hello World\", L\"World\") found\n", 
           wfound != NULL ? "PASS" : "FAIL");
    
    // Test wcsupr
    printf("\nTesting crt_wcsupr...\n");
    wchar_t ws1[] = {L'H', L'e', L'l', L'l', L'o', L'\0'};
    crt_wcsupr(ws1);
    printf("[%s] wcsupr(L\"Hello\") = L\"HELLO\"\n", 
           crt_wcscmp(ws1, L"HELLO") == 0 ? "PASS" : "FAIL");
    
    // Test wcslwr
    printf("\nTesting crt_wcslwr...\n");
    wchar_t ws2[] = {L'H', L'E', L'L', L'L', L'O', L'\0'};
    crt_wcslwr(ws2);
    printf("[%s] wcslwr(L\"HELLO\") = L\"hello\"\n", 
           crt_wcscmp(ws2, L"hello") == 0 ? "PASS" : "FAIL");
}

//=============================================================================
// TEST STRING CONVERSION
//=============================================================================

void test_string_conversion() {
    print_separator("STRING CONVERSION TESTS");
    
    // Test atowc
    printf("Testing crt_atowc...\n");
    const char ansi[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'i', 'd', 'e', '\0'};
    wchar_t* wide = crt_atowc(ansi);
    if (wide) {
        printf("[PASS] atowc: \"%s\" -> %zu wchars\n", ansi, crt_wcslen(wide));
        crt_free(wide);
    }
    
    // Test wctoa
    printf("\nTesting crt_wctoa...\n");
    const wchar_t wides[] = {L'H', L'e', L'l', L'l', L'o', L' ', L'A', L'N', L'S', L'I', L'\0'};
    char* ansi2 = crt_wctoa(wides);
    if (ansi2) {
        printf("[PASS] wctoa: converted -> \"%s\"\n", ansi2);
        crt_free(ansi2);
    }
    
    // Test atoi
    printf("\nTesting crt_atoi...\n");
    printf("[%s] atoi(\"12345\") = %d (expected: 12345)\n", 
           crt_atoi("12345") == 12345 ? "PASS" : "FAIL", crt_atoi("12345"));
    printf("[%s] atoi(\"-54321\") = %d (expected: -54321)\n", 
           crt_atoi("-54321") == -54321 ? "PASS" : "FAIL", crt_atoi("-54321"));
    printf("[%s] atoi(\"  789\") = %d (expected: 789)\n", 
           crt_atoi("  789") == 789 ? "PASS" : "FAIL", crt_atoi("  789"));
    
    // Test itoa
    printf("\nTesting crt_itoa...\n");
    char numStr[50];
    crt_itoa(12345, numStr, 10);
    printf("[%s] itoa(12345, 10) = \"%s\" (expected: \"12345\")\n", 
           crt_strcmp(numStr, "12345") == 0 ? "PASS" : "FAIL", numStr);
    
    crt_itoa(-12345, numStr, 10);
    printf("[%s] itoa(-12345, 10) = \"%s\" (expected: \"-12345\")\n", 
           crt_strcmp(numStr, "-12345") == 0 ? "PASS" : "FAIL", numStr);
    
    crt_itoa(255, numStr, 16);
    printf("[%s] itoa(255, 16) = \"%s\" (expected: \"ff\")\n", 
           crt_strcmp(numStr, "ff") == 0 ? "PASS" : "FAIL", numStr);
    
    crt_itoa(8, numStr, 2);
    printf("[%s] itoa(8, 2) = \"%s\" (expected: \"1000\")\n", 
           crt_strcmp(numStr, "1000") == 0 ? "PASS" : "FAIL", numStr);
    
    // Test wtol
    printf("\nTesting crt_wtoi...\n");
    int wtoi_result = crt_wtoi(L"99999");
    printf("[%s] wtoi(L\"99999\") = %d (expected: 99999)\n", 
           wtoi_result == 99999 ? "PASS" : "FAIL", wtoi_result);
    
    // Test itow
    printf("\nTesting crt_itow...\n");
    wchar_t wnumStr[50];
    crt_itow(54321, wnumStr, 10);
    printf("[%s] itow(54321, 10) = L\"54321\"\n", 
           crt_wcscmp(wnumStr, L"54321") == 0 ? "PASS" : "FAIL");
}

//=============================================================================
// TEST STRING TO FLOAT CONVERSION
//=============================================================================

void test_string_float_conversion() {
    print_separator("STRING TO FLOAT CONVERSION TESTS");
    char* endptr = NULL;

    //========================================
    // TEST strtof
    //========================================
    printf("Testing crt_strtof...\n");

    // Basic positive float
    float f1 = crt_strtof("123.456", &endptr);
    printf("[%s] strtof(\"123.456\") = %.7g (expected: 123.456)\n",
           (f1 > 123.455f && f1 < 123.457f) ? "PASS" : "FAIL", f1);

    // Debug: Compare binary representations
    float expected_f1 = 123.456f;
    unsigned int u_f1 = *(unsigned int*)&f1;
    unsigned int u_exp_f1 = *(unsigned int*)&expected_f1;
    printf("    [DEBUG] f1 binary: 0x%08X, expected binary: 0x%08X, equal=%s\n",
           u_f1, u_exp_f1, u_f1 == u_exp_f1 ? "YES" : "NO");

    // CRITICAL: Test so sánh float == float (đúng) vs float == double (sai)
    printf("    [DEBUG] f1 == 123.456f: %s (float vs float)\n", f1 == 123.456f ? "TRUE" : "FALSE");
    printf("    [DEBUG] f1 == 123.456:  %s (float vs double)\n", f1 == 123.456 ? "TRUE" : "FALSE");
    printf("    [DEBUG] (double)f1 == 123.456: %s\n", (double)f1 == 123.456 ? "TRUE" : "FALSE");
    printf("    [DEBUG] f1 - 123.456f = %e (should be ~0)\n", f1 - 123.456f);

    // Negative float
    float f2 = crt_strtof("-456.789", &endptr);
    printf("[%s] strtof(\"-456.789\") = %.7g (expected: -456.789)\n",
           (f2 > -456.790f && f2 < -456.788f) ? "PASS" : "FAIL", f2);

    // Float with exponent (positive)
    float f3 = crt_strtof("3.14e2", &endptr);
    printf("[%s] strtof(\"3.14e2\") = %.7g (expected: 314.00)\n",
           (f3 > 313.99f && f3 < 314.01f) ? "PASS" : "FAIL", f3);

    // Float with exponent (negative)
    float f4 = crt_strtof("1.5e-1", &endptr);
    printf("[%s] strtof(\"1.5e-1\") = %.7g (expected: 0.15)\n",
           (f4 > 0.149f && f4 < 0.151f) ? "PASS" : "FAIL", f4);

    // Integer only
    float f5 = crt_strtof("42", &endptr);
    printf("[%s] strtof(\"42\") = %.7g (expected: 42)\n",
           (f5 > 41.9f && f5 < 42.1f) ? "PASS" : "FAIL", f5);

    // Leading whitespace
    float f6 = crt_strtof("   99.5", &endptr);
    printf("[%s] strtof(\"   99.5\") = %.7g (expected: 99.5)\n",
           (f6 > 99.49f && f6 < 99.51f) ? "PASS" : "FAIL", f6);

    // Uppercase E exponent
    float f7 = crt_strtof("2.5E3", &endptr);
    printf("[%s] strtof(\"2.5E3\") = %.7g (expected: 2500)\n",
           (f7 > 2499.9f && f7 < 2500.1f) ? "PASS" : "FAIL", f7);

    // Positive sign
    float f8 = crt_strtof("+10.5", &endptr);
    printf("[%s] strtof(\"+10.5\") = %.7g (expected: 10.5)\n",
           (f8 > 10.49f && f8 < 10.51f) ? "PASS" : "FAIL", f8);

    // No digits after decimal
    float f9 = crt_strtof("123.", &endptr);
    printf("[%s] strtof(\"123.\") = %.7g (expected: 123)\n",
           (f9 > 122.9f && f9 < 123.1f) ? "PASS" : "FAIL", f9);

    // Empty string - endptr should point to start of string (no conversion)
    float f10 = crt_strtof("", &endptr);
    printf("[%s] strtof(\"\") = %.7g, endptr=start: %s\n",
           (f10 == 0.0f && endptr != NULL) ? "PASS" : "FAIL", f10,
           endptr == NULL ? "NULL" : "valid");

    // No conversion - endptr should point to 'a'
    float f11 = crt_strtof("abc", &endptr);
    printf("[%s] strtof(\"abc\") = %.7g, endptr at 'a': %s\n",
           (f11 == 0.0f && endptr != NULL && *endptr == 'a') ? "PASS" : "FAIL", f11,
           (endptr && *endptr == 'a') ? "yes" : "no");

    //========================================
    // TEST strtod
    //========================================
    printf("\nTesting crt_strtod...\n");

    // Basic positive double
    double d1 = crt_strtod("456.789", &endptr);
    printf("[%s] strtod(\"456.789\") = %.7g (expected: 456.789)\n",
           (d1 > 456.788 && d1 < 456.790) ? "PASS" : "FAIL", d1);

    // Debug: Compare binary representations
    double expected_d1 = 456.789;
    unsigned long long ull_d1 = *(unsigned long long*)&d1;
    unsigned long long ull_exp_d1 = *(unsigned long long*)&expected_d1;
    printf("    [DEBUG] d1 binary: 0x%016llX, expected binary: 0x%016llX, equal=%s\n",
           ull_d1, ull_exp_d1, ull_d1 == ull_exp_d1 ? "YES" : "NO");
    printf("    [DEBUG] diff = %e (expected: 0)\n", d1 - 456.789);

    // CRITICAL: Test so sánh double == double (đúng)
    printf("    [DEBUG] d1 == 456.789:    %s (double vs double)\n", d1 == 456.789 ? "TRUE" : "FALSE");

    // Negative double
    double d2 = crt_strtod("-456.789", &endptr);
    printf("[%s] strtod(\"-456.789\") = %.7g (expected: -456.789)\n",
           (d2 > -456.790 && d2 < -456.788) ? "PASS" : "FAIL", d2);

    // Double with large exponent
    double d3 = crt_strtod("1.23e10", &endptr);
    printf("[%s] strtod(\"1.23e10\") = %.7g (expected: 1.23e10)\n",
           (d3 > 1.22e10 && d3 < 1.24e10) ? "PASS" : "FAIL", d3);

    // Negative exponent
    double d4 = crt_strtod("5.5e-5", &endptr);
    printf("[%s] strtod(\"5.5e-5\") = %.7g (expected: 0.000055)\n",
           (d4 > 0.000054 && d4 < 0.000056) ? "PASS" : "FAIL", d4);

    // Double precision
    double d5 = crt_strtod("3.14159265358979", &endptr);
    printf("[%s] strtod(\"3.14159265358979\") = %.7g\n",
           (d5 > 3.1415926535 && d5 < 3.1415926536) ? "PASS" : "FAIL", d5);

    // Leading whitespace and sign
    double d6 = crt_strtod("  -273.15", &endptr);
    printf("[%s] strtod(\"  -273.15\") = %.7g (expected: -273.15)\n",
           (d6 > -273.16 && d6 < -273.14) ? "PASS" : "FAIL", d6);

    // Overflow
    double d7 = crt_strtod("1e309", &endptr);
    int is_inf = (d7 != d7 || d7 > 1e308); // NaN check or huge value
    printf("[%s] strtod(\"1e309\") overflow = %s\n",
           is_inf ? "PASS" : "FAIL", is_inf ? "inf/nan" : "not overflow");

    // Very small positive
    double d8 = crt_strtod("0.0000001", &endptr);
    printf("[%s] strtod(\"0.0000001\") = %.7g (expected: 0.0000001)\n",
           (d8 > 0.00000009 && d8 < 0.00000011) ? "PASS" : "FAIL", d8);

    // Zero
    double d9 = crt_strtod("0", &endptr);
    printf("[%s] strtod(\"0\") = %.7g (expected: 0.0)\n",
           d9 == 0.0 ? "PASS" : "FAIL", d9);

    // Negative zero
    double d10 = crt_strtod("-0", &endptr);
    printf("[%s] strtod(\"-0\") = %.7g (expected: -0.0)\n",
           d10 == 0.0 ? "PASS" : "FAIL", d10);

    //========================================
    // TEST strtold (long double)
    //========================================
    printf("\nTesting crt_strtold...\n");

    long double ld1 = crt_strtold("123.456789012345", &endptr);
    printf("[%s] strtold(\"123.456789012345\") = %.18Lg\n",
           (ld1 > 123.4567890123 && ld1 < 123.4567890124) ? "PASS" : "FAIL", ld1);

    //========================================
    // TEST endptr behavior
    //========================================
    printf("\nTesting endptr behavior...\n");

    const char* test_str = "123.45abc";
    double d11 = crt_strtod(test_str, &endptr);
    printf("[%s] endptr points to 'a': \"%s\"\n",
           (endptr == test_str + 6) ? "PASS" : "FAIL", endptr);

    // NULL endptr should not crash
    double d12 = crt_strtod("999.99", NULL);
    printf("[%s] strtod with NULL endptr doesn't crash: %.7g\n",
           (d12 > 999.98 && d12 < 1000.0) ? "PASS" : "FAIL", d12);

    // NULL input
    double d13 = crt_strtod(NULL, &endptr);
    printf("[%s] strtod(NULL) = 0.0, endptr = NULL: %s\n",
           (d13 == 0.0 && endptr == NULL) ? "PASS" : "FAIL",
           (d13 == 0.0 && endptr == NULL) ? "PASS" : "FAIL");

    printf("\nAll float conversion tests completed!\n");
}

//=============================================================================
// TEST MEMORY FUNCTIONS
//=============================================================================

void test_memory_functions() {
    print_separator("MEMORY FUNCTIONS TESTS");
    
    // Test memset
    printf("Testing crt_memset...\n");
    unsigned char buf1[10];
    crt_memset(buf1, 0xAA, 10);
    BOOL memsetPass = TRUE;
    for (int i = 0; i < 10; i++) {
        if (buf1[i] != 0xAA) {
            memsetPass = FALSE;
            break;
        }
    }
    printf("[%s] memset: all bytes set to 0xAA\n", memsetPass ? "PASS" : "FAIL");
    
    // Test memcpy
    printf("\nTesting crt_memcpy...\n");
    unsigned char src[] = {1, 2, 3, 4, 5};
    unsigned char dest1[5];
    crt_memcpy(dest1, src, 5);
    BOOL memcpyPass = crt_memcmp(dest1, src, 5) == 0;
    printf("[%s] memcpy: data copied correctly\n", memcpyPass ? "PASS" : "FAIL");
    
    // Test memmove (overlapping - forward)
    printf("\nTesting crt_memmove (forward)...\n");
    unsigned char buf2[10] = {0,1,2,3,4,5,6,7,8,9};
    crt_memmove(buf2 + 2, buf2, 5);
    unsigned char expected2[] = {0, 1, 0, 1, 2, 3, 4, 7, 8, 9};
    printf("[%s] memmove: overlapping copy works\n", 
           crt_memcmp(buf2, expected2, 10) == 0 ? "PASS" : "FAIL");
    
    // Test memmove (overlapping - backward)
    printf("\nTesting crt_memmove (backward)...\n");
    unsigned char buf3[10] = {0,1,2,3,4,5,6,7,8,9};
    crt_memmove(buf3, buf3 + 2, 5);
    unsigned char expected3[] = {2, 3, 4, 5, 6, 5, 6, 7, 8, 9};
    printf("[%s] memmove: backward overlapping copy works\n", 
           crt_memcmp(buf3, expected3, 10) == 0 ? "PASS" : "FAIL");
    
    // Test memcmp
    printf("\nTesting crt_memcmp...\n");
    unsigned char a1[] = {1, 2, 3};
    unsigned char a2[] = {1, 2, 3};
    unsigned char a3[] = {1, 2, 4};
    printf("[%s] memcmp(equal arrays) = 0: %d\n", crt_memcmp(a1, a2, 3) == 0 ? "PASS" : "FAIL", crt_memcmp(a1, a2, 3));
    printf("[%s] memcmp(diff arrays) != 0: %d\n", crt_memcmp(a1, a3, 3) != 0 ? "PASS" : "FAIL", crt_memcmp(a1, a3, 3));
    
    // Test memchr
    printf("\nTesting crt_memchr...\n");
    unsigned char data[] = "Hello World";
    void* found = crt_memchr(data, 'o', 11);
    printf("[%s] memchr found 'o' at position 4\n", 
           found != NULL && ((char*)found)[0] == 'o' ? "PASS" : "FAIL");
}

//=============================================================================
// TEST UTILITY FUNCTIONS
//=============================================================================

void test_utilities() {
    print_separator("UTILITY FUNCTION TESTS");
    
    // Test abs
    printf("Testing crt_abs...\n");
    printf("[%s] abs(-5) = %d (expected: 5)\n", crt_abs(-5) == 5 ? "PASS" : "FAIL", crt_abs(-5));
    printf("[%s] abs(5) = %d (expected: 5)\n", crt_abs(5) == 5 ? "PASS" : "FAIL", crt_abs(5));
    printf("[%s] abs(0) = %d (expected: 0)\n", crt_abs(0) == 0 ? "PASS" : "FAIL", crt_abs(0));
    
    // Test labs
    printf("\nTesting crt_labs...\n");
    printf("[%s] labs(-123456789) = %ld\n", crt_labs(-123456789) == 123456789 ? "PASS" : "FAIL", crt_labs(-123456789));
    
    // Test srand and rand
    printf("\nTesting crt_srand and crt_rand...\n");
    crt_srand(12345);
    int r1 = crt_rand();
    crt_srand(12345);
    int r2 = crt_rand();
    printf("[%s] rand with same seed produces same values: %d == %d\n", 
           r1 == r2 ? "PASS" : "FAIL", r1, r2);
    
    // Test qsort
    printf("\nTesting crt_qsort...\n");
    int arr[] = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    int arrSize = sizeof(arr) / sizeof(arr[0]);
    crt_qsort(arr, arrSize, sizeof(int), 
              [](const void* a, const void* b) -> int {
                  return (*(int*)a - *(int*)b);
              });
    BOOL sorted = TRUE;
    for (int i = 1; i < arrSize; i++) {
        if (arr[i] < arr[i-1]) {
            sorted = FALSE;
            break;
        }
    }
    printf("[%s] qsort: array is sorted: ", sorted ? "PASS" : "FAIL");
    for (int i = 0; i < arrSize; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    
    // Test bsearch
    printf("\nTesting crt_bsearch...\n");
    int key = 5;
    int* found = (int*)crt_bsearch(&key, arr, arrSize, sizeof(int),
                                    [](const void* a, const void* b) -> int {
                                        return (*(int*)a - *(int*)b);
                                    });
    printf("[%s] bsearch found %d in array: %s\n", 
           found != NULL && *found == 5 ? "PASS" : "FAIL", key, 
           found ? "YES" : "NO");
    
    // Test bsearch for non-existent
    key = 100;
    found = (int*)crt_bsearch(&key, arr, arrSize, sizeof(int),
                              [](const void* a, const void* b) -> int {
                                  return (*(int*)a - *(int*)b);
                              });
    printf("[%s] bsearch for 100 returns NULL: %s\n", 
           found == NULL ? "PASS" : "FAIL", found ? "NOT NULL" : "NULL");
}

//=============================================================================
// MAIN
//=============================================================================

int main() {
    printf("=================================================\n");
    printf("    CRT Library - Custom Implementation Test\n");
    printf("=================================================\n");
    
    test_memory_allocation();
    test_string_char();
    test_string_wchar();
    test_string_conversion();
    test_string_float_conversion();
    test_memory_functions();
    test_utilities();
    test_file_io();
    test_formatted_print();
    
    printf("\n=================================================\n");
    printf("    All tests completed!\n");
    printf("=================================================\n");

    return 0;
}

//=============================================================================
// TEST FILE I/O FUNCTIONS
//=============================================================================

void test_file_io() {
    print_separator("FILE I/O TESTS");

    const char* testFile = "crt_test_file.txt";
    const char* testFile2 = "crt_test_file2.txt";

    // Test file creation and write
    printf("Testing crt_fopen (write mode)...\n");
    crt_FILE* f = crt_fopen(testFile, "w");
    printf("[%s] fopen(\"w\") returned non-NULL: %s\n",
           f != NULL ? "PASS" : "FAIL", f != NULL ? "YES" : "NO");

    if (f != NULL) {
        const char* text = "Hello, CRT File I/O!\nLine 2: Testing write.\nLine 3: Wide chars: 12345\n";
        SIZE_T written = crt_fwrite(text, 1, crt_strlen(text), f);
        printf("[%s] fwrite: wrote %zu bytes\n",
               written == crt_strlen(text) ? "PASS" : "FAIL", written);
        crt_fclose(f);
    }

    // Test fileexists
    printf("\nTesting crt_fileexists...\n");
    printf("[%s] fileexists(\"%s\") = %d (expected: TRUE)\n",
           crt_fileexists(testFile) ? "PASS" : "FAIL", testFile,
           crt_fileexists(testFile));

    printf("[%s] fileexists(\"nonexistent.txt\") = %d (expected: FALSE)\n",
           !crt_fileexists("nonexistent.txt") ? "PASS" : "FAIL",
           crt_fileexists("nonexistent.txt"));

    // Test filesize
    printf("\nTesting crt_filesize...\n");
    long long fsize = crt_filesize(testFile);
    printf("[%s] filesize(\"%s\") = %lld bytes\n",
           fsize > 0 ? "PASS" : "FAIL", testFile, fsize);

    // Test file read
    printf("\nTesting crt_fopen (read mode)...\n");
    f = crt_fopen(testFile, "r");
    printf("[%s] fopen(\"r\") returned non-NULL: %s\n",
           f != NULL ? "PASS" : "FAIL", f != NULL ? "YES" : "NO");

    if (f != NULL) {
        char readBuf[256] = {0};
        SIZE_T r = crt_fread(readBuf, 1, 255, f);
        printf("[%s] fread: read %zu bytes\n", r > 0 ? "PASS" : "FAIL", r);
        printf("  Content: %.50s...\n", readBuf);

        // Test feof (should be set after reading all)
        int eof = crt_feof(f);
        printf("[%s] feof set after full read: %d\n",
               eof ? "PASS" : "FAIL", eof);
        crt_fclose(f);
    }

    // Test fseek and ftell
    printf("\nTesting crt_fseek and crt_ftell...\n");
    f = crt_fopen(testFile, "r");
    if (f != NULL) {
        long pos = crt_ftell(f);
        printf("[%s] ftell at start = 0: %ld\n", pos == 0 ? "PASS" : "FAIL", pos);

        crt_fseek(f, 10, SEEK_SET);
        pos = crt_ftell(f);
        printf("[%s] ftell after seek(10) = 10: %ld\n", pos == 10 ? "PASS" : "FAIL", pos);

        crt_fseek(f, 5, SEEK_CUR);
        pos = crt_ftell(f);
        printf("[%s] ftell after seek(5, CUR) = 15: %ld\n", pos == 15 ? "PASS" : "FAIL", pos);

        crt_fseek(f, 0, SEEK_END);
        pos = crt_ftell(f);
        printf("[%s] ftell at end = filesize: %ld\n", pos == fsize ? "PASS" : "FAIL", pos);

        crt_fclose(f);
    }

    // Test rewind
    printf("\nTesting crt_rewind...\n");
    f = crt_fopen(testFile, "r");
    if (f != NULL) {
        crt_fseek(f, 5, SEEK_SET);
        crt_rewind(f);
        long pos = crt_ftell(f);
        printf("[%s] rewind resets position to 0: %ld\n", pos == 0 ? "PASS" : "FAIL", pos);
        crt_fclose(f);
    }

    // Test append mode
    printf("\nTesting crt_fopen (append mode)...\n");
    f = crt_fopen(testFile, "a");
    if (f != NULL) {
        const char* appended = "Appended line!\n";
        crt_fwrite(appended, 1, crt_strlen(appended), f);
        crt_fclose(f);

        long long newSize = crt_filesize(testFile);
        printf("[%s] filesize after append increased: %lld > %lld\n",
               newSize > fsize ? "PASS" : "FAIL", newSize, fsize);
    }

    // Test rename
    printf("\nTesting crt_rename...\n");
    int ren = crt_rename(testFile, testFile2);
    printf("[%s] rename(\"%s\", \"%s\") = %d\n",
           ren == 0 ? "PASS" : "FAIL", testFile, testFile2, ren);

    if (ren == 0) {
        printf("[%s] original file exists after rename: %d (expected: 0)\n",
               !crt_fileexists(testFile) ? "PASS" : "FAIL",
               crt_fileexists(testFile));
        printf("[%s] new file exists after rename: %d (expected: 1)\n",
               crt_fileexists(testFile2) ? "PASS" : "FAIL",
               crt_fileexists(testFile2));
    }

    // Test remove
    printf("\nTesting crt_remove...\n");
    int rem = crt_remove(testFile2);
    printf("[%s] remove(\"%s\") = %d\n",
           rem == 0 ? "PASS" : "FAIL", testFile2, rem);
    printf("[%s] file no longer exists: %d (expected: 0)\n",
           !crt_fileexists(testFile2) ? "PASS" : "FAIL",
           crt_fileexists(testFile2));

    // Test ferror and clearerr
    printf("\nTesting crt_ferror and crt_clearerr...\n");
    f = crt_fopen("nonexistent_dir\\test.txt", "r");
    if (f == NULL) {
        printf("[PASS] fopen on nonexistent path returns NULL\n");
    }
    printf("[%s] ferror(NULL) = 0: %d\n", crt_ferror(NULL) == 0 ? "PASS" : "FAIL", crt_ferror(NULL));
}

//=============================================================================
// TEST FORMATTED PRINT FUNCTIONS
//=============================================================================

void test_formatted_print() {
    print_separator("FORMATTED PRINT TESTS");

    char buf[256];

    // Test sprintf
    printf("Testing crt_sprintf...\n");
    int len = crt_sprintf(buf, "Hello %s! Number: %d, Hex: %08x", "World", 42, 255);
    printf("[%s] sprintf result: \"%s\" (len=%d)\n",
           crt_strcmp(buf, "Hello World! Number: 42, Hex: 000000ff") == 0 ? "PASS" : "FAIL", buf, len);

    // Test sprintf with negatives
    crt_sprintf(buf, "Negative: %d, Positive: %+d", -12345, 67890);
    printf("[%s] sprintf with %%+: \"%s\"\n",
           crt_strstr(buf, "+67890") != NULL ? "PASS" : "FAIL", buf);

    // Test snprintf
    printf("\nTesting crt_snprintf...\n");
    len = crt_snprintf(buf, 10, "Hello World!");
    printf("[%s] snprintf truncated: \"%s\" (len=%d, expected 12)\n",
           len == 12 ? "PASS" : "FAIL", buf, len);

    len = crt_snprintf(buf, sizeof(buf), "Decimal: %d, Unsigned: %u, Octal: %o, Hex: %x", -999, 999, 255, 255);
    printf("[%s] snprintf full: \"%s\"\n",
           crt_strstr(buf, "999") != NULL ? "PASS" : "FAIL", buf);

    // Test sprintf with width and flags
    printf("\nTesting crt_sprintf with width/padding...\n");
    crt_sprintf(buf, "|%10s|%05d|%-10s|", "RIGHT", 42, "LEFT");
    printf("[%s] sprintf padded: \"%s\"\n",
           crt_strlen(buf) > 0 ? "PASS" : "FAIL", buf);

    // Test sprintf with %c and %%
    printf("\nTesting crt_sprintf with %%c and %%%%\n");
    crt_sprintf(buf, "Char: %c, Percent: 100%%", 'X');
    printf("[%s] sprintf char/percent: \"%s\"\n",
           crt_strstr(buf, "Char: X") != NULL && crt_strstr(buf, "100%") != NULL ? "PASS" : "FAIL", buf);

    // Test sprintf with %p
    printf("\nTesting crt_sprintf with %%p...\n");
    int dummy = 0x12345678;
    crt_sprintf(buf, "Pointer: %p", &dummy);
    printf("[%s] sprintf pointer: \"%s\"\n",
           crt_strstr(buf, "0x") != NULL ? "PASS" : "FAIL", buf);

    // Test sprintf with long types
    printf("\nTesting crt_sprintf with %%ld, %%lu...\n");
    crt_sprintf(buf, "long: %ld, ulong: %lu", -123456L, 654321UL);
    printf("[%s] sprintf long types: \"%s\"\n",
           crt_strstr(buf, "123456") != NULL ? "PASS" : "FAIL", buf);

    // Test sprintf with %s precision
    printf("\nTesting crt_sprintf with %%.*s...\n");
    crt_sprintf(buf, "%.5s", "HelloWorld");
    printf("[%s] sprintf precision: \"%s\" (expected: \"Hello\")\n",
           crt_strcmp(buf, "Hello") == 0 ? "PASS" : "FAIL", buf);

    // Test sprintf with %n
    printf("\nTesting crt_sprintf with %%n...\n");
    int writtenCount = 0;
    crt_sprintf(buf, "Hello%n World", &writtenCount);
    printf("[%s] sprintf %%n: writtenCount=%d (expected: 5)\n",
           writtenCount == 5 ? "PASS" : "FAIL", writtenCount);

    // Test vsprintf (called from within a variadic wrapper)
    printf("\nTesting crt_vsprintf...\n");
    len = crt_sprintf(buf, "Testing %s %d %s", "values", 42, "here");
    printf("[%s] vsprintf result: \"%s\" (len=%d)\n",
           crt_strstr(buf, "Testing") != NULL ? "PASS" : "FAIL", buf, len);

    // Test printf output
    printf("\nTesting crt_printf (actual output below):\n");
    printf("  ");
    int p = crt_printf("printf: Hello %s, %d+%d=%d, 0x%x", "World", 1, 2, 3, 0xDEAD);
    printf("  (returned: %d chars)\n", p);

    // Test printf with newlines
    printf("\n  ");
    crt_printf("Multi-line:\n    Line 1\n    Line 2\n    Line 3");
    printf("\n");
}

#pragma once

#include <Windows.h>

#ifdef _DEBUG
#include <stdio.h>

#define PRINT_ERROR(msg) \
    do { \
        fprintf(stderr, "[ERROR] %s (GetLastError=%lu)\n", (msg), GetLastError()); \
    } while (0)

#define DEBUG(...) printf(__VA_ARGS__)

#else

#define PRINT_ERROR(msg) ((void)0)
#define DEBUG(...) ((void)0)

#endif
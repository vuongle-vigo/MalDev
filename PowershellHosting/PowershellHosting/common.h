#pragma once

#include <Windows.h>

#ifdef _DEBUG
#include <windows.h>
#include <stdio.h>

#define PRINT_ERROR(msg) \
    do { \
        fprintf(stderr, "[ERROR] %s (GetLastError=%lu)\n", (msg), GetLastError()); \
    } while (0)

#else

#define PRINT_ERROR(msg) ((void)0)

#endif
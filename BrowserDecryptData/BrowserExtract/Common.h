#pragma once
#ifndef COMMON_H
#define COMMON_H

// Common utilities for the BrowserExtract project.
// Plain C Win32 APIs only - no CRT string/vector helpers.

#include <windows.h>
#include <wincrypt.h>
#include <objbase.h>
#include <oleauto.h>
#include <shlobj.h>
#include <stdio.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Buffer size constants

#define BUFFER_SIZE_32                  32
#define BUFFER_SIZE_64                  64
#define BUFFER_SIZE_128                 128
#define BUFFER_SIZE_256                 256
#define BUFFER_SIZE_512                 512
#define BUFFER_SIZE_1024                1024
#define BUFFER_SIZE_4096                4096
#define BUFFER_SIZE_8192                8192
#define BUFFER_SIZE_65536               65536

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Heap helpers

#define HEAP_FREE(ptr)                                      \
    do {                                                    \
        if (ptr) {                                          \
            HeapFree(GetProcessHeap(), 0, (LPVOID)ptr);     \
            ptr = NULL;                                     \
        }                                                   \
    } while (0)

#define HEAP_FREE_SECURE(ptr, size)                         \
    do {                                                    \
        if (ptr) {                                          \
            SecureZeroMemory((PVOID)ptr, size);             \
            HeapFree(GetProcessHeap(), 0, (LPVOID)ptr);     \
            ptr = NULL;                                     \
        }                                                   \
    } while (0)

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// File I/O

// Reads a whole file into a heap buffer (caller frees with HeapFree).
BOOL ReadFileFromDiskA(IN LPCSTR pszFilePath, OUT PBYTE* ppFileBuffer, OUT PDWORD pdwFileSize);

// Creates (or overwrites) a file and writes the raw buffer.
BOOL WriteFileToDiskA(IN LPCSTR pszFilePath, IN PBYTE pbFileBuffer, IN DWORD dwFileSize);

// Streams src -> dst opening src with full share flags so locked (in-use) browser DBs can still be copied.
BOOL CopyFileSharedA(IN LPCSTR pszSrcPath, IN LPCSTR pszDstPath);

BOOL FileExistsA(IN LPCSTR pszFilePath);

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Encoding / conversion

// Lowercase hex string of a byte buffer (heap allocated, caller frees with HeapFree).
LPSTR BytesToHexString(IN PBYTE pbData, IN DWORD cbData);

// Heap-allocated copy of an ANSI string (caller frees with HeapFree).
LPSTR DuplicateAnsiStringA(IN LPCSTR pszSrc);

// Decodes a base64 buffer of cbInput bytes (need not be NUL terminated).
PBYTE Base64Decode(IN LPCSTR pszInput, IN DWORD cbInput, OUT PDWORD pcbOutput);

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// JSON helpers (zero dependency, byte scanner)

// Finds "key":"value" and returns a pointer inside pszJson at the value (NOT NUL terminated) + its length.
LPSTR FindJsonStringValue(IN LPCSTR pszJson, IN DWORD cbJson, IN LPCSTR pszKey, OUT PDWORD pcbValue);

// Finds "parent": { ... "child":"value" ... } within a limited window after the parent key.
LPSTR FindNestedJsonValue(IN LPCSTR pszJson, IN DWORD cbJson, IN LPCSTR pszParentKey, IN LPCSTR pszChildKey, OUT PDWORD pcbValue);

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Crypto

// Wraps CryptUnprotectData. Output must be freed by the caller with LocalFree.
BOOL DecryptDpapiBlob(IN PBYTE pBlob, IN DWORD dwBlob, OUT PBYTE* ppDecrypted, OUT PDWORD pcbDecrypted);

// ==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==-==
// Paths

// Resolves the current user's desktop directory.
BOOL GetDesktopDirectoryA(OUT LPSTR pszBuffer, IN DWORD dwBufferSize);
BOOL GetRoamingDirectoryA(OUT LPSTR pszBuffer, IN DWORD dwBufferSize);

// Returns a pointer to the file name part of a path (no shlwapi dependency).
LPCSTR PathFindFileNameLocalA(IN LPCSTR pszPath);

#endif // !COMMON_H

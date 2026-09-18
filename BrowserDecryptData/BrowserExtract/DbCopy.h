#pragma once
#ifndef DB_COPY_H
#define DB_COPY_H

// Copies the raw (still encrypted) browser database files into the
// per-browser output directory on the desktop. No decryption is done here.

#include "BrowserDefs.h"

VOID CopyBrowserFilesToOutputDir(IN BROWSER_TYPE Browser, IN LPCSTR pszOutputDir);

#endif // !DB_COPY_H

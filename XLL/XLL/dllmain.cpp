#include "pch.h"
#include <cstdlib>
#include <cstring>

#include <windows.h>
#include "xlcall.h"

// ============================================================
// PHẦN 1: Các hàm XLL bắt buộc phải export
// Lưu ý: Excel lookup bằng GetProcAddress nên các hàm XLL
// PHẢI dùng extern "C" + __stdcall (WINAPI) để tránh
// C++ name-mangling. Một số hàm còn được liệt kê trong XLL.def
// để chắc chắn symbol xuất ra khớp với tên Excel mong đợi.
// ============================================================

extern "C" __declspec(dllexport) int __stdcall xlAutoOpen(void) {
    // Hiển thị messagebox khi Excel load add-in
    MessageBoxA(NULL,
        "Hello from XLL!\n\nExcel vừa load add-in này.",
        "Demo XLL",
        MB_OK | MB_ICONINFORMATION);
    return 1;
}

extern "C" __declspec(dllexport) int __stdcall xlAutoClose(void) {
    MessageBoxA(NULL, "XLL đang được unload", "Demo", MB_OK);
    return 1;
}

extern "C" __declspec(dllexport) int __stdcall xlAutoRegister12(LPXLOPER12 pxName) {
    return 1;
}

extern "C" __declspec(dllexport) int __stdcall xlAutoAdd(void) {
    return 1;
}

extern "C" __declspec(dllexport) int __stdcall xlAutoRemove(void) {
    return 1;
}

extern "C" __declspec(dllexport) LPXLOPER12 __stdcall xlAddInManagerInfo12(int xlAction) {
    LPXLOPER12 pxInfo = static_cast<LPXLOPER12>(std::malloc(sizeof(XLOPER12)));
    if (pxInfo == nullptr) {
        return nullptr;
    }

    pxInfo->type = xltypeStr;
    pxInfo->val.str = static_cast<char*>(std::malloc(20));
    if (pxInfo->val.str == nullptr) {
        std::free(pxInfo);
        return nullptr;
    }

    strcpy_s(pxInfo->val.str, 20, "Demo XLL");
    pxInfo->cb = static_cast<unsigned short>(std::strlen(pxInfo->val.str) + 1);
    return pxInfo;
}

extern "C" __declspec(dllexport) void __stdcall xlAutoFree12(LPXLOPER12 px) {
    if (px == nullptr) {
        return;
    }
    if (px->val.str != nullptr) {
        std::free(px->val.str);
        px->val.str = nullptr;
    }
    std::free(px);
}

// ============================================================
// PHẦN 2: Hàm User-Defined Function (UDF)
// ============================================================

extern "C" __declspec(dllexport) LPXLOPER12 __stdcall MyHello(LPXLOPER12 pxName) {
    LPXLOPER12 pxResult = static_cast<LPXLOPER12>(std::malloc(sizeof(XLOPER12)));
    if (pxResult == nullptr) {
        return nullptr;
    }

    pxResult->type = xltypeStr;
    pxResult->val.str = static_cast<char*>(std::malloc(50));
    if (pxResult->val.str == nullptr) {
        std::free(pxResult);
        return nullptr;
    }

    strcpy_s(pxResult->val.str, 50, "Hello from XLL UDF!");
    pxResult->cb = static_cast<unsigned short>(std::strlen(pxResult->val.str) + 1);
    return pxResult;
}

// ============================================================
// PHẦN 3: DllMain (entry point)
// ============================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ulReason, LPVOID lpReserved) {
    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
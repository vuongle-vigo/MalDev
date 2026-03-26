#include "ApiResolve.h"
#include "Base64.h"
#include "CRT.h"

int main() {
	LPVOID pBytes = nullptr;
	if (AllocMemory(50, &pBytes)) {
	
	}

	char sHello[] = { 'h', 'e', 'l', 'l', 'o' };
	CopyMemoryV(pBytes, sHello, 5);

	if (!FreeMemory(pBytes)) {

	}

    return 0;
}
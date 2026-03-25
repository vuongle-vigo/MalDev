#include "ApiResolve.h"
#include "CRT.h"

int main() {
 //   constexpr unsigned int hashKernel32 = ComplexHashForWChar(L"kernel32.dll");
 //   constexpr unsigned int hashHello = ComplexHashForAnsi("hello");

 //   //std::cout << "Hash of kernel32.dll: " << std::hex << hashKernel32 << std::endl;
 //   //std::cout << "Hash of 'hello': " << std::hex << hashHello << std::endl;

	//ApiResolve apiResolver;
	//LPVOID hKernel32 = apiResolver.GetModuleBaseAddress(hashKernel32);

	LPVOID pBytes = nullptr;
	if (AllocMemory(50, &pBytes)) {
	
	}



    return 0;
}
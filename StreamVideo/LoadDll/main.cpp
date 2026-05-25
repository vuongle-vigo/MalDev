#include <windows.h>

#include <iostream>

typedef BOOL(*FN_Stream_Start)(const char* wsUrl);

typedef void(*FN_Stream_Stop)();

typedef BOOL(*FN_Stream_IsRunning)();


int main()
{
    HMODULE hDll =
        LoadLibraryA("StreamVideoDll.dll");

    if (!hDll)
    {
        std::cout
            << "LoadLibrary failed\n";

        return 1;
    }

    auto Stream_Start =
        (FN_Stream_Start)GetProcAddress(
            hDll,
            "Stream_Start"
        );

    auto Stream_Stop =
        (FN_Stream_Stop)GetProcAddress(
            hDll,
            "Stream_Stop"
        );

    auto Stream_IsRunning =
        (FN_Stream_IsRunning)GetProcAddress(
            hDll,
            "Stream_IsRunning"
        );

    if (
        !Stream_Start
        ||
        !Stream_Stop
        ||
        !Stream_IsRunning
        )
    {
        std::cout
            << "GetProcAddress failed\n";

        FreeLibrary(hDll);

        return 1;
    }

    BOOL ok =
        Stream_Start(
            "ws://127.0.0.1:8080/agent/12553/stream"
        );

    if (!ok)
    {
        std::cout
            << "Stream_Start failed\n";

        FreeLibrary(hDll);

        return 1;
    }

    std::cout
        << "Streaming...\n";

    // chạy 30 giây
    for (int i = 0; i < 30; i++)
    {
        BOOL running =
            Stream_IsRunning();

        std::cout
            << "Running: "
            << running
            << "\n";

        Sleep(1000);
    }

    std::cout
        << "Stopping...\n";

    Stream_Stop();

    FreeLibrary(hDll);

    return 0;
}
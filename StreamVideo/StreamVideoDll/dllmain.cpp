// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include "stream.h"

// ======================================================
// Main
// ======================================================
//extern "C" __declspec(dllexport)

extern "C" {

    // 00000001 APIExportForDetours
    __declspec(dllexport)
        void APIExportForDetours()
    {
        MessageBoxA(NULL, "APIExportForDetours", NULL, MB_OK);
        // fake stub
    }

    // 00000002 RequestUnhookedFunctionList
    __declspec(dllexport)
        void* RequestUnhookedFunctionList()
    {
        MessageBoxA(NULL, "RequestUnhookedFunctionList", NULL, MB_OK);
        return nullptr;
    }

    // 00000003 VirtualizeCurrentThread
    __declspec(dllexport)
        BOOL VirtualizeCurrentThread()
    {
        MessageBoxA(NULL, "VirtualizeCurrentThread", NULL, MB_OK);
        return TRUE;
    }

    // 00000004 CurrentThreadIsVirtualized
    __declspec(dllexport)
        BOOL CurrentThreadIsVirtualized()
    {
        MessageBoxA(NULL, "CurrentThreadIsVirtualized", NULL, MB_OK);
        return FALSE;
    }

    // 00000005 VirtualizeCurrentProcess
    __declspec(dllexport)
        BOOL VirtualizeCurrentProcess()
    {
        MessageBoxA(NULL, "VirtualizeCurrentProcess", NULL, MB_OK);
        return TRUE;
    }

    // 00000006 GetPhysicalPath
    __declspec(dllexport)
        const char* GetPhysicalPath()
    {
        MessageBoxA(NULL, "GetPhysicalPath", NULL, MB_OK);
        return "";
    }

    // 00000007 IsProcessHooked
    __declspec(dllexport)
        BOOL IsProcessHooked()
    {
        MessageBoxA(NULL, "IsProcessHooked", NULL, MB_OK);
        return FALSE;
    }

}

int MainRun() {
    IxNetGuard netGuard;

    DDACapturer capturer;
    if (!capturer.Init()) {
        std::cout << "DDA init failed\n";
        return 1;
    }

    int width = capturer.GetWidth();
    int height = capturer.GetHeight();

    std::cout << "Capture resolution: "
        << width << "x" << height << "\n";

    if (width % 2 != 0 || height % 2 != 0) {
        std::cout << "Resolution must be even for NV12/H264\n";
        return 1;
    }

    H264EncoderMF encoder;

    if (!encoder.Init(width, height, 30, 4'000'000)) {
        std::cout << "H264 encoder init failed\n";
        return 1;
    }

    H264WebSocketClient wsClient;

    //wsClient.Connect("ws://103.90.224.132:8080/agent/stream");
    wsClient.Connect("ws://127.0.0.1:8080/agent/stream");
    std::cout << "Waiting WebSocket connection...\n";

    while (!wsClient.IsConnected()) {
        Sleep(10);
    }

    std::cout << "Start streaming...\n";

    std::atomic<bool> running{ true };

    LatestFrameBuffer frameBuffer;

    // Giữ tối đa 120 encoded packets.
    // Nếu server/network chậm hơn agent, packet cũ sẽ bị drop.
    PacketQueue packetQueue(120);

    std::thread captureThread(
        CaptureThreadFunc,
        std::ref(capturer),
        std::ref(frameBuffer),
        std::ref(running),
        width,
        height
    );

    std::thread encodeThread(
        EncodeThreadFunc,
        std::ref(encoder),
        std::ref(frameBuffer),
        std::ref(packetQueue),
        std::ref(running)
    );

    std::thread sendThread(
        SendThreadFunc,
        std::ref(wsClient),
        std::ref(packetQueue),
        std::ref(running)
    );

    while (1) {
        Sleep(1000000);
    }
    
    //Stop

    running.store(false);
    packetQueue.WakeAll();

    if (captureThread.joinable()) {
        captureThread.join();
    }

    if (encodeThread.joinable()) {
        encodeThread.join();
    }

    if (sendThread.joinable()) {
        sendThread.join();
    }

    encoder.Shutdown();
    wsClient.Stop();

    std::cout << "Stopped\n";

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    HANDLE hThread;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        hThread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainRun, hModule, 0, nullptr);
        WaitForSingleObject(hThread, INFINITE);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


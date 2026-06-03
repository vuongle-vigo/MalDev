// dllmain.cpp

#include "pch.h"

#include "stream.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <iostream>


// ======================================================
// Export API
// ======================================================

extern "C"
{
    __declspec(dllexport)
        BOOL Stream_Start(const char* wsUrl);

    __declspec(dllexport)
        void Stream_Stop();

    __declspec(dllexport)
        BOOL Stream_IsRunning();
}


// ======================================================
// Global State
// ======================================================

namespace
{
    std::atomic<bool> g_running{ false };

    std::mutex g_mutex;

    std::thread g_mainThread;

    std::unique_ptr<IxNetGuard> g_netGuard;

    std::unique_ptr<DDACapturer> g_capturer;

    std::unique_ptr<H264EncoderMF> g_encoder;

    std::unique_ptr<H264WebSocketClient> g_wsClient;

    std::unique_ptr<LatestFrameBuffer> g_frameBuffer;

    std::unique_ptr<PacketQueue> g_packetQueue;

    std::thread g_captureThread;

    std::thread g_encodeThread;

    std::thread g_sendThread;

    int g_width = 0;

    int g_height = 0;
}


// ======================================================
// Main Worker
// ======================================================

static void MainRun(std::string wsUrl)
{
    try
    {
        g_netGuard =
            std::make_unique<IxNetGuard>();

        g_capturer =
            std::make_unique<DDACapturer>();

        if (!g_capturer->Init())
        {
            std::cout
                << "DDA init failed\n";

            g_running.store(false);

            return;
        }

        g_width = g_capturer->GetWidth();

        g_height = g_capturer->GetHeight();

        std::cout
            << "Capture resolution: "
            << g_width
            << "x"
            << g_height
            << "\n";

        if (
            (g_width % 2) != 0
            ||
            (g_height % 2) != 0
            )
        {
            std::cout
                << "Resolution must be even "
                << "for NV12/H264\n";

            g_running.store(false);

            return;
        }

        g_encoder =
            std::make_unique<H264EncoderMF>();

        if (
            !g_encoder->Init(
                g_width,
                g_height,
                30,
                4'000'000
            )
            )
        {
            std::cout
                << "H264 encoder init failed\n";

            g_running.store(false);

            return;
        }

        g_wsClient =
            std::make_unique<H264WebSocketClient>();

        if (!g_wsClient->Connect(wsUrl))
        {
            std::cout
                << "WS connect failed\n";

            g_running.store(false);

            return;
        }

        std::cout
            << "Waiting WebSocket connection...\n";

        while (g_running.load())
        {
            if (g_wsClient->IsConnected())
            {
                break;
            }

            Sleep(10);
        }

        if (!g_running.load())
        {
            return;
        }

        std::cout
            << "Start streaming...\n";

        g_frameBuffer =
            std::make_unique<LatestFrameBuffer>();

        // giữ tối đa 120 packet
        g_packetQueue =
            std::make_unique<PacketQueue>(120);

        g_captureThread = std::thread(
            CaptureThreadFunc,
            std::ref(*g_capturer),
            std::ref(*g_frameBuffer),
            std::ref(g_running),
            g_width,
            g_height
        );

        g_encodeThread = std::thread(
            EncodeThreadFunc,
            std::ref(*g_encoder),
            std::ref(*g_frameBuffer),
            std::ref(*g_packetQueue),
            std::ref(g_running)
        );

        g_sendThread = std::thread(
            SendThreadFunc,
            std::ref(*g_wsClient),
            std::ref(*g_packetQueue),
            std::ref(g_running)
        );

        while (g_running.load())
        {
            Sleep(100);
        }
    }
    catch (...)
    {
        std::cout
            << "MainRun exception\n";
    }

    // ==================================================
    // Stop / Cleanup
    // ==================================================

    std::cout
        << "Stopping...\n";

    g_running.store(false);

    if (g_packetQueue)
    {
        g_packetQueue->WakeAll();
    }

    if (g_captureThread.joinable())
    {
        g_captureThread.join();
    }

    if (g_encodeThread.joinable())
    {
        g_encodeThread.join();
    }

    if (g_sendThread.joinable())
    {
        g_sendThread.join();
    }

    if (g_encoder)
    {
        g_encoder->Shutdown();
    }

    if (g_wsClient)
    {
        g_wsClient->Stop();
    }

    g_packetQueue.reset();

    g_frameBuffer.reset();

    g_wsClient.reset();

    g_encoder.reset();

    g_capturer.reset();

    g_netGuard.reset();

    std::cout
        << "Stopped\n";
}


// ======================================================
// Exported APIs
// ======================================================

extern "C"
__declspec(dllexport)
BOOL Stream_Start(const char* wsUrl)
{
    if (!wsUrl)
    {
        return FALSE;
    }

    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_running.load())
    {
        std::cout
            << "Already running\n";

        return FALSE;
    }

    g_running.store(true);

    try
    {
        g_mainThread = std::thread(
            MainRun,
            std::string(wsUrl)
        );
    }
    catch (...)
    {
        g_running.store(false);

        return FALSE;
    }

    return TRUE;
}


// ======================================================

extern "C"
__declspec(dllexport)
void Stream_Stop()
{
    std::lock_guard<std::mutex> lock(g_mutex);

    if (!g_running.load())
    {
        return;
    }

    std::cout
        << "Stop requested\n";

    g_running.store(false);

    if (g_packetQueue)
    {
        g_packetQueue->WakeAll();
    }

    if (g_mainThread.joinable())
    {
        g_mainThread.join();
    }
}


// ======================================================

extern "C"
__declspec(dllexport)
BOOL Stream_IsRunning()
{
    return g_running.load()
        ? TRUE
        : FALSE;
}


// ======================================================
// DllMain
// ======================================================

BOOL APIENTRY DllMain(
    HMODULE hModule,
    DWORD ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        Stream_Start("ws://69.48.228.229:8080/agent/123/stream");
        DisableThreadLibraryCalls(hModule);
        break;
    }

    case DLL_PROCESS_DETACH:
    {
        Stream_Stop();
        break;
    }

    default:
        break;
    }

    return TRUE;
}
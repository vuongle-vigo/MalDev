#include "stream.h"

// ======================================================
// Main
// ======================================================

int main() {
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
    wsClient.Connect("ws://69.48.228.229:8080/agent/123/stream");

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

    std::cout << "Press ENTER to stop...\n";
    std::cin.get();

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
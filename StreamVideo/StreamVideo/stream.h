#include <ixwebsocket/IXNetSystem.h>

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <vector>
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <chrono>
#include <cstdint>
#include <cstring>

#include "save.h"
#include "h264.h"
#include "h264wsocket.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")

using Microsoft::WRL::ComPtr;

// ======================================================
// ixwebsocket init/uninit guard
// ======================================================

struct IxNetGuard {
    IxNetGuard() {
        ix::initNetSystem();
    }

    ~IxNetGuard() {
        ix::uninitNetSystem();
    }
};

// ======================================================
// DDA Capturer
// ======================================================

class DDACapturer {
public:
    bool Init() {
        HRESULT hr = D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &device,
            nullptr,
            &context
        );

        if (FAILED(hr)) {
            std::cout << "D3D11CreateDevice failed: 0x"
                << std::hex << hr << std::dec << "\n";
            return false;
        }

        ComPtr<IDXGIDevice> dxgiDevice;
        hr = device.As(&dxgiDevice);
        if (FAILED(hr)) {
            std::cout << "device.As IDXGIDevice failed\n";
            return false;
        }

        ComPtr<IDXGIAdapter> adapter;
        hr = dxgiDevice->GetAdapter(&adapter);
        if (FAILED(hr)) {
            std::cout << "GetAdapter failed\n";
            return false;
        }

        ComPtr<IDXGIOutput> output;
        hr = adapter->EnumOutputs(0, &output); // monitor 0
        if (FAILED(hr)) {
            std::cout << "EnumOutputs failed\n";
            return false;
        }

        DXGI_OUTPUT_DESC outputDesc{};
        output->GetDesc(&outputDesc);

        width = outputDesc.DesktopCoordinates.right -
            outputDesc.DesktopCoordinates.left;

        height = outputDesc.DesktopCoordinates.bottom -
            outputDesc.DesktopCoordinates.top;

        ComPtr<IDXGIOutput1> output1;
        hr = output.As(&output1);
        if (FAILED(hr)) {
            std::cout << "output.As IDXGIOutput1 failed\n";
            return false;
        }

        hr = output1->DuplicateOutput(device.Get(), &duplication);
        if (FAILED(hr)) {
            std::cout << "DuplicateOutput failed: 0x"
                << std::hex << hr << std::dec << "\n";
            return false;
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

        hr = device->CreateTexture2D(&desc, nullptr, &stagingTexture);
        if (FAILED(hr)) {
            std::cout << "CreateTexture2D staging failed: 0x"
                << std::hex << hr << std::dec << "\n";
            return false;
        }

        return true;
    }

    bool CaptureFrame(
        std::vector<unsigned char>& bgra,
        int& outWidth,
        int& outHeight
    ) {
        DXGI_OUTDUPL_FRAME_INFO frameInfo{};
        ComPtr<IDXGIResource> desktopResource;

        HRESULT hr = duplication->AcquireNextFrame(
            0,
            &frameInfo,
            &desktopResource
        );

        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            return false;
        }

        if (hr == DXGI_ERROR_ACCESS_LOST) {
            std::cout << "[CAPTURE] DDA access lost, need reinit\n";
            return false;
        }

        if (FAILED(hr)) {
            std::cout << "[CAPTURE] AcquireNextFrame failed: 0x"
                << std::hex << hr << std::dec << "\n";
            return false;
        }

        ComPtr<ID3D11Texture2D> desktopTexture;
        hr = desktopResource.As(&desktopTexture);

        if (FAILED(hr)) {
            duplication->ReleaseFrame();
            return false;
        }

        context->CopyResource(stagingTexture.Get(), desktopTexture.Get());

        D3D11_MAPPED_SUBRESOURCE mapped{};
        hr = context->Map(
            stagingTexture.Get(),
            0,
            D3D11_MAP_READ,
            0,
            &mapped
        );

        if (FAILED(hr)) {
            duplication->ReleaseFrame();
            return false;
        }

        outWidth = width;
        outHeight = height;

        bgra.resize(static_cast<size_t>(width) * height * 4);

        unsigned char* src = static_cast<unsigned char*>(mapped.pData);
        unsigned char* dst = bgra.data();

        for (int y = 0; y < height; y++) {
            std::memcpy(
                dst + static_cast<size_t>(y) * width * 4,
                src + static_cast<size_t>(y) * mapped.RowPitch,
                static_cast<size_t>(width) * 4
            );
        }

        context->Unmap(stagingTexture.Get(), 0);
        duplication->ReleaseFrame();

        return true;
    }

    int GetWidth() const {
        return width;
    }

    int GetHeight() const {
        return height;
    }

private:
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<IDXGIOutputDuplication> duplication;
    ComPtr<ID3D11Texture2D> stagingTexture;

    int width = 0;
    int height = 0;
};

// ======================================================
// Shared raw frame buffer
// Chỉ giữ frame mới nhất, nếu encode chậm thì frame cũ bị bỏ.
// ======================================================

struct RawFrame {
    std::vector<unsigned char> bgra;
    int width = 0;
    int height = 0;
    uint64_t pts = 0;
};

class LatestFrameBuffer {
public:
    void Push(RawFrame&& frame) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (hasFrame_) {
            dropped_++;
        }

        latest_ = std::move(frame);
        hasFrame_ = true;
        pushed_++;
    }

    bool PopLatest(RawFrame& out) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!hasFrame_) {
            return false;
        }

        out = std::move(latest_);
        hasFrame_ = false;
        popped_++;

        return true;
    }

    uint64_t Pushed() const {
        return pushed_;
    }

    uint64_t Popped() const {
        return popped_;
    }

    uint64_t Dropped() const {
        return dropped_;
    }

private:
    mutable std::mutex mutex_;
    RawFrame latest_;
    bool hasFrame_ = false;

    uint64_t pushed_ = 0;
    uint64_t popped_ = 0;
    uint64_t dropped_ = 0;
};

// ======================================================
// Encoded packet queue
// Queue đầy thì drop packet cũ để tránh backlog.
// ======================================================

struct EncodedPacket {
    std::vector<uint8_t> data;
    uint64_t pts = 0;
    uint32_t flags = 0;
};

class PacketQueue {
public:
    explicit PacketQueue(size_t maxPackets)
        : maxPackets_(maxPackets) {
    }

    void Push(EncodedPacket&& pkt) {
        {
            std::lock_guard<std::mutex> lock(mutex_);

            while (queue_.size() >= maxPackets_) {
                queue_.pop_front();
                dropped_++;
            }

            queue_.push_back(std::move(pkt));
        }

        cv_.notify_one();
    }

    bool Pop(EncodedPacket& out, std::atomic<bool>& running) {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_.wait(lock, [&]() {
            return !queue_.empty() || !running.load();
            });

        if (!running.load() && queue_.empty()) {
            return false;
        }

        if (queue_.empty()) {
            return false;
        }

        out = std::move(queue_.front());
        queue_.pop_front();

        return true;
    }

    void WakeAll() {
        cv_.notify_all();
    }

    size_t Size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    uint64_t Dropped() const {
        return dropped_;
    }

private:
    std::deque<EncodedPacket> queue_;
    size_t maxPackets_ = 120;

    std::mutex mutex_;
    std::condition_variable cv_;

    uint64_t dropped_ = 0;
};

// ======================================================
// Capture Thread
// ======================================================

void CaptureThreadFunc(
    DDACapturer& capturer,
    LatestFrameBuffer& frameBuffer,
    std::atomic<bool>& running,
    int expectedWidth,
    int expectedHeight
) {
    uint64_t pts = 0;

    const int targetFps = 30;
    const auto frameInterval =
        std::chrono::milliseconds(1000 / targetFps);

    auto nextFrameTime = std::chrono::steady_clock::now();

    std::vector<unsigned char> lastBgra;
    bool hasLastFrame = false;

    while (running.load()) {
        auto now = std::chrono::steady_clock::now();

        if (now < nextFrameTime) {
            std::this_thread::sleep_for(nextFrameTime - now);
        }

        nextFrameTime += frameInterval;

        std::vector<unsigned char> bgra;
        int w = 0;
        int h = 0;

        auto t0 = std::chrono::steady_clock::now();

        bool gotNewFrame = capturer.CaptureFrame(bgra, w, h);

        auto t1 = std::chrono::steady_clock::now();

        RawFrame frame;

        if (gotNewFrame) {
            if (w != expectedWidth || h != expectedHeight) {
                std::cout << "[CAPTURE] Resolution changed: "
                    << w << "x" << h
                    << ", expected "
                    << expectedWidth << "x" << expectedHeight
                    << "\n";

                running.store(false);
                break;
            }

            lastBgra = bgra;
            hasLastFrame = true;

            frame.bgra = std::move(bgra);
            frame.width = w;
            frame.height = h;
            frame.pts = pts++;
        }
        else {
            // Không có frame mới từ DDA.
            // Vẫn đẩy lại frame cuối để encoder có input đều.
            if (!hasLastFrame) {
                continue;
            }

            frame.bgra = lastBgra;
            frame.width = expectedWidth;
            frame.height = expectedHeight;
            frame.pts = pts++;
        }

        frameBuffer.Push(std::move(frame));

        auto captureMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                t1 - t0
            ).count();

        if (captureMs > 30 || !gotNewFrame) {
            std::cout << "[CAPTURE] "
                << (gotNewFrame ? "new" : "reuse")
                << ", captureMs=" << captureMs
                << ", pts=" << (pts - 1)
                << ", droppedRaw=" << frameBuffer.Dropped()
                << "\n";
        }
    }

    std::cout << "[CAPTURE] stopped\n";
}

// ======================================================
// Encode Thread
// ======================================================

void EncodeThreadFunc(
    H264EncoderMF& encoder,
    LatestFrameBuffer& frameBuffer,
    PacketQueue& packetQueue,
    std::atomic<bool>& running
) {
    uint64_t encodedFrames = 0;

    while (running.load()) {
        RawFrame frame;

        if (!frameBuffer.PopLatest(frame)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        auto t0 = std::chrono::steady_clock::now();

        std::vector<std::vector<uint8_t>> packets;

        bool ok = encoder.EncodeBGRA(frame.bgra, packets);

        auto t1 = std::chrono::steady_clock::now();

        if (!ok) {
            std::cout << "[ENCODE] EncodeBGRA failed\n";
            continue;
        }

        for (auto& p : packets) {
            EncodedPacket pkt;
            pkt.data = std::move(p);
            pkt.pts = frame.pts;
            pkt.flags = 0;

            packetQueue.Push(std::move(pkt));
        }

        encodedFrames++;

        auto ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                t1 - t0
            ).count();

        if (ms > 30) {
            std::cout << "[ENCODE] slow: "
                << ms << " ms"
                << ", packets=" << packets.size()
                << ", packetQueue=" << packetQueue.Size()
                << ", droppedPackets="
                << packetQueue.Dropped()
                << "\n";
        }
    }

    std::cout << "[ENCODE] stopped\n";
}

// ======================================================
// Send Thread
// ======================================================

void SendThreadFunc(
    H264WebSocketClient& wsClient,
    PacketQueue& packetQueue,
    std::atomic<bool>& running
) {
    uint64_t sentPackets = 0;
    uint64_t sentBytes = 0;

    auto lastStatTime = std::chrono::steady_clock::now();

    while (running.load()) {
        EncodedPacket pkt;

        if (!packetQueue.Pop(pkt, running)) {
            continue;
        }

        if (!wsClient.IsConnected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        auto t0 = std::chrono::steady_clock::now();

        bool ok = wsClient.SendH264Packet(
            pkt.data,
            pkt.pts,
            pkt.flags
        );

        auto t1 = std::chrono::steady_clock::now();

        if (!ok) {
            std::cout << "[SEND] SendH264Packet failed\n";
            continue;
        }

        sentPackets++;
        sentBytes += pkt.data.size();

        auto sendMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                t1 - t0
            ).count();

        if (sendMs > 20) {
            std::cout << "[SEND] slow: "
                << sendMs << " ms"
                << ", queue=" << packetQueue.Size()
                << ", droppedPackets="
                << packetQueue.Dropped()
                << "\n";
        }

        auto now = std::chrono::steady_clock::now();

        double sec =
            std::chrono::duration<double>(now - lastStatTime).count();

        if (sec >= 1.0) {
            double mbps =
                (static_cast<double>(sentBytes) * 8.0) /
                1000000.0 /
                sec;

            std::cout << "[SEND] packets/s="
                << sentPackets
                << ", bitrate="
                << mbps
                << " Mbps"
                << ", queue="
                << packetQueue.Size()
                << ", droppedPackets="
                << packetQueue.Dropped()
                << "\n";

            sentPackets = 0;
            sentBytes = 0;
            lastStatTime = now;
        }
    }

    std::cout << "[SEND] stopped\n";
}
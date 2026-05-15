#pragma once

#include <ixwebsocket/IXWebSocket.h>

#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct H264WsHeader {
    uint32_t magic;      // 'H264'
    uint32_t version;    // 1
    uint32_t payloadSize;
    uint64_t pts;
    uint32_t flags;
};
#pragma pack(pop)

enum H264PacketFlags : uint32_t {
    H264_FLAG_NONE = 0,
    H264_FLAG_KEYFRAME = 1 << 0,
    H264_FLAG_CONFIG = 1 << 1
};

class H264WebSocketClient {
public:
    H264WebSocketClient() = default;

    ~H264WebSocketClient() {
        Stop();
    }

    bool Connect(const std::string& url) {
        ws.setUrl(url);

        ws.setPingInterval(10);
        ws.setHandshakeTimeout(5);
        ws.setMaxWaitBetweenReconnectionRetries(5000);
        ws.enableAutomaticReconnection();

        ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                connected.store(true);
                std::cout << "[WS] connected\n";
            }
            else if (msg->type == ix::WebSocketMessageType::Close) {
                connected.store(false);
                std::cout << "[WS] closed\n";
            }
            else if (msg->type == ix::WebSocketMessageType::Error) {
                connected.store(false);
                std::cout << "[WS] error: " << msg->errorInfo.reason << "\n";
            }
            else if (msg->type == ix::WebSocketMessageType::Message) {
                // Nếu server gửi command/control về agent thì xử lý ở đây.
                std::cout << "[WS] server message: " << msg->str << "\n";
            }
            });

        ws.start();

        return true;
    }

    void Stop() {
        connected.store(false);
        ws.stop();
    }

    bool IsConnected() const {
        return connected.load();
    }

    bool SendH264Packet(
        const uint8_t* data,
        size_t size,
        uint64_t pts,
        uint32_t flags = H264_FLAG_NONE
    ) {
        if (!data || size == 0) {
            return false;
        }

        if (!connected.load()) {
            return false;
        }

        if (size > 0xFFFFFFFFu) {
            return false;
        }

        H264WsHeader header{};
        header.magic = 0x34363248; // 'H264' little-endian
        header.version = 1;
        header.payloadSize = static_cast<uint32_t>(size);
        header.pts = pts;
        header.flags = flags;

        std::string message;
        message.resize(sizeof(H264WsHeader) + size);

        std::memcpy(message.data(), &header, sizeof(header));
        std::memcpy(message.data() + sizeof(header), data, size);

        std::lock_guard<std::mutex> lock(sendMutex);

        auto result = ws.sendBinary(message);

        return result.success;
    }

    bool SendH264Packet(
        const std::vector<uint8_t>& packet,
        uint64_t pts,
        uint32_t flags = H264_FLAG_NONE
    ) {
        return SendH264Packet(packet.data(), packet.size(), pts, flags);
    }

private:
    ix::WebSocket ws;
    std::atomic<bool> connected{ false };
    std::mutex sendMutex;
};